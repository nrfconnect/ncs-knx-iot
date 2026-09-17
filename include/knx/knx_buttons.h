/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Button support for momentary actions and confirmed long-press actions.
 */

#ifndef KNX_BUTTONS_H_
#define KNX_BUTTONS_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handler invoked when an application button is pressed.
 *
 * @param button 0-based DK button index that was pressed.
 */
typedef void (*knx_application_button_callback_t)(uint8_t button);

/** @brief Callback invoked for a system-button phase. */
typedef void (*knx_system_button_phase_callback_t)(void);

/**
 * @brief System-button behavior.
 *
 *  Lifecycle of a single press of @ref mask:
 *   released before @ref long_press_ms          -> on_short
 *   held for @ref long_press_ms                 -> on_arm (warning window opens)
 *   released within @ref cancel_window_ms       -> on_cancel
 *   held through @ref cancel_window_ms          -> on_confirm
 *
 * Any callback may be NULL.
 */
struct knx_system_button {
	uint32_t mask;		       /**< DK button mask (a single button). */
	uint32_t long_press_ms;	   /**< Hold time before arming. */
	uint32_t cancel_window_ms; /**< Grace period after arming before confirm. */
	knx_system_button_phase_callback_t on_short;
	knx_system_button_phase_callback_t on_arm;
	knx_system_button_phase_callback_t on_cancel;
	knx_system_button_phase_callback_t on_confirm;
};

/**
 * @brief Bring up the button hardware.
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_buttons_init(void);

/**
 * @brief Register a callback for presses of one or more application buttons.
 *
 * @param mask    DK button mask covered by the callback.
 * @param callback Invoked on press for each matching button.
 */
void knx_buttons_set_application_button_callback(
	uint32_t mask, knx_application_button_callback_t callback);

/**
 * @brief Install one or more system-button behaviors.
 *
 * At most two configurations are supported, and each mask must identify a
 * different button. The configurations are referenced, not copied, and must
 * stay valid. Pass NULL with a count of zero to clear them.
 *
 * @return 0 on success, or -EINVAL if the configuration is invalid.
 */
int knx_buttons_set_system_buttons(const struct knx_system_button *configs, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* KNX_BUTTONS_H_ */
