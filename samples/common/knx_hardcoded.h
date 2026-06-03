/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Shared "automatic" commissioning fabric for the light_switch samples.
 *
 * These values describe the single group the sensor and actuator share, plus
 * the group OSCORE key material. They MUST be identical on every board that
 * talks to each other.
 */

#ifndef KNX_HARDCODED_H_
#define KNX_HARDCODED_H_

#include <stdint.h>

/* Shared fabric (identical on actuator + sensor). */
#define KNX_HC_FID 1001ULL
#define KNX_HC_IID 1ULL
#define KNX_HC_GA 2305U /* ETS group address 1/1/1 */
#define KNX_HC_GRPID 0x80000001U

/* Shared group OSCORE master secret (both boards must match). */
static const uint8_t knx_hc_group_ms[] = {
    0xff, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

/* OSCORE Sender ID / kid for GA 2305 (0x0901). */
static const uint8_t knx_hc_group_kid[] = {0x09, 0x01};

#endif /* KNX_HARDCODED_H_ */
