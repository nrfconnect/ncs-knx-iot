/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Sensor preset for hardcoded commissioning. Only built with
 * CONFIG_KNX_HARDCODED_COMMISSIONING.
 */

#include "knx_hardcoded.h"

#include <knx/knx_presets.h>

#include <zephyr/sys/util.h>

#include "oc_knx_fp.h"

/* IOO listens to the GA of its channel, so every sensor learns the state that
 * another sensor switched and toggles from it.
 */
#define SENSOR_IOO_CFLAGS (OC_CFLAG_WRITE | OC_CFLAG_UPDATE)

static const struct knx_preset_group_object sensor_group_objects[] = {
	{.href = "/p/lssb/0/soo", .ga = KNX_HC_GA_CH0, .cflags = OC_CFLAG_TRANSMISSION},
	{.href = "/p/lssb/0/ioo", .ga = KNX_HC_GA_CH0, .cflags = SENSOR_IOO_CFLAGS},
	{.href = "/p/lssb/1/soo", .ga = KNX_HC_GA_CH1, .cflags = OC_CFLAG_TRANSMISSION},
	{.href = "/p/lssb/1/ioo", .ga = KNX_HC_GA_CH1, .cflags = SENSOR_IOO_CFLAGS},
};

/* Sensor: transmits on the channel GAs. The individual address 1.1.2 is only
 * used with CONFIG_KNX_HARDCODED_IDENTITY_FIXED.
 */
const knx_preset_t knx_hardcoded_preset = {
	.fid = KNX_HC_FID,
	.iid = KNX_HC_IID,
	.grpid = KNX_HC_GRPID,
	.ia = 0x1102,
	.group_objects = sensor_group_objects,
	.num_group_objects = ARRAY_SIZE(sensor_group_objects),
	.group_ms = knx_hc_group_ms,
	.group_ms_len = sizeof(knx_hc_group_ms),
	.group_kid = knx_hc_group_kid,
	.group_kid_len = sizeof(knx_hc_group_kid),
};
