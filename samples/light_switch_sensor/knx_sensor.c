/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Sensor device profile: KNX functional block 421 (LSSB). Owns everything
 * role-specific and registers it with the generic application layer. The
 * stack, event loop, resource registration and shared GPIO come from the common
 * layer.
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
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "oc_knx_fp.h"

LOG_MODULE_REGISTER(knx_sensor, LOG_LEVEL_INF);

/* Datapoint indices within a switching channel. */
#define SOO	   0 /* switch on/off (control) */
#define IOO	   1 /* info on/off (status) */
#define NUM_POINTS 2

#define NUM_CHANNELS 2

static atomic_t pending_toggles[NUM_CHANNELS];

/*
 * Identity. serialnumber and application_name are non-static because the
 * compiled libknx shell (port/zephyr/knx_shell.c) references them by name.
 */
const char sn_lower_case[] = "00fa10020700";
const char application_name[] = "KNX virtual sensor (LSSB)";

static const knx_identity_t sensor_identity = {
	.serialnumber = sn_lower_case,
	.application_name = application_name,
	.hw_type = "000102030405",
	.dev_model = "6800",
	.mid = 0x00fa,
};

/* Application parameter required by the KNX virtual LSxB reference ETS product. */
static knx_datapoint_t sensor_parameters[] = {
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

/* LSSB: soo is the control output (GET, if.o); ioo is the status input
 * (GET + PUT, if.i). No mirroring. During typical operation, this would behave
 * something like this: The button is pressed, this device sends the inverse of
 * its IOO on SOO as an s-mode multicast and records the sent value in IOO. If
 * there is an actuator bound to the same GA, it receives the message, writes
 * its SOO, updates its output (LED) and announces its IOO status. This switch
 * receives it and updates its IOO, so a light switched elsewhere toggles
 * correctly on the next press. Nothing is persisted: the light state belongs to
 * the actuator, and with the I-flag set on IOO the stack reads it at startup.
 */
static knx_datapoint_t sensor_datapoints[] = {
	{.path = "/p/lssb/0/soo",
	 .dpa = "urn:knx:dpa.421.61",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(0, SOO),
	 .methods = KNX_DP_GET,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_O,
	 .get_iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lssb/0/ioo",
	 .dpa = "urn:knx:dpa.421.53",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(0, IOO),
	 .methods = KNX_DP_GET | KNX_DP_PUT,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_I,
	 .get_iface = OC_IF_I,
	 .put_acl = OC_ACL_I,
	 .put_iface = OC_IF_I,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lssb/1/soo",
	 .dpa = "urn:knx:dpa.421.61",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(1, SOO),
	 .methods = KNX_DP_GET,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_O,
	 .get_iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/lssb/1/ioo",
	 .dpa = "urn:knx:dpa.421.53",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(1, IOO),
	 .methods = KNX_DP_GET | KNX_DP_PUT,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_I,
	 .get_iface = OC_IF_I,
	 .put_acl = OC_ACL_I,
	 .put_iface = OC_IF_I,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
};

static const knx_functional_block_t sensor_blocks[] = {
	{.number = 421,
	 .instance = 1,
	 .datapoints = &sensor_datapoints[0],
	 .num_datapoints = NUM_POINTS},
	{.number = 421,
	 .instance = 2,
	 .datapoints = &sensor_datapoints[NUM_POINTS],
	 .num_datapoints = NUM_POINTS},
};

#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
/* Sensor: individual address 1.1.2, transmits on the shared GA. */
static const knx_preset_t sensor_preset = {
	.fid = KNX_HC_FID,
	.iid = KNX_HC_IID,
	.ga = KNX_HC_GA,
	.grpid = KNX_HC_GRPID,
	.ia = 0x1102,
	.got_href = "/p/lssb/0/soo",
	.got_cflags = OC_CFLAG_TRANSMISSION,
	.is_publisher = false,
	.group_ms = knx_hc_group_ms,
	.group_ms_len = sizeof(knx_hc_group_ms),
	.group_kid = knx_hc_group_kid,
	.group_kid_len = sizeof(knx_hc_group_kid),
};
#endif

/* The channel LED blinks while the status is unknown after startup, then shows
 * the reported light state as a short flash.
 */
static void sensor_update_channel_led(size_t channel)
{
	uint16_t status_id = KNX_DP_ID(channel, IOO);
	enum knx_led_mode mode = KNX_LED_OFF;
	bool value;

	if (knx_datapoint_init_read_pending(status_id)) {
		mode = KNX_LED_BLINK;
	} else if (knx_datapoint_get_bool(status_id, &value) == 0 && value) {
		mode = KNX_LED_FLASH;
	}

	knx_board_set_app_led_mode((enum knx_board_app_led)channel, mode);
}

static void sensor_on_status(const knx_datapoint_t *dp)
{
	if (KNX_DP_CHANNEL(dp->id) >= NUM_CHANNELS || KNX_DP_POINT(dp->id) != IOO) {
		return;
	}

	sensor_update_channel_led(KNX_DP_CHANNEL(dp->id));
}

static void sensor_on_init_read(const knx_datapoint_t *dp, bool received)
{
	if (!received) {
		LOG_WRN("No status received for %s", dp->path);
	}

	sensor_on_status(dp);
}

static void sensor_toggle_channel(size_t channel)
{
	uint16_t toggle_id = KNX_DP_ID(channel, SOO);
	uint16_t status_id = KNX_DP_ID(channel, IOO);
	bool value;

	if (knx_datapoint_get_bool(status_id, &value) < 0) {
		LOG_ERR("button: failed to read sensor IOO");
		return;
	}

	value = !value;

	if (knx_datapoint_set_bool(toggle_id, value) < 0) {
		LOG_ERR("button: failed to set sensor SOO");
		return;
	}

	if (knx_datapoint_set_bool(status_id, value) < 0) {
		LOG_ERR("button: failed to set sensor IOO");
		return;
	}

	sensor_update_channel_led(channel);

	LOG_INF("Switch pressed: sending light %s", value ? "on" : "off");
	knx_datapoint_transmit(toggle_id);
}

static void sensor_process_buttons(void)
{
	for (size_t channel = 0; channel < ARRAY_SIZE(pending_toggles); channel++) {
		atomic_val_t count = atomic_set(&pending_toggles[channel], 0);

		while (count-- > 0) {
			sensor_toggle_channel(channel);
		}
	}
}

static void sensor_on_button(enum knx_board_app_button button)
{
	size_t channel = (size_t)button;

	if (channel >= ARRAY_SIZE(pending_toggles)) {
		LOG_ERR("Invalid application button: %d", button);
		return;
	}

	atomic_inc(&pending_toggles[channel]);
	knx_app_post_work();
}

static void sensor_on_init(void)
{
	for (size_t channel = 0; channel < ARRAY_SIZE(pending_toggles); channel++) {
		atomic_set(&pending_toggles[channel], 0);
		sensor_update_channel_led(channel);
	}
	knx_board_set_app_button_handler(sensor_on_button);
	knx_app_set_work_handler(sensor_process_buttons);
}

static const knx_device_t sensor_device = {
	.identity = &sensor_identity,
	.parameters = sensor_parameters,
	.num_parameters = ARRAY_SIZE(sensor_parameters),
	.functional_blocks = sensor_blocks,
	.num_functional_blocks = NUM_CHANNELS,
	.on_init = sensor_on_init,
	.on_ready = NULL,
	.on_write = sensor_on_status,
	.on_init_read = sensor_on_init_read,
#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
	.preset = &sensor_preset,
#else
	.preset = NULL,
#endif
};

void app_register_device(void)
{
	knx_device_register(&sensor_device);
}
