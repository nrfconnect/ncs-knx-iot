/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Internal declarations shared between the common KNX add-on sources. Not part
 * of the public add-on API (include/knx). */

#ifndef KNX_PRIV_H_
#define KNX_PRIV_H_

#include <stdbool.h>

#include <knx/knx_device.h>

#include "oc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stack lifecycle (knx_stack.c). */
int knx_stack_init(const char *storage_folder_name);

/* oc_handler signal hook */
void signal_event_loop(void);

/* Resource registration + datapoint request handlers (knx_resources.c). */
void register_resources(void);
void knx_get_dp(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data);
void knx_put_dp(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data);
void knx_restart_handler(void *data);

/* Calls visit for every parameter and functional block datapoint of the
 * registered device. The device must be registered.
 */
void knx_datapoint_for_each(void (*visit)(knx_datapoint_t *dp));

/* Datapoint persistence.
 * Load runs once after stack init and reads saved values from flash.
 * Factory reset erases stored values and restores defaults.
 */
void knx_datapoints_load(void);
void knx_datapoints_factory_reset(void);

/* Handles reading values of datapoints from the network (I flag, read on init)
 * Begin (before knx_board_init) marks all the dp and starts the process, called before the network
 * is attached. Start (after attached to a network) does the fetching and potential retries after
 * the network is found. Finish stops waiting for a datapoint (knx_init_read.c).
 */
void knx_init_read_begin(void);
void knx_init_read_start(void);
void knx_init_read_cancel(void);
void knx_init_read_finish(knx_datapoint_t *dp, bool received);

/* Helper required by compiled libknx sources (port/zephyr/knx_shell.c). */
const char *app_get_password(void);

/* Shared CBOR helper used by the datapoint GET handler. */
void add_all_interface_short_urns_for_a_resource(const oc_resource_t *resource);

#ifdef __cplusplus
}
#endif

#endif /* KNX_PRIV_H_ */
