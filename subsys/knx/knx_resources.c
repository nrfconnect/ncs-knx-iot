/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Device profile registry, generic resource registration and datapoint GET/PUT
 * request handlers. All of this is driven by the registered knx_device_t
 * functional-block/datapoint tables.
 */

#include <knx/knx_device.h>

#include "knx_priv.h"

#include <errno.h>

#include <zephyr/logging/log.h>

#include "api/oc_knx_fp.h"
#include "oc_api.h"
#include "oc_core_res.h"
#include "oc_helpers.h"
#include "oc_knx_client.h"

LOG_MODULE_REGISTER(knx_resources, LOG_LEVEL_INF);

static const knx_device_t *g_device;

void knx_device_register(const knx_device_t *device) { g_device = device; }

const knx_device_t *knx_device_get(void) { return g_device; }

knx_datapoint_t *knx_datapoint_by_id(uint16_t id) {
  if (g_device == NULL) {
    return NULL;
  }

  for (size_t fb_idx = 0; fb_idx < g_device->num_functional_blocks; fb_idx++) {
    const knx_functional_block_t *fb = &g_device->functional_blocks[fb_idx];

    for (size_t dp_idx = 0; dp_idx < fb->num_datapoints; dp_idx++) {
      if (fb->datapoints[dp_idx].id == id) {
        return &fb->datapoints[dp_idx];
      }
    }
  }

  return NULL;
}

int knx_datapoint_get(uint16_t id, knx_datapoint_value_t *value) {
  if (value == NULL) {
    return -EINVAL;
  }

  const knx_datapoint_t *dp = knx_datapoint_by_id(id);

  if (dp == NULL) {
    return -ENOENT;
  }

  if (dp->value.kind != value->kind) {
    return -EINVAL;
  }

  switch (dp->value.kind) {
  case KNX_DPT_BOOL:
    value->data.boolean = dp->value.data.boolean;
    return 0;
  default:
    return -EINVAL;
  }
}

int knx_datapoint_set(uint16_t id, knx_datapoint_value_t value) {
  knx_datapoint_t *dp = knx_datapoint_by_id(id);

  if (dp == NULL) {
    return -ENOENT;
  }

  if (dp->value.kind != value.kind) {
    return -EINVAL;
  }

  switch (dp->value.kind) {
  case KNX_DPT_BOOL:
    dp->value.data.boolean = value.data.boolean;
    return 0;
  default:
    return -EINVAL;
  }
}

int knx_datapoint_get_bool(uint16_t id, bool *value) {
  if (value == NULL) {
    return -EINVAL;
  }

  knx_datapoint_value_t typed_value = {
      .kind = KNX_DPT_BOOL,
  };
  int ret = knx_datapoint_get(id, &typed_value);

  if (ret < 0) {
    return ret;
  }

  *value = typed_value.data.boolean;

  return 0;
}

int knx_datapoint_set_bool(uint16_t id, bool value) {
  return knx_datapoint_set(id, (knx_datapoint_value_t){
                                   .kind = KNX_DPT_BOOL,
                                   .data.boolean = value,
                               });
}

void knx_datapoint_transmit(uint16_t id) {
  const knx_datapoint_t *dp = knx_datapoint_by_id(id);

  if (dp != NULL) {
    oc_send_s_mode_mc_or_uc_message(OC_SENDER_MULTICAST_SCOPE, dp->path, 'w');
  } else {
    LOG_ERR("transmit requested for unknown datapoint id 0x%04x", id);
  }
}

/* Generic GET for any supported datapoint. user_data is the knx_datapoint_t *.
 */
