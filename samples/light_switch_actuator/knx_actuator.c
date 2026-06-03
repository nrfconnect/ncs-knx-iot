/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Actuator device profile: KNX functional block 417 (LSAB). Owns everything
 * role-specific and registers it with the generic application layer. The stack,
 * event loop, resource registration and shared GPIO come from the
 * common layer.
 */

#include "app.h"

#include <knx/knx_app.h>
#include <knx/knx_board.h>
#include <knx/knx_device.h>
#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
#include "knx_hardcoded.h"
#include <knx/knx_presets.h>
#endif

#include <zephyr/logging/log.h>

#include "oc_knx_fp.h"

LOG_MODULE_REGISTER(knx_actuator, LOG_LEVEL_INF);

static void actuator_set_light(bool on)
{
	knx_board_set_app_led(on);
}

/* Datapoint indices within a switching channel. */
#define SOO	   0 /* switch on/off (control) */
#define IOO	   1 /* info on/off (status) */
#define NUM_POINTS 2

#define NUM_CHANNELS 2

/*
 * Identity. serialnumber and application_name are non-static because the
 * compiled libknx shell (port/zephyr/knx_shell.c) references them by name.
 */
const char sn_lower_case[] = "00fa10020900";
const char application_name[] = "KNX virtual actuator (LSAB)";

static const knx_identity_t actuator_identity = {
	.serialnumber = sn_lower_case,
	.application_name = application_name,
	.hw_type = "000102030405",
	.dev_model = "6800",
	.mid = 0x00fa,
};

/* LSAB: soo is the control input (GET + PUT, if.i) and mirrors onto ioo, the
 * status output (GET, if.o). During typical operation, the actuator listens for
 * s-mode multicasts on its SOO from devices bound to the same GA. When this
 * happens, it sets the indicator led, mirrors the value to IOO and announces
 * it, so the switch can know it worked.
 */
static knx_datapoint_t actuator_datapoints[] = {
	{.path = "/p/lsab/0/soo",
	 .dpa = "urn:knx:dpa.417.52",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(0, SOO),
	 .methods = OC_GET | OC_PUT,
	 .acl = OC_ACL_I,
	 .iface = OC_IF_I,
	 .mirror_to = KNX_DP_ID(0, IOO),
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lsab/0/ioo",
	 .dpa = "urn:knx:dpa.417.51",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(0, IOO),
	 .methods = OC_GET,
	 .acl = OC_ACL_O,
	 .iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lsab/1/soo",
	 .dpa = "urn:knx:dpa.417.52",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(1, SOO),
	 .methods = OC_GET | OC_PUT,
	 .acl = OC_ACL_I,
	 .iface = OC_IF_I,
	 .mirror_to = KNX_DP_ID(1, IOO),
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lsab/1/ioo",
	 .dpa = "urn:knx:dpa.417.51",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(1, IOO),
	 .methods = OC_GET,
	 .acl = OC_ACL_O,
	 .iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
};

static const knx_functional_block_t actuator_blocks[] = {
	{.number = 417,
	 .instance = 1,
	 .datapoints = &actuator_datapoints[0],
	 .num_datapoints = NUM_POINTS},
	{.number = 417,
	 .instance = 2,
	 .datapoints = &actuator_datapoints[NUM_POINTS],
	 .num_datapoints = NUM_POINTS},
};

#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
/* Actuator: individual address 1.1.1, receives writes on the shared GA. */
static const knx_preset_t actuator_preset = {
	.fid = KNX_HC_FID,
	.iid = KNX_HC_IID,
	.ga = KNX_HC_GA,
	.grpid = KNX_HC_GRPID,
	.ia = 0x1101,
	.got_href = "/p/lsab/0/soo",
	.got_cflags = OC_CFLAG_WRITE,
	.is_publisher = true,
	.group_ms = knx_hc_group_ms,
	.group_ms_len = sizeof(knx_hc_group_ms),
	.group_kid = knx_hc_group_kid,
	.group_kid_len = sizeof(knx_hc_group_kid),
};
#endif

/* Runs on the KNX thread after a PUT: mirror the switched state to the LED. */
static void actuator_on_write(const knx_datapoint_t *dp)
{
	bool value;

	if (KNX_DP_POINT(dp->id) == SOO) {
		if (knx_datapoint_get_bool(dp->id, &value) < 0) {
			LOG_ERR("failed to read written actuator SOO");
			return;
		}

		actuator_set_light(value);
	}
}

static void actuator_on_init(void)
{
	bool value = false;

	if (knx_datapoint_get_bool(KNX_DP_ID(0, SOO), &value) < 0) {
		LOG_ERR("failed to read initial actuator SOO");
	}

	actuator_set_light(value);
}

static const knx_device_t actuator_device = {
	.identity = &actuator_identity,
	.functional_blocks = actuator_blocks,
	.num_functional_blocks = NUM_CHANNELS,
	.on_init = actuator_on_init,
	.on_ready = NULL,
	.on_write = actuator_on_write,
#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
	.preset = &actuator_preset,
#else
	.preset = NULL,
#endif
};

void app_register_device(void)
{
	knx_device_register(&actuator_device);
}
