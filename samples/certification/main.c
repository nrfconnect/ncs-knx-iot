/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app.h"

#include <knx/knx_app.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(certification, LOG_LEVEL_INF);

int main(void)
{
	int err;

	LOG_INF("KNX IoT certification device");

	app_register_device();

	err = knx_app_start();

	if (err) {
		LOG_ERR("Failed to start KNX application: %d", err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