void knx_get_dp(oc_request_t *request, oc_interface_mask_t interfaces,
                void *user_data) {
  (void)interfaces;
  bool error_state = true;

  const knx_datapoint_t *dp = user_data;

  if (dp == NULL) {
    LOG_ERR("GET handler called without datapoint context");
    oc_prepare_no_format_response_no_payload(request, OC_STATUS_NOT_FOUND);
    return;
  }

  LOG_DBG("GET %s", oc_string(request->resource->uri));

  if (!oc_accept_header_is_ok(request, APPLICATION_CBOR)) {
    LOG_ERR("GET %s rejected: unsupported Accept header",
            oc_string(request->resource->uri));
    return;
  }

  if (oc_is_redirected_request_from(request) == 1) {
    LOG_DBG("redirected request %.*s", (int)request->uri_path_len,
            request->uri_path);
  }

  const oc_device_info_t *const device = oc_core_get_device_info();

  oc_rep_begin_root_object();

  if (oc_query_value_exists(request, "m") != -1) {
    char *m_value;
    char *m_key;
    size_t m_value_len = oc_get_query_value(request, "m", &m_value);
    size_t m_key_len;

    const bool wildcard = strncmp(m_value, "*", m_value_len) == 0;

    LOG_DBG("query parameter: %.*s", (int)m_value_len, m_value);

    oc_init_query_iterator();

    while (oc_iterate_query(request, &m_key, &m_key_len, &m_value,
                            &m_value_len) != -1) {
      if (strncmp(m_value, "id", m_value_len) == 0 || wildcard) {
        char serial_number[65];

        (void)snprintf(serial_number, 65, "knx://sn:%s%s",
                       oc_string(device->serialnumber),
                       oc_string(request->resource->uri));
        oc_rep_i_set_text_string(root, 0, serial_number);
        error_state = false;
      }
      if (strncmp(m_value, "value", m_value_len) == 0 || wildcard) {
        switch (dp->value.kind) {
        case KNX_DPT_BOOL:
          oc_rep_text_set_boolean(root, value, dp->value.data.boolean);
          error_state = false;
          break;
        default:
          LOG_ERR("GET %s rejected: unsupported datapoint type %d for value "
                  "metadata",
                  oc_string(request->resource->uri), dp->value.kind);
          error_state = true;
          break;
        }
      }
      if (strncmp(m_value, "rt", m_value_len) == 0 || wildcard) {
        const char *first_type =
            oc_string_array_get_item(request->resource->types, 0);

        oc_rep_text_set_text_string(root, rt, first_type + 7);
        error_state = false;
      }
      if (strncmp(m_value, "if", m_value_len) == 0 || wildcard) {
        add_all_interface_short_urns_for_a_resource(request->resource);
        error_state = false;
      }
      if (strncmp(m_value, "dpt", m_value_len) == 0 || wildcard) {
        oc_rep_text_set_text_string(root, dpt,
                                    oc_string(request->resource->dpt));
        error_state = false;
      }
      if (strncmp(m_value, "ga", m_value_len) == 0 || wildcard) {
        const int index = oc_core_find_first_group_object_table_index_from_href(
            oc_string(request->resource->uri));
        if (index > -1) {
          oc_group_object_table_t *got_table_entry =
              oc_core_get_group_object_table_entry(index);
          if (got_table_entry) {
            oc_rep_set_int_array(root, ga, got_table_entry->ga,
                                 got_table_entry->ga_len);
          }
        }
        error_state = false;
      }
      if (strncmp(m_value, "href", m_value_len) == 0 || wildcard) {
        oc_rep_text_set_text_string(root, href,
                                    oc_string(request->resource->uri));
        error_state = false;
      }
    }
  } else {
    switch (dp->value.kind) {
    case KNX_DPT_BOOL:
      oc_rep_i_set_boolean(root, 1, dp->value.data.boolean);
      error_state = false;
      break;
    default:
      LOG_ERR("GET %s rejected: unsupported datapoint type %d",
              oc_string(request->resource->uri), dp->value.kind);
      error_state = true;
      break;
    }
  }

  oc_rep_end_root_object();

  if (g_err != CborNoError) {
    LOG_ERR("GET %s failed: CBOR encoding error %d",
            oc_string(request->resource->uri), g_err);
    error_state = true;
  }

  LOG_DBG("CBOR encoded payload size %d", oc_rep_get_encoded_payload_size());

  if (error_state) {
    LOG_ERR("GET %s failed: no supported response payload",
            oc_string(request->resource->uri));
    oc_prepare_no_format_response_no_payload(request, OC_STATUS_NOT_FOUND);
  } else {
    oc_prepare_cbor_response(request, OC_STATUS_OK);
  }
}

/* Generic PUT for any supported writable datapoint. user_data is the
 * knx_datapoint_t *. Applies the value, notifies the device, and if the
 * datapoint declares a mirror target copies the value onto it and announces
 * it (e.g. an actuator reflecting control onto its status output).
 */
