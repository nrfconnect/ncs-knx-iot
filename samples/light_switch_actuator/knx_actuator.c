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
#include <zephyr/sys/util.h>

#include "oc_knx_fp.h"

LOG_MODULE_REGISTER(knx_actuator, LOG_LEVEL_INF);

/* Datapoint indices within a switching channel. */
#define SOO	   0 /* switch on/off (control) */
#define IOO	   1 /* info on/off (status) */
#define NUM_POINTS 2

#define NUM_CHANNELS 2

/* The light blinks while the actuator reads the current state from the group
 * after startup (I-flag on SOO), and then shows it. The stored state is used if
 * no response arrives.
 */
static void actuator_update_light(size_t channel)
{
	uint16_t state_id = KNX_DP_ID(channel, SOO);
	enum knx_led_mode mode = KNX_LED_OFF;
	bool value;

	if (knx_datapoint_init_read_pending(state_id)) {
		mode = KNX_LED_BLINK;
	} else if (knx_datapoint_get_bool(state_id, &value) < 0) {
		LOG_ERR("failed to read actuator SOO");
	} else if (value) {
		mode = KNX_LED_ON;
	}

	knx_board_set_app_led_mode((enum knx_board_app_led)channel, mode);
}

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

/* Application parameter required by the KNX virtual LSxB reference ETS product. */
static knx_datapoint_t actuator_parameters[] = {
	{
		.path = "/p/globalTestParameter",
		.dpa = "urn:knx:dpa.65500.201",
		.dpt = ":dpt.value2Ucount",
		.id = KNX_PARAMETER_ID(0),
		.methods = KNX_DP_GET | KNX_DP_PUT,
		.properties = OC_DISCOVERABLE | OC_OBSERVABLE | OC_WRITE_AFFECTS_FP,
		.get_acl = OC_ACL_D,
		.get_iface = OC_IF_D,
		.put_acl = OC_ACL_P,
		.put_iface = OC_IF_P,
		.mirror_to = KNX_DP_NONE,
		.value = {.kind = KNX_DPT_VALUE_2_UCOUNT},
	},
};

/* LSAB: soo is the control input (GET + PUT, if.i) and mirrors onto ioo, the
 * status output (GET, if.o). During typical operation, the actuator listens for
 * s-mode multicasts on its SOO from devices bound to the same GA. When this
 * happens, it sets the indicator led, mirrors the value to IOO and announces
 * it, so the switch can know it worked. SOO is the light state and persists
 * across reboots; IOO is rebuilt from it.
 */
static knx_datapoint_t actuator_datapoints[] = {
	{.path = "/p/lsab/0/soo",
	 .dpa = "urn:knx:dpa.417.52",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(0, SOO),
	 .methods = KNX_DP_GET | KNX_DP_PUT,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_I,
	 .get_iface = OC_IF_I,
	 .put_acl = OC_ACL_I,
	 .put_iface = OC_IF_I,
	 .mirror_to = KNX_DP_ID(0, IOO),
	 .persist = knx_persist_always,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lsab/0/ioo",
	 .dpa = "urn:knx:dpa.417.51",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(0, IOO),
	 .methods = KNX_DP_GET,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_O,
	 .get_iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lsab/1/soo",
	 .dpa = "urn:knx:dpa.417.52",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(1, SOO),
	 .methods = KNX_DP_GET | KNX_DP_PUT,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_I,
	 .get_iface = OC_IF_I,
	 .put_acl = OC_ACL_I,
	 .put_iface = OC_IF_I,
	 .mirror_to = KNX_DP_ID(1, IOO),
	 .persist = knx_persist_always,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lsab/1/ioo",
	 .dpa = "urn:knx:dpa.417.51",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(1, IOO),
	 .methods = KNX_DP_GET,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_O,
	 .get_iface = OC_IF_O,
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

static bool actuator_is_state(const knx_datapoint_t *dp)
{
	return !KNX_DP_IS_PARAMETER(dp->id) && KNX_DP_CHANNEL(dp->id) < NUM_CHANNELS &&
	       KNX_DP_POINT(dp->id) == SOO;
}

/* Runs on the KNX thread after a PUT: mirror the switched state to the LED. */
static void actuator_on_write(const knx_datapoint_t *dp)
{
	bool value;

	if (!actuator_is_state(dp)) {
		return;
	}

	actuator_update_light(KNX_DP_CHANNEL(dp->id));
	if (knx_datapoint_get_bool(dp->id, &value) == 0) {
		LOG_INF("Light turned %s", value ? "on" : "off");
	}
}

static void actuator_on_init_read(const knx_datapoint_t *dp, bool received)
{
	if (!actuator_is_state(dp)) {
		return;
	}

	if (!received) {
		LOG_WRN("No state received for %s, using the stored state", dp->path);
	}

	actuator_update_light(KNX_DP_CHANNEL(dp->id));
}

static void actuator_on_init(void)
{
	for (size_t channel = 0; channel < NUM_CHANNELS; channel++) {
		actuator_update_light(channel);
	}
}

static const knx_device_t actuator_device = {
	.identity = &actuator_identity,
	.parameters = actuator_parameters,
	.num_parameters = ARRAY_SIZE(actuator_parameters),
	.functional_blocks = actuator_blocks,
	.num_functional_blocks = NUM_CHANNELS,
	.on_init = actuator_on_init,
	.on_ready = NULL,
	.on_write = actuator_on_write,
	.on_init_read = actuator_on_init_read,
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
