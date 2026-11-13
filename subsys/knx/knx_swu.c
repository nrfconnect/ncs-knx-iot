/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * KNX IoT software update (swu) on top of MCUboot.
 *
 * The package pushed to a/swu is a signed MCUboot image. It is streamed into the
 * secondary slot through the DFU target library. swu/update requests a test swap
 * and reboots. MCUboot verifies the image before it swaps. The next boot confirms
 * the new image and reports the result. If the new image is never confirmed,
 * MCUboot reverts to the previous one.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>

#include "knx_priv.h"

#include "oc_api.h"
#include "oc_knx_swu.h"

LOG_MODULE_REGISTER(knx_swu, LOG_LEVEL_INF);

/* Marker that a swap was requested. Holds the version of the requested image so
 * the next boot can tell whether that image is the one running.
 */
#define SWU_PENDING_KEY "knx_swu/pending"

/* The stack sends the swu/update response after the upgrade callback returns. */
#define SWU_APPLY_MIN_DELAY_S 2

/* An MCUboot image the device can update, identified by its MCUboot image number. */
struct swu_image {
	int num;
	/* Flash area of the running image. */
	uint8_t primary_area;
	/* Flash area that receives the update. */
	uint8_t upload_area;
};

/* The application image, the only image updated through a/swu. */
static const struct swu_image app_image = {
	.num = 0,
	.primary_area = PARTITION_ID(slot0_partition),
	.upload_area = PARTITION_ID(slot1_partition),
};

static struct {
	const struct swu_image *image;
	/* The DFU target is initialized for image and must be reset before reuse. */
	bool target_open;
	/* Receiving into the upload slot. */
	bool active;
	/* Image bytes received in order from offset 0. */
	size_t written;
	/* Package size announced with the first block. */
	size_t size;
	/* Version of the downloaded image. */
	struct mcuboot_img_sem_ver version;
} download;

/* Collects received data until a full flash write block is available. */
static uint8_t write_buf[CONFIG_KNX_SWU_WRITE_BUFFER_SIZE] __aligned(4);

/* Discards the image written so far and releases the DFU target. */
static void target_close(void)
{
	if (!download.target_open) {
		return;
	}

	const int rc = dfu_target_reset();

	if (rc != 0) {
		LOG_WRN("Cannot reset the update target: %d", rc);
	}
	download.target_open = false;
}

static int target_init(const struct swu_image *image, size_t size)
{
	int rc = dfu_target_mcuboot_set_buf(write_buf, sizeof(write_buf));

	if (rc == 0) {
		rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, image->num, size, NULL);
	}
	download.target_open = rc == 0;

	return rc;
}

static void download_fail(oc_swu_result_t result)
{
	LOG_ERR("Download failed at %zu bytes, result %d", download.written, result);

	download.active = false;
	target_close();
	oc_swu_set_result(result);
	oc_swu_set_state(OC_SWU_STATE_IDLE);
}

static void download_complete(void)
{
	struct mcuboot_img_header hdr;
	char name[sizeof("fw-255.255.65535")];
	const int rc = boot_read_bank_header(download.image->upload_area, &hdr, sizeof(hdr));

	download.active = false;

	if (rc != 0) {
		LOG_ERR("Cannot read the downloaded image header: %d", rc);
		download_fail(OC_SWU_RESULT_ERR_UPT);
		return;
	}

	download.version = hdr.h.v1.sem_ver;

	const struct mcuboot_img_sem_ver *ver = &download.version;

	snprintf(name, sizeof(name), "fw-%u.%u.%u", ver->major, ver->minor, ver->revision);
	oc_swu_set_package_name(name);
	oc_swu_set_package_version(ver->major, ver->minor, ver->revision);
	oc_swu_set_state(OC_SWU_STATE_DOWNLOADED);

	LOG_INF("Downloaded image %u.%u.%u+%u (%zu bytes)", ver->major, ver->minor, ver->revision,
		ver->build_num, download.size);
}

static oc_event_callback_retval_t swu_apply(void *data);

static bool download_start(const struct swu_image *image, size_t size, const uint8_t *block,
			   size_t block_len)
{
	int rc;

	oc_remove_delayed_callback(NULL, swu_apply);
	target_close();
	memset(&download, 0, sizeof(download));
	download.image = image;
	download.size = size;

	if (size == 0) {
		LOG_ERR("The first block does not announce the package size");
		download_fail(OC_SWU_RESULT_ERR_SUF);
		return false;
	}

	if (dfu_target_img_type(block, block_len) != DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
		LOG_ERR("Package is not an MCUboot image");
		download_fail(OC_SWU_RESULT_ERR_UPT);
		return false;
	}

	rc = target_init(image, size);
	if (rc != 0) {
		LOG_ERR("Cannot prepare the update target for %zu bytes: %d", size, rc);
		download_fail(OC_SWU_RESULT_ERR_FLASH);
		return false;
	}

	download.active = true;
	LOG_INF("Download started, package size %zu", size);
	return true;
}

