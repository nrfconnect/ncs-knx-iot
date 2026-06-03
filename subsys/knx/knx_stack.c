/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Device-agnostic stack lifecycle. Sets the device identity from the registered
 * profile and wires the handler callbacks the stack requires.
 */

#include <knx/knx_device.h>

#include "knx_priv.h"

#include <ctype.h>

#include <zephyr/logging/log.h>

#include "oc_api.h"
#include "oc_core_res.h"
#include "oc_endpoint.h"
#include "port/oc_storage.h"

LOG_MODULE_REGISTER(knx_stack, LOG_LEVEL_INF);

/* Used by the stack's shell layer (knx_shell.c "knx qr" command). */
void app_str_to_upper(char *str) {
  while (*str != '\0') {
    *str = (char)toupper((unsigned char)*str);
    str++;
  }
}

char *app_get_password(void) {
#if defined(CONFIG_KNX_IOT_PASSWORD)
  return CONFIG_KNX_IOT_PASSWORD;
#else
  return "2X4W3TE0DFLLS19Y1FCH";
#endif
}

void add_all_interface_short_urns_for_a_resource(
    const oc_resource_t *resource) {
  oc_interface_mask_t res_interfaces = OC_IF_NONE;
  oc_resource_get_all_interfaces_for_a_resource(resource, &res_interfaces);

  const unsigned int nr_entries =
      oc_count_total_interfaces_in_mask(res_interfaces);
  oc_string_array_t interface_list;
  oc_new_string_array(&interface_list, nr_entries);

  oc_put_all_interface_short_urns_from_a_mask_in_string_array(res_interfaces,
                                                              interface_list);

  oc_rep_set_string_array(root, if, interface_list);

  oc_free_string_array(&interface_list);
}

static void factory_presets_cb(void *data) { (void)data; }

static void restart_presets_cb(void *data) { app_restart_handler(data); }

static void hostname_cb(const oc_string_t host_name, void *data) {
  (void)data;

  LOG_DBG("host name: %s", oc_string(host_name));
}

void initialize_variables(void) {}

int app_init(void) {
  const knx_identity_t *id = knx_device_get()->identity;

  /* Permanent identity. */
  oc_core_set_device(id->serialnumber, id->application_name);
  oc_core_set_device_hwv(0, 0, 1);
  oc_core_set_device_mid(id->mid);
  oc_core_set_device_hwt(id->hw_type);
  oc_core_set_device_model(id->dev_model);

  oc_core_set_device_fwv(0, 0, 1);
  oc_core_set_device_apv(1, 0, 0);

  char hname[HNAME_SIZE];
  (void)snprintf(hname, HNAME_SIZE, HNAME_TYPE, id->serialnumber);
  oc_core_set_device_hostname(hname);

  return 0;
}

int app_initialize_stack(const char *storage_folder_name) {
  (void)storage_folder_name;

  oc_storage_config(NULL);

  initialize_variables();

  static oc_handler_t handler = {.init = app_init,
                                 .signal_event_loop = signal_event_loop,
                                 .register_resources = register_resources,
                                 .requests_entry = NULL};

  oc_set_hostname_cb(hostname_cb, NULL);
  oc_set_factory_presets_cb(factory_presets_cb, NULL);
  oc_set_restart_cb(restart_presets_cb, NULL);

  return oc_main_init(&handler);
}
