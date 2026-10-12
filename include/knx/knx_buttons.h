/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Button gesture detection: momentary presses and confirmed long presses.
 *
 * Button indices follow the DK Buttons and LEDs library, which numbers the
 * children of the devicetree /buttons node in order. All callbacks run on the
 * system work queue.
 */

#ifndef KNX_BUTTONS_H_
#define KNX_BUTTONS_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of buttons handled by the engine. */
#define KNX_BUTTONS_COUNT DT_CHILD_NUM(DT_PATH(buttons))

/**
 * @brief Gesture reported for a button press.
 *
 * For a press that tracks a long press:
 *   released before long_press_ms              -> KNX_BUTTON_SHORT
 *   held for long_press_ms                     -> KNX_BUTTON_ARM
 *   released within cancel_window_ms of arming -> KNX_BUTTON_CANCEL
 *   held through cancel_window_ms              -> KNX_BUTTON_CONFIRM
 *
 * For a press that does not track a long press, KNX_BUTTON_SHORT is reported
 * immediately on press.
 */
enum knx_button_event {
	KNX_BUTTON_SHORT,
	KNX_BUTTON_ARM,
	KNX_BUTTON_CANCEL,
	KNX_BUTTON_CONFIRM,
};

/**
 * @brief Called when a button is pressed.
 *
 * @param button DK button index.
 *
 * @return true to track a long press for this press, false to report
 *         KNX_BUTTON_SHORT immediately.
 */
typedef bool (*knx_button_press_cb_t)(uint8_t button);

/**
 * @brief Called for each gesture event of a press.
 *
 * @param button DK button index.
 * @param event  Gesture event.
 */
typedef void (*knx_button_event_cb_t)(uint8_t button, enum knx_button_event event);

/** @brief Button gesture configuration. */
struct knx_buttons_config {
	uint32_t long_press_ms;	   /**< Hold time before arming. */
	uint32_t cancel_window_ms; /**< Grace period after arming before confirm. */
	knx_button_press_cb_t on_press;
	knx_button_event_cb_t on_event;
};

/**
 * @brief Bring up the button hardware and start gesture detection.
 *
 * @param config Gesture configuration. Copied; both callbacks are required.
 *
 * @return 0 on success, -EINVAL on invalid configuration, or negative errno
 *         from the button driver.
 */
int knx_buttons_init(const struct knx_buttons_config *config);

#ifdef __cplusplus
}
#endif

#endif /* KNX_BUTTONS_H_ */
