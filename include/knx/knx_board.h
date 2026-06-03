/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Shared board support for the KNX add-on. Owns the parts that are common to
 * every device: bringing up the DK LED/button subsystem, the network status
 * LED, and (in the future) the system button for factory reset.
 *
 * Backed by the Nordic dk_buttons_and_leds library; compiled only when
 * CONFIG_DK_LIBRARY is enabled.
 */

#ifndef KNX_BOARD_H_
#define KNX_BOARD_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network status reflected on the board status LED.
 *
 * The status LED is driven automatically by @ref knx_board_set_net_state:
 * - @ref KNX_BOARD_NET_DETACHED : LED off
 * - @ref KNX_BOARD_NET_JOINING  : LED blinking
 * - @ref KNX_BOARD_NET_JOINED   : LED on
 */
enum knx_board_net_state {
	KNX_BOARD_NET_DETACHED,
	KNX_BOARD_NET_JOINING,
	KNX_BOARD_NET_JOINED,
};

/**
 * @brief Handler invoked on an application button press.
 *
 */
typedef void (*knx_board_button_handler_t)(void);

/**
 * @brief Bring up the DK LEDs and buttons.
 *
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_board_init(void);

/**
 * @brief Set the network status and update the status LED accordingly.
 *
 * @param state Network state to reflect on the status LED.
 */
void knx_board_set_net_state(enum knx_board_net_state state);

/**
 * @brief Drive the application output LED.
 *
 * The board owns the pin mapping; the application only expresses on/off.
 *
 * @param on true to switch the LED on, false to switch it off.
 */
void knx_board_set_app_led(bool on);

/**
 * @brief Register the handler for the application button.
 *
 * Optional.
 */
void knx_board_set_button_handler(knx_board_button_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* KNX_BOARD_H_ */