void knx_put_dp(oc_request_t *request, oc_interface_mask_t interfaces,
                void *user_data) {
  bool error_state = true;
  bool is_input_datapoint = interfaces & OC_IF_I;
  const oc_rep_t *rep = request->request_payload;

  knx_datapoint_t *dp = user_data;

  if (dp == NULL) {
    LOG_ERR("PUT handler called without datapoint context");
    oc_prepare_no_format_response_no_payload(request, OC_STATUS_NOT_FOUND);
    return;
  }

  LOG_DBG("PUT %s", oc_string(request->resource->uri));

  if (oc_is_redirected_request_from(request) == 1) {
    is_input_datapoint = true;
    LOG_DBG("redirected request %.*s", (int)request->uri_path_len,
            request->uri_path);
  }

  while (rep) {
    if (rep->iname == 1) {
      if (!is_input_datapoint) {
        LOG_ERR("PUT %s rejected: datapoint is not writable",
                oc_string(request->resource->uri));
        oc_prepare_no_format_response_no_payload(request,
                                                 OC_STATUS_METHOD_NOT_ALLOWED);
        return;
      }

      knx_datapoint_value_t value = {
          .kind = dp->value.kind,
      };

      switch (dp->value.kind) {
      case KNX_DPT_BOOL:
        if (rep->type != OC_REP_BOOL) {
          LOG_ERR("PUT %s rejected: expected boolean payload, got type %d",
                  oc_string(request->resource->uri), rep->type);
          oc_prepare_no_format_response_no_payload(request,
                                                   OC_STATUS_BAD_REQUEST);
          return;
        }
        value.data.boolean = rep->value.boolean;
        break;
      default:
        LOG_ERR("PUT %s rejected: unsupported datapoint type %d",
                oc_string(request->resource->uri), dp->value.kind);
        oc_prepare_no_format_response_no_payload(request,
                                                 OC_STATUS_BAD_REQUEST);
        return;
      }

      const int ret = knx_datapoint_set(dp->id, value);

      if (ret < 0) {
        LOG_ERR("PUT %s failed: datapoint set returned %d",
                oc_string(request->resource->uri), ret);
        oc_prepare_no_format_response_no_payload(request,
                                                 OC_STATUS_BAD_REQUEST);
        return;
      }

      if (g_device->on_write != NULL) {
        g_device->on_write(dp);
      }

      error_state = false;
      switch (value.kind) {
      case KNX_DPT_BOOL:
        LOG_INF("set %s to %d", oc_string(request->resource->uri),
                value.data.boolean);
        break;
      default:
        break;
      }
      break;
    } else {
      LOG_WRN("PUT %s ignored unsupported CBOR key %d",
              oc_string(request->resource->uri), rep->iname);
    }
    rep = rep->next;
  }

  if (!error_state) {
    if (dp->mirror_to != KNX_DP_NONE) {
      knx_datapoint_t *mirror = knx_datapoint_by_id((uint16_t)dp->mirror_to);

      if (mirror != NULL) {
        knx_datapoint_value_t value = {
            .kind = mirror->value.kind,
        };

        if (knx_datapoint_get(dp->id, &value) == 0 &&
            knx_datapoint_set(mirror->id, value) == 0) {
          LOG_DBG("announce status %s", mirror->path);
          oc_send_s_mode_mc_or_uc_message(OC_SENDER_MULTICAST_SCOPE,
                                          mirror->path, 'w');
        } else {
          LOG_ERR("PUT %s failed: could not mirror value to datapoint id "
                  "0x%04x",
                  oc_string(request->resource->uri), mirror->id);
        }
      } else {
        LOG_ERR("PUT %s failed: mirror target id 0x%04x not found",
                oc_string(request->resource->uri), (uint16_t)dp->mirror_to);
      }
    }

    oc_prepare_no_format_response_no_payload(request, OC_STATUS_CHANGED);
    return;
  }

  LOG_ERR("PUT %s failed: no supported payload value found",
          oc_string(request->resource->uri));
  oc_prepare_no_format_response_no_payload(request, OC_STATUS_BAD_REQUEST);
}

void register_resources(void) {
  if (g_device == NULL) {
    LOG_ERR("register_resources: no device registered");
    return;
  }

  for (size_t fb_idx = 0; fb_idx < g_device->num_functional_blocks; fb_idx++) {
    const knx_functional_block_t *fb = &g_device->functional_blocks[fb_idx];

    for (size_t dp_idx = 0; dp_idx < fb->num_datapoints; dp_idx++) {
      knx_datapoint_t *dp = &fb->datapoints[dp_idx];

      oc_resource_t *resource = oc_new_resource(dp->path, 1);

      oc_resource_bind_resource_type(resource, dp->dpa);
      oc_resource_bind_dpt(resource, dp->dpt);
      oc_resource_bind_content_type(resource, APPLICATION_CBOR, CONTENT_NONE);
      oc_resource_set_functional_block_data(resource, fb->number, fb->instance,
                                            fb->num_datapoints);
      oc_resource_set_properties(resource, OC_DISCOVERABLE + OC_OBSERVABLE);

      if (dp->methods & OC_GET) {
        oc_resource_set_request_handler(resource, OC_GET, knx_get_dp, dp,
                                        dp->acl, dp->iface);
      }
      if (dp->methods & OC_PUT) {
        oc_resource_set_request_handler(resource, OC_PUT, knx_put_dp, dp,
                                        dp->acl, dp->iface);
      }

      oc_add_resource(resource);
    }
  }
}

void app_restart_handler(void *data) {
  (void)data;

  if (g_device == NULL) {
    LOG_ERR("restart handler called before KNX device registration");
    return;
  }

  for (size_t fb_idx = 0; fb_idx < g_device->num_functional_blocks; fb_idx++) {
    const knx_functional_block_t *fb = &g_device->functional_blocks[fb_idx];

    for (size_t dp_idx = 0; dp_idx < fb->num_datapoints; dp_idx++) {
      knx_datapoint_t *dp = &fb->datapoints[dp_idx];

      switch (dp->value.kind) {
      case KNX_DPT_BOOL:
        (void)knx_datapoint_set(dp->id, (knx_datapoint_value_t){
                                            .kind = KNX_DPT_BOOL,
                                            .data.boolean = false,
                                        });
        break;
      default:
        break;
      }
    }
  }
}
