/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KNX_TRANSPORT_H_
#define KNX_TRANSPORT_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether the underlying network is ready for KNX traffic.
 *
 * @return true if the device can send/receive KNX messages.
 */
bool knx_is_network_connected(void);

/**
 * @brief Block until the underlying network is ready for KNX traffic.
 *
 */
void knx_wait_for_network_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* KNX_TRANSPORT_H_ */
