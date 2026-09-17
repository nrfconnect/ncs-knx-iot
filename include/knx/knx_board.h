/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Board UI mapping for the KNX samples:
 *
 *   LED 0  Thread status             LED 1  KNX status
 *   LED 2  application output 1      LED 3  application output 2
 *   Button 0  Thread actions         Button 1  KNX actions
 *   Button 2  application input 1    Button 3  application input 2
 */

#ifndef KNX_BOARD_H_
#define KNX_BOARD_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application button identifier.
 */
enum knx_board_app_button {
	KNX_BOARD_APP_BUTTON_1 = 0,
	KNX_BOARD_APP_BUTTON_2,
};

/**
 * @brief Application LED identifier.
 */
enum knx_board_app_led {
	KNX_BOARD_APP_LED_1 = 0,
	KNX_BOARD_APP_LED_2,
};

/**
 * @brief Handler invoked on an application button press.
 */
typedef void (*knx_board_app_button_handler_t)(enum knx_board_app_button button);

/**
 * @brief Handler invoked on a system button action.
 */
typedef void (*knx_board_system_button_handler_t)(void);

/** @brief Handlers for board-level Thread and KNX actions. */
struct knx_board_system_handlers {
	knx_board_system_button_handler_t thread_joiner;
	knx_board_system_button_handler_t thread_factory_reset;
	knx_board_system_button_handler_t programming_mode;
	knx_board_system_button_handler_t knx_factory_reset;
};

/**
 * @brief Bring up the DK LEDs and buttons.
 *
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_board_init(void);

/**
 * @brief Re-derive the status LEDs from the current device state.
 *
 * Call after a change to KNX programming mode or commissioning state. Thread
 * network changes are reflected automatically.
 */
void knx_board_update_status(void);

/**
 * @brief Drive an application output LED.
 *
 * @param led Application LED identifier.
 * @param on  true to switch the LED on, false to switch it off.
 */
void knx_board_set_app_led(enum knx_board_app_led led, bool on);

/**
 * @brief Register the handler for the application buttons.
 *
 * Optional.
 */
void knx_board_set_app_button_handler(knx_board_app_button_handler_t handler);

/**
 * @brief Register callbacks for board-level Thread and KNX actions.
 *
 * The handler table is copied. Individual callbacks may be NULL.
 * Pass NULL to clear all registered callbacks.
 *
 * @param handlers Callback table for board-level actions, or NULL to clear
 *                 all callbacks.
 */
void knx_board_set_system_handlers(const struct knx_board_system_handlers *handlers);

#ifdef __cplusplus
}
#endif

#endif /* KNX_BOARD_H_ */
