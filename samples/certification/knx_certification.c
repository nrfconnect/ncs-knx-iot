/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Certification device profile: one KNX functional block 417 (LSAB) and one
 * functional block 421 (LSSB) with the identity, resource paths, datapoint
 * types and parameter expected by the KNX IoT EITT certification test
 * template.
 */

#include "app.h"

#include <knx/knx_app.h>
#include <knx/knx_board.h>
#include <knx/knx_device.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(knx_certification, LOG_LEVEL_INF);

/* Functional-block channels. */
#define CHANNEL_LSAB 0
#define CHANNEL_LSSB 1

/* Datapoint indices within a switching channel. */
#define SOO	   0 /* switch on/off */
#define IOO	   1 /* info on/off */
#define NUM_POINTS 2

static atomic_t pending_toggles;

/*
 * Identity, fixed by the EITT test template. serialnumber and
 * application_name are non-static because the compiled libknx shell
 * (port/zephyr/knx_shell.c) references them by name.
 */
const char sn_lower_case[] = CONFIG_KNX_SERIAL_NUMBER;
const char application_name[] = "KNX IoT EITT certification device";

static const knx_identity_t certification_identity = {
	.serialnumber = sn_lower_case,
	.application_name = application_name,
	.hw_type = "Windows",
	.dev_model = "KNX Certification",
	.mid = 667,
};

/* Global test parameter of the EITT test template.
 */
static knx_datapoint_t certification_parameters[] = {
	{
		.path = "/p/p1",
		.dpa = "urn:knx:dpa.65500.201",
		.dpt = ":dpt.propDataType",
		.id = KNX_PARAMETER_ID(0),
		.methods = KNX_DP_GET | KNX_DP_PUT,
		.properties = OC_DISCOVERABLE | OC_WRITE_AFFECTS_FP,
		.get_acl = OC_ACL_D,
		.get_iface = OC_IF_D,
		.put_acl = OC_ACL_P,
		.put_iface = OC_IF_P,
		.mirror_to = KNX_DP_NONE,
		.value = {.kind = KNX_DPT_VALUE_4_COUNT},
	},
};

/* LSAB: soo is the control input (GET + PUT, if.i) and mirrors onto ioo, the
 * status output (GET, if.o).
 * LSSB: soo is the control output (GET, if.o); ioo is the status input
 * (GET + PUT, if.i).
 * The datapoint addresses (.61/.62) are the EITT test template values.
 */
