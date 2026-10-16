/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Hardcoded "automatic" commissioning mechanism. Writes the identity, load
 * state, group object / publisher / recipient tables and a shared group OSCORE
 * key directly into the stack so two boards can exchange s-mode group messages
 * with no ETS or Thread commissioner.
 */

#include <knx/knx_presets.h>

#include "knx_priv.h"

#include "oc_core_res.h"
#include "oc_endpoint.h"
#include "oc_helpers.h"
#include "oc_knx.h"
#include "oc_knx_dev.h"
#include "oc_knx_fp.h"
#include "oc_knx_sec.h"
#include "port/oc_storage.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(knx_presets, LOG_LEVEL_INF);

/* Group objects with these flags send on their group address. */
#define SENDING_CFLAGS	 (OC_CFLAG_TRANSMISSION | OC_CFLAG_INIT)
/* Group objects with these flags receive on their group address. */
#define RECEIVING_CFLAGS (OC_CFLAG_WRITE | OC_CFLAG_UPDATE | OC_CFLAG_READ)

/* The stack owns (and frees) the ga arrays, so each must be its own heap slot.
 */
static uint32_t *dup_ga(uint32_t ga)
{
	uint32_t *slot = malloc(sizeof(uint32_t));

	if (slot != NULL) {
		*slot = ga;
	}

	return slot;
}

/* Unique group addresses of the group objects with any of the given flags, in
 * a heap array of their own for the stack to own. *ga is NULL if there are none.
 */
static int collect_gas(const knx_preset_t *preset, uint32_t cflags, uint32_t **ga, uint16_t *len)
{
	*ga = malloc(preset->num_group_objects * sizeof(uint32_t));
	*len = 0;
	if (*ga == NULL) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < preset->num_group_objects; i++) {
		const struct knx_preset_group_object *go = &preset->group_objects[i];
		bool found = false;

		if ((go->cflags & cflags) == 0) {
			continue;
		}

		for (uint16_t j = 0; j < *len && !found; j++) {
			found = (*ga)[j] == go->ga;
		}

		if (!found) {
			(*ga)[(*len)++] = go->ga;
		}
	}

	if (*len == 0) {
		free(*ga);
		*ga = NULL;
	}

	return 0;
}

/* Takes ownership of ga. */
static int setup_hardcoded_group_table(oc_group_table_t *entry, uint32_t *ga, uint16_t ga_len,
				       uint32_t grpid)
{
	if (entry == NULL) {
		free(ga);
		return -EINVAL;
	}

	if (entry->id >= 0) {
		oc_free_string(&entry->at);
		free(entry->ga);
	}

	entry->id = 0;
	entry->ia = -1;
	entry->iid = -1;
	entry->fid = -1;
	entry->grpid = grpid;
	entry->non = true;
	entry->ga_len = ga_len;
	entry->ga = ga;

	oc_new_string(&entry->at, "", 0);

	return 0;
}

static int setup_hardcoded_got(int index, uint32_t ga, const char *href, oc_cflag_mask_t cflags)
{
	oc_group_object_table_t *got = oc_core_get_group_object_table_entry(index);

	if (got == NULL) {
		return -EINVAL;
	}

	if (got->id >= 0) {
		oc_free_string(&got->href);
		free(got->ga);
	}

	got->id = index;
	got->cflags = cflags;
	got->ga_len = 1;
	got->ga = dup_ga(ga);
	if (got->ga == NULL) {
		return -ENOMEM;
	}

	oc_new_string(&got->href, href, strlen(href));
	oc_store_group_object_table_entry(index);

	return 0;
}

/* An unused access token entry is one whose id is still the empty string. */
static oc_auth_at_t *find_empty_at_entry(void)
{
	const int table_size = oc_core_get_at_table_size();

	for (int i = 0; i < table_size; i++) {
		oc_auth_at_t *at = oc_get_auth_at_entry(i);

		if (at != NULL && oc_string_len(at->id) == 0) {
			return at;
		}
	}

	return NULL;
}

