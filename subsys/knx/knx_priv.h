/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Internal declarations shared between the common KNX add-on sources. Not part
 * of the public add-on API (include/knx). */

#ifndef KNX_PRIV_H_
#define KNX_PRIV_H_

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

/*
 * Helpers required by compiled libknx sources (port/zephyr/knx_shell.c,
 * security/oc_spake2plus.c)
 */
char *app_get_password(void);
void app_str_to_upper(char *str);

/* Shared CBOR helper used by the datapoint GET handler. */
void add_all_interface_short_urns_for_a_resource(const oc_resource_t *resource);

#ifdef __cplusplus
}
#endif

#endif /* KNX_PRIV_H_ */
