/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app.h"

#include <knx/knx_app.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(light_switch_sensor, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("KNX IoT light switch sensor");

	app_register_device();

	if (knx_app_start()) {
		LOG_ERR("Error Initiating KNX App");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
