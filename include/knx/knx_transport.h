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
 * @brief Underlying network attachment state.
 */
enum knx_network_state {
	KNX_NETWORK_DOWN,      /**< Stack disabled or stopped. */
	KNX_NETWORK_ATTACHING, /**< Enabled, not yet attached. */
	KNX_NETWORK_ATTACHED,  /**< Attached; KNX traffic possible. */
};

/**
 * @brief Get the current network attachment state.
 */
enum knx_network_state knx_get_network_state(void);

/**
 * @brief Handler invoked when the network attachment state changes.
 */
typedef void (*knx_network_state_handler_t)(enum knx_network_state state);

/**
 * @brief Register a handler notified on network attachment changes.
 *
 * The handler is invoked once with the current state. Pass NULL to
 * unregister.
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_set_network_state_handler(knx_network_state_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* KNX_TRANSPORT_H_ */
