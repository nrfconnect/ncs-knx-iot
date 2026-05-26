/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(light_switch_actuator, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("KNX IoT light switch actuator");

	return 0;
}