static void swu_cb(oc_separate_response_t *response, size_t binary_size, size_t block_offset,
		   const uint8_t *block_data, size_t block_len, void *data)
{
	ARG_UNUSED(response);
	ARG_UNUSED(data);

	if (block_offset == 0 &&
	    !download_start(&app_image, binary_size, block_data, block_len)) {
		return;
	}

	/* The stack passes the blocks of a download in order, without gaps or repeats. */
	if (!download.active || block_offset != download.written) {
		LOG_ERR("Block at %zu does not continue the download (%zu bytes, %s)",
			block_offset, download.written, download.active ? "active" : "not started");
		download_fail(OC_SWU_RESULT_ERR_CONN);
		return;
	}

	if (block_len > download.size - download.written) {
		LOG_ERR("Data beyond the announced package size %zu", download.size);
		download_fail(OC_SWU_RESULT_ERR_ICF);
		return;
	}

	const bool last = download.written + block_len == download.size;
	int rc = dfu_target_write(block_data, block_len);

	if (rc == 0 && last) {
		rc = dfu_target_done(true);
	}
	if (rc != 0) {
		LOG_ERR("Writing the update slot failed: %d", rc);
		download_fail(OC_SWU_RESULT_ERR_FLASH);
		return;
	}

	download.written += block_len;

	if (last) {
		download_complete();
	}
}

static void apply_fail(void)
{
	(void)settings_delete(SWU_PENDING_KEY);
	oc_swu_set_result(OC_SWU_RESULT_ERR_SUF);
	oc_swu_set_state(OC_SWU_STATE_DOWNLOADED);
}

static oc_event_callback_retval_t swu_apply(void *data)
{
	ARG_UNUSED(data);

	int rc = settings_save_one(SWU_PENDING_KEY, &download.version, sizeof(download.version));

	if (rc != 0) {
		LOG_ERR("Cannot store the pending update marker: %d", rc);
		apply_fail();
		return OC_EVENT_DONE;
	}

	rc = dfu_target_schedule_update(download.image->num);
	if (rc != 0) {
		LOG_ERR("Upgrade request failed: %d", rc);
		apply_fail();
		return OC_EVENT_DONE;
	}

	LOG_INF("Rebooting to apply the update");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);

	return OC_EVENT_DONE;
}

static void swu_upgrade_cb(int defer_time, void *data)
{
	ARG_UNUSED(data);

	const uint16_t delay = CLAMP(defer_time, SWU_APPLY_MIN_DELAY_S, UINT16_MAX);

	LOG_INF("Update requested, applying in %u s", delay);
	oc_set_delayed_callback(NULL, swu_apply, delay);
}

void knx_swu_register(void)
{
	oc_set_swu_cb(swu_cb, NULL);
	oc_set_swu_upgrade_cb(swu_upgrade_cb, NULL);
}

void knx_swu_running_version(oc_knx_version_info_t *version)
{
	struct mcuboot_img_header hdr;

	if (boot_read_bank_header(app_image.primary_area, &hdr, sizeof(hdr)) != 0) {
		LOG_WRN("Cannot read the running image header, keeping firmware version %u.%u.%u",
			version->major, version->minor, version->patch);
		return;
	}

	version->major = hdr.h.v1.sem_ver.major;
	version->minor = hdr.h.v1.sem_ver.minor;
	version->patch = hdr.h.v1.sem_ver.revision;
}

static bool running_image_is(const struct swu_image *image,
			     const struct mcuboot_img_sem_ver *expected)
{
	struct mcuboot_img_header hdr;
	const struct mcuboot_img_sem_ver *running = &hdr.h.v1.sem_ver;

	return boot_read_bank_header(image->primary_area, &hdr, sizeof(hdr)) == 0 &&
	       running->major == expected->major && running->minor == expected->minor &&
	       running->revision == expected->revision &&
	       running->build_num == expected->build_num;
}

void knx_swu_boot_complete(void)
{
	struct mcuboot_img_sem_ver requested;
	const bool pending = settings_load_one(SWU_PENDING_KEY, &requested, sizeof(requested)) ==
			     sizeof(requested);

	if (!boot_is_img_confirmed()) {
		const int rc = boot_write_img_confirmed();

		if (rc != 0) {
			/* MCUboot reverts to the previous image on the next reset. */
			LOG_ERR("Cannot confirm the running image: %d", rc);
			return;
		}
		LOG_INF("Running image confirmed");
	}

	if (!pending) {
		return;
	}

	(void)settings_delete(SWU_PENDING_KEY);

	if (!running_image_is(&app_image, &requested)) {
		LOG_ERR("Requested update is not running, MCUboot rejected or reverted it");
		oc_swu_set_result(OC_SWU_RESULT_ERR_SUF);
		return;
	}

	LOG_INF("Software update applied");
	oc_swu_set_last_update(OC_SWU_LAST_UPDATE_OSV);
	oc_swu_set_result(OC_SWU_RESULT_SUCCESS);
}
