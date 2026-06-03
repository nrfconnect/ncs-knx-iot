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

#include "oc_knx_fp.h"

LOG_MODULE_REGISTER(knx_sensor, LOG_LEVEL_INF);

/* Datapoint indices within a switching channel. */
#define SOO 0 /* switch on/off (control) */
#define IOO 1 /* info on/off (status) */
#define NUM_POINTS 2

#define NUM_CHANNELS 2

/* The button toggles and transmits channel 0's control datapoint. */
#define TOGGLE_ID KNX_DP_ID(0, SOO)

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

/* LSSB: soo is the control output (GET, if.o); ioo is the status input
 * (GET + PUT, if.i). No mirroring. During typical operation, this would behave
 * something like this: The button is pressed this device's SOO is toggled,
 * s-mode multicast is send. If there is a actuator bound to the same GA, it
 * receives the message, writes its SOO, updates its output (LED) and announces
 * its IOO status. this switch receives it and updates its IOO and now the
 * values at SOO and IOO match.
 */
static knx_datapoint_t sensor_datapoints[] = {
    {.path = "/p/lssb/0/soo",
     .dpa = "urn:knx:dpa.421.61",
     .dpt = ":dpt.switch",
     .id = KNX_DP_ID(0, SOO),
     .methods = OC_GET,
     .acl = OC_ACL_O,
     .iface = OC_IF_O,
     .mirror_to = KNX_DP_NONE,
     .value = {.kind = KNX_DPT_BOOL}},
    {.path = "/p/lssb/0/ioo",
     .dpa = "urn:knx:dpa.421.53",
     .dpt = ":dpt.switch",
     .id = KNX_DP_ID(0, IOO),
     .methods = OC_GET | OC_PUT,
     .acl = OC_ACL_I,
     .iface = OC_IF_I,
     .mirror_to = KNX_DP_NONE,
     .value = {.kind = KNX_DPT_BOOL}},
    {.path = "/p/lssb/1/soo",
     .dpa = "urn:knx:dpa.421.61",
     .dpt = ":dpt.switch",
     .id = KNX_DP_ID(1, SOO),
     .methods = OC_GET,
     .acl = OC_ACL_O,
     .iface = OC_IF_O,
     .mirror_to = KNX_DP_NONE,
     .value = {.kind = KNX_DPT_BOOL}},
    {.path = "/p/lssb/1/ioo",
     .dpa = "urn:knx:dpa.421.53",
     .dpt = ":dpt.switch",
     .id = KNX_DP_ID(1, IOO),
     .methods = OC_GET | OC_PUT,
     .acl = OC_ACL_I,
     .iface = OC_IF_I,
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

/* Runs on the KNX thread: toggle the switch datapoint and transmit it. */
static void sensor_send_toggle(void) {
  bool value;

  if (knx_datapoint_get_bool(TOGGLE_ID, &value) < 0) {
    LOG_ERR("button: failed to read sensor SOO");
    return;
  }

  value = !value;

  if (knx_datapoint_set_bool(TOGGLE_ID, value) < 0) {
    LOG_ERR("button: failed to set sensor SOO");
    return;
  }

  LOG_INF("button: sensor SOO -> %d, transmitting", value);
  knx_datapoint_transmit(TOGGLE_ID);
}

static void sensor_on_button(void) { knx_app_post_work(); }

static void sensor_on_init(void) {
  knx_board_set_button_handler(sensor_on_button);
  knx_app_set_work_handler(sensor_send_toggle);
}

static const knx_device_t sensor_device = {
    .identity = &sensor_identity,
    .functional_blocks = sensor_blocks,
    .num_functional_blocks = NUM_CHANNELS,
    .on_init = sensor_on_init,
    .on_ready = NULL,
    .on_write = NULL,
#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
    .preset = &sensor_preset,
#else
    .preset = NULL,
#endif
};

void app_register_device(void) { knx_device_register(&sensor_device); }
