/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Actuator preset for hardcoded commissioning. Only built with
 * CONFIG_KNX_HARDCODED_COMMISSIONING.
 */

#include "knx_hardcoded.h"

#include <knx/knx_presets.h>

#include <zephyr/sys/util.h>

#include "oc_knx_fp.h"

static const struct knx_preset_group_object actuator_group_objects[] = {
	{.href = "/p/lsab/0/soo", .ga = KNX_HC_GA_CH0, .cflags = OC_CFLAG_WRITE},
	{.href = "/p/lsab/1/soo", .ga = KNX_HC_GA_CH1, .cflags = OC_CFLAG_WRITE},
};

/* Actuator: receives writes on the channel GAs. The individual address 1.1.1
 * is only used with CONFIG_KNX_HARDCODED_IDENTITY_FIXED.
 */
const knx_preset_t knx_hardcoded_preset = {
	.fid = KNX_HC_FID,
	.iid = KNX_HC_IID,
	.grpid = KNX_HC_GRPID,
	.ia = 0x1101,
	.group_objects = actuator_group_objects,
	.num_group_objects = ARRAY_SIZE(actuator_group_objects),
	.group_ms = knx_hc_group_ms,
	.group_ms_len = sizeof(knx_hc_group_ms),
	.group_kid = knx_hc_group_kid,
	.group_kid_len = sizeof(knx_hc_group_kid),
};
