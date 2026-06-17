/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KNX_PRESETS_H_
#define KNX_PRESETS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parameters for hardcoded "automatic" commissioning.
 *
 * The shared fabric (fid/iid/ga/grpid and the group key material) must be
 * identical on every device that talks to each other.
 */
struct knx_preset {
  uint64_t fid;
  uint64_t iid;
  uint32_t ga;
  uint32_t grpid;
  int32_t ia;

  /*This defines a single Group Object Table (got) (for simplicity in this
   * hardcoded commissioning), therefore in this hardcoded configuration, only
   * one datapoint can be used)*/
  const char *got_href; /* bound resource, e.g. "/p/lsab/0/soo" */
  uint32_t
      got_cflags; /* oc_cflag_mask_t value (OC_CFLAG_WRITE / _TRANSMISSION) */
  bool is_publisher; /* true: publisher table (receive); false: recipient table
                        (transmit) */

  const uint8_t *group_ms; /* group OSCORE master secret */
  size_t group_ms_len;
  const uint8_t *group_kid; /* group OSCORE sender id / kid */
  size_t group_kid_len;
};

typedef struct knx_preset knx_preset_t;

/**
 * @brief Apply a hardcoded commissioning preset.
 *
 * Configures device identity, load state, group tables and the shared group
 * OSCORE key so two boards can talk without ETS or a commissioner.
 *
 * Only built when CONFIG_KNX_HARDCODED_COMMISSIONING is set. Called by the
 * common layer.
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_apply_presets(const knx_preset_t *preset);

#ifdef __cplusplus
}
#endif

#endif /* KNX_PRESETS_H_ */