static knx_datapoint_t certification_datapoints[] = {
	{.path = "/p/1",
	 .dpa = "urn:knx:dpa.417.61",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(CHANNEL_LSAB, SOO),
	 .methods = KNX_DP_GET | KNX_DP_PUT,
	 .properties = OC_DISCOVERABLE,
	 .get_acl = OC_ACL_I,
	 .get_iface = OC_IF_I,
	 .put_acl = OC_ACL_I,
	 .put_iface = OC_IF_I,
	 .mirror_to = KNX_DP_ID(CHANNEL_LSAB, IOO),
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/2",
	 .dpa = "urn:knx:dpa.417.62",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(CHANNEL_LSAB, IOO),
	 .methods = KNX_DP_GET,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_O,
	 .get_iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/3",
	 .dpa = "urn:knx:dpa.421.61",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(CHANNEL_LSSB, SOO),
	 .methods = KNX_DP_GET,
	 .properties = OC_DISCOVERABLE | OC_OBSERVABLE,
	 .get_acl = OC_ACL_O,
	 .get_iface = OC_IF_O,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
	{.path = "/p/4",
	 .dpa = "urn:knx:dpa.421.62",
	 .dpt = ":dpt.switch",
	 .id = KNX_DP_ID(CHANNEL_LSSB, IOO),
	 .methods = KNX_DP_GET | KNX_DP_PUT,
	 .properties = OC_DISCOVERABLE,
	 .get_acl = OC_ACL_I,
	 .get_iface = OC_IF_I,
	 .put_acl = OC_ACL_I,
	 .put_iface = OC_IF_I,
	 .mirror_to = KNX_DP_NONE,
	 .value = {.kind = KNX_DPT_BOOL}},
};

/* The EITT test template expects instance 1 for both functional blocks. */
static const knx_functional_block_t certification_blocks[] = {
	{.number = 417,
	 .instance = 1,
	 .datapoints = &certification_datapoints[CHANNEL_LSAB * NUM_POINTS],
	 .num_datapoints = NUM_POINTS},
	{.number = 421,
	 .instance = 1,
	 .datapoints = &certification_datapoints[CHANNEL_LSSB * NUM_POINTS],
	 .num_datapoints = NUM_POINTS},
};

/* Application LED 1 shows the LSAB switch state, application LED 2 the
 * reported LSSB status.
 */
static void certification_update_led(uint16_t id, enum knx_board_app_led led)
{
	bool value;

	if (knx_datapoint_get_bool(id, &value) < 0) {
		LOG_ERR("failed to read datapoint 0x%04x", id);
		return;
	}

	knx_board_set_app_led(led, value);
}

static void certification_update_leds(void)
{
	certification_update_led(KNX_DP_ID(CHANNEL_LSAB, SOO), KNX_BOARD_APP_LED_1);
	certification_update_led(KNX_DP_ID(CHANNEL_LSSB, IOO), KNX_BOARD_APP_LED_2);
}

/* Runs on the KNX thread after a PUT or a KNX factory reset. */
static void certification_on_write(const knx_datapoint_t *dp)
{
	if (dp->id == KNX_DP_ID(CHANNEL_LSAB, SOO) || dp->id == KNX_DP_ID(CHANNEL_LSSB, IOO)) {
		certification_update_leds();
	}
}

static void certification_on_init_read(const knx_datapoint_t *dp, bool received)
{
	if (!received) {
		LOG_WRN("No value received for %s", dp->path);
	}

	certification_on_write(dp);
}

/* Sends the inverse of the LSSB status on the LSSB switch output. */
static void certification_toggle_lssb(void)
{
	uint16_t toggle_id = KNX_DP_ID(CHANNEL_LSSB, SOO);
	uint16_t status_id = KNX_DP_ID(CHANNEL_LSSB, IOO);
	bool value;

	if (knx_datapoint_get_bool(status_id, &value) < 0) {
		LOG_ERR("button: failed to read LSSB IOO");
		return;
	}

	value = !value;

	if (knx_datapoint_set_bool(toggle_id, value) < 0) {
		LOG_ERR("button: failed to set LSSB SOO");
		return;
	}

	if (knx_datapoint_set_bool(status_id, value) < 0) {
		LOG_ERR("button: failed to set LSSB IOO");
		return;
	}

	certification_update_leds();

	LOG_INF("Switch pressed: sending %s", value ? "on" : "off");
	knx_datapoint_transmit(toggle_id);
}

static void certification_process_buttons(void)
{
	atomic_val_t count = atomic_set(&pending_toggles, 0);

	while (count-- > 0) {
		certification_toggle_lssb();
	}
}

static void certification_on_button(enum knx_board_app_button button)
{
	if (button != KNX_BOARD_APP_BUTTON_1) {
		return;
	}

	atomic_inc(&pending_toggles);
	knx_app_post_work();
}

static void certification_on_init(void)
{
	atomic_set(&pending_toggles, 0);
	certification_update_leds();
	knx_board_set_app_button_handler(certification_on_button);
	knx_app_set_work_handler(certification_process_buttons);
}

static const knx_device_t certification_device = {
	.identity = &certification_identity,
	.parameters = certification_parameters,
	.num_parameters = ARRAY_SIZE(certification_parameters),
	.functional_blocks = certification_blocks,
	.num_functional_blocks = ARRAY_SIZE(certification_blocks),
	.on_init = certification_on_init,
	.on_ready = NULL,
	.on_write = certification_on_write,
	.on_init_read = certification_on_init_read,
	.preset = NULL,
};

void app_register_device(void)
{
	knx_device_register(&certification_device);
}
