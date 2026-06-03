/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Hardcoded "automatic" commissioning mechanism. Writes the identity, load
 * state, group object / publisher / recipient tables and a shared group OSCORE
 * key directly into the stack so two boards can exchange s-mode group messages
 * with no ETS or Thread commissioner.
 */

#include <knx/knx_presets.h>

#include "oc_core_res.h"
#include "oc_helpers.h"
#include "oc_knx.h"
#include "oc_knx_dev.h"
#include "oc_knx_fp.h"
#include "oc_knx_sec.h"
#include "oc_storage.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_presets, LOG_LEVEL_INF);

/* The stack owns (and frees) the ga arrays, so each must be its own heap slot.
 */
static uint32_t *dup_ga(uint32_t ga) {
  uint32_t *slot = malloc(sizeof(uint32_t));

  if (slot != NULL) {
    *slot = ga;
  }

  return slot;
}

static int setup_hardcoded_group_table(oc_group_table_t *entry, uint32_t ga,
                                       uint32_t grpid) {
  if (entry == NULL) {
    return -EINVAL;
  }

  if (entry->id >= 0) {
    oc_free_string(&entry->at);
    free(entry->ga);
  }

  entry->id = 0;
  entry->ia = -1;
  entry->iid = -1;
  entry->fid = -1;
  entry->grpid = grpid;
  entry->non = true;
  entry->ga_len = 1;
  entry->ga = dup_ga(ga);
  if (entry->ga == NULL) {
    return -ENOMEM;
  }

  oc_new_string(&entry->at, "", 0);

  return 0;
}

static int setup_hardcoded_got(uint32_t ga, const char *href,
                               oc_cflag_mask_t cflags) {
  oc_group_object_table_t *got = oc_core_get_group_object_table_entry(0);

  if (got == NULL) {
    return -EINVAL;
  }

  if (got->id >= 0) {
    oc_free_string(&got->href);
    free(got->ga);
  }

  got->id = 0;
  got->cflags = cflags;
  got->ga_len = 1;
  got->ga = dup_ga(ga);
  if (got->ga == NULL) {
    return -ENOMEM;
  }

  oc_new_string(&got->href, href, strlen(href));
  oc_store_group_object_table_entry(0);

  return 0;
}

static int setup_hardcoded_access_token(const knx_preset_t *preset) {
  const int idx = oc_core_find_at_entry_empty_slot();

  if (idx < 0) {
    return -ENOMEM;
  }

  oc_auth_at_t *at = oc_get_auth_at_entry(idx);

  if (at == NULL) {
    return -EINVAL;
  }

  oc_new_string(&at->id, "hardcoded-at-0", 14);
  at->profile = OC_PROFILE_COAP_OSCORE;
  at->scope = OC_ACL_GA;
  at->ga_len = 1;
  at->ga = dup_ga(preset->ga);
  if (at->ga == NULL) {
    return -ENOMEM;
  }

  oc_new_byte_string(&at->osc_id, (char *)preset->group_kid,
                     preset->group_kid_len);
  oc_new_byte_string(&at->osc_ms, (char *)preset->group_ms,
                     preset->group_ms_len);
  oc_new_byte_string(&at->osc_salt, "", 0);
  oc_new_byte_string(&at->osc_contextid, "", 0);

  return 0;
}

int knx_apply_presets(const knx_preset_t *preset) {
  bool pm = false;
  int ret;

  if (preset == NULL) {
    return -EINVAL;
  }

  LOG_INF("applying hardcoded KNX presets (fid=%llu iid=%llu ia=%d ga=%u)",
          (unsigned long long)preset->fid, (unsigned long long)preset->iid,
          preset->ia, preset->ga);

  oc_core_set_and_store_device_fid(preset->fid);
  oc_core_set_and_store_device_iid(preset->iid);
  oc_core_set_and_store_device_ia(preset->ia);
  oc_knx_set_and_store_lsm(LSM_S_LOADED);

  oc_device_info_t *device = oc_core_get_device_info();

  device->pm = pm;
  oc_storage_write(KNX_STORAGE_PM, (uint8_t *)&pm, sizeof(pm));

  ret = setup_hardcoded_got(preset->ga, preset->got_href,
                            (oc_cflag_mask_t)preset->got_cflags);
  if (ret < 0) {
    LOG_ERR("failed to configure group object table: %d", ret);
    return ret;
  }

  if (preset->is_publisher) {
    ret = setup_hardcoded_group_table(oc_core_get_publisher_table_entry(0),
                                      preset->ga, preset->grpid);
    if (ret < 0) {
      LOG_ERR("failed to configure publisher table: %d", ret);
      return ret;
    }
  } else {
    ret = setup_hardcoded_group_table(
        oc_core_get_recipient_table_entry(0), preset->ga, preset->grpid);
    if (ret < 0) {
      LOG_ERR("failed to configure recipient table: %d", ret);
      return ret;
    }
  }

  ret = setup_hardcoded_access_token(preset);
  if (ret < 0) {
    LOG_ERR("failed to configure group OSCORE access token: %d", ret);
    return ret;
  }

  oc_init_oscore_from_storage(false);

  if (preset->is_publisher) {
    /* Re-register: the PUB table was empty during oc_main_init(). */
    oc_register_group_multicasts();
  }

  LOG_INF("hardcoded KNX presets applied");

  return 0;
}
