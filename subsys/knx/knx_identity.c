/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Device identity derived from the hardware device ID, so one firmware image
 * gives every device a unique serial number and Individual Address.
 */

#include "knx_priv.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "oc_endpoint.h"

LOG_MODULE_REGISTER(knx_identity, LOG_LEVEL_INF);

/* Manufacturer ID characters kept from the configured serial number. */
#define MID_CHARS 4

/* Individual Address ranges: areas 1-15, lines 0-15, devices 1-254. */
#define IA_AREAS   15U
#define IA_LINES   16U
#define IA_DEVICES 254U

BUILD_ASSERT(SERIAL_NUM_SIZE == MID_CHARS + 2 * sizeof(uint32_t),
	     "the serial number must hold the manufacturer ID and 32 bits of device ID");

static char serial_number[SERIAL_NUM_SIZE + 1];
static uint32_t device_bits;

int knx_identity_hwinfo_init(const char *base_serial_number)
{
	uint8_t id[16];
	ssize_t len = hwinfo_get_device_id(id, sizeof(id));

	if (len < 0) {
		LOG_ERR("failed to read the hardware device ID: %d", (int)len);
		return (int)len;
	}

	if (len == 0) {
		LOG_ERR("the hardware device ID is empty");
		return -ENODATA;
	}

	/* Fold the whole device ID so no part of it is discarded. */
	device_bits = 0;
	for (ssize_t i = 0; i < len; i++) {
		device_bits ^= (uint32_t)id[i] << (8 * (3 - (i % 4)));
	}

	(void)snprintf(serial_number, sizeof(serial_number), "%.*s%08" PRIx32, MID_CHARS,
		       base_serial_number, device_bits);

	LOG_INF("serial number %s derived from the hardware device ID", serial_number);

	return 0;
}

const char *knx_identity_hwinfo_serial_number(void)
{
	return serial_number;
}

int32_t knx_identity_hwinfo_ia(void)
{
	uint32_t n = device_bits % (IA_AREAS * IA_LINES * IA_DEVICES);
	const uint32_t device = 1U + n % IA_DEVICES;

	n /= IA_DEVICES;

	const uint32_t line = n % IA_LINES;
	const uint32_t area = 1U + n / IA_LINES;

	return (int32_t)((area << 12) | (line << 8) | device);
}