/* The serial number is the OSCORE ID Context, so receivers keep a separate
 * context and replay window for every sender of the shared group key.
 */
static int setup_hardcoded_access_token(const knx_preset_t *preset)
{
	oc_auth_at_t *at = find_empty_at_entry();
	const char *serial_number = oc_core_get_device_info()->serialnumber;
	uint8_t context_id[SERIAL_NUM_SIZE / 2];
	uint16_t ga_len;

	if (at == NULL) {
		return -ENOMEM;
	}

	if (hex2bin(serial_number, strlen(serial_number), context_id, sizeof(context_id)) !=
	    sizeof(context_id)) {
		return -EINVAL;
	}

	if (collect_gas(preset, UINT32_MAX, &at->ga, &ga_len) < 0) {
		return -ENOMEM;
	}
	at->ga_len = ga_len;

	oc_new_string(&at->id, "hardcoded-at-0", 14);
	at->profile = OC_PROFILE_COAP_OSCORE;
	at->scope = OC_ACL_GA;

	oc_new_byte_string(&at->osc_id, (char *)preset->group_kid, preset->group_kid_len);
	oc_new_byte_string(&at->osc_ms, (char *)preset->group_ms, preset->group_ms_len);
	oc_new_byte_string(&at->osc_salt, "", 0);
	oc_new_byte_string(&at->osc_contextid, (char *)context_id, sizeof(context_id));

	return 0;
}

int knx_apply_presets(const knx_preset_t *preset)
{
	uint32_t *ga;
	uint16_t ga_len;
	bool has_publisher_table;
	int ret;

	if (preset == NULL || preset->num_group_objects == 0) {
		return -EINVAL;
	}

#if defined(CONFIG_KNX_HARDCODED_IDENTITY_HWINFO)
	const int32_t ia = knx_identity_hwinfo_ia();
#else
	const int32_t ia = preset->ia;
#endif

	LOG_DBG("applying hardcoded KNX presets (fid=%llu iid=%llu ia=%d)",
		(unsigned long long)preset->fid, (unsigned long long)preset->iid, ia);

	oc_core_set_and_store_device_fid(preset->fid);
	oc_core_set_and_store_device_iid(preset->iid);
	oc_core_set_and_store_device_ia(ia);
	oc_knx_set_and_store_lsm(LSM_S_LOADED);

	oc_device_info_t *device = oc_core_get_device_info();

	device->pm = false;

	for (size_t i = 0; i < preset->num_group_objects; i++) {
		const struct knx_preset_group_object *go = &preset->group_objects[i];

		ret = setup_hardcoded_got((int)i, go->ga, go->href, (oc_cflag_mask_t)go->cflags);
		if (ret < 0) {
			LOG_ERR("failed to configure group object table: %d", ret);
			return ret;
		}
	}

	ret = collect_gas(preset, RECEIVING_CFLAGS, &ga, &ga_len);
	has_publisher_table = ga != NULL;
	if (ret == 0 && has_publisher_table) {
		ret = setup_hardcoded_group_table(oc_core_get_publisher_table_entry(0), ga, ga_len,
						  preset->grpid);
	}
	if (ret < 0) {
		LOG_ERR("failed to configure publisher table: %d", ret);
		return ret;
	}

	ret = collect_gas(preset, SENDING_CFLAGS, &ga, &ga_len);
	if (ret == 0 && ga != NULL) {
		ret = setup_hardcoded_group_table(oc_core_get_recipient_table_entry(0), ga, ga_len,
						  preset->grpid);
	}
	if (ret < 0) {
		LOG_ERR("failed to configure recipient table: %d", ret);
		return ret;
	}

	ret = setup_hardcoded_access_token(preset);
	if (ret < 0) {
		LOG_ERR("failed to configure group OSCORE access token: %d", ret);
		return ret;
	}

	oc_init_oscore_from_storage(true);

	if (has_publisher_table) {
		/* Re-register: the PUB table was empty during oc_main_init(). */
		oc_register_group_multicasts();
	}

	LOG_INF("hardcoded KNX presets applied");

	return 0;
}
