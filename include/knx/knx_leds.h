/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Indicator LED support for the KNX add-on. Each logical LED can be off, on,
 * or blinking. A temporary global override is used for factory-reset feedback.
 *
 * The runtime LED APIs are thread-safe but may sleep, so they must not be
 * called from an ISR. Call knx_leds_init() once before using the other APIs.
 */

#ifndef KNX_LEDS_H_
#define KNX_LEDS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of logical LEDs managed by the engine. */
#define KNX_LEDS_COUNT 4

/** @brief Display mode of a single LED. */
enum knx_led_mode {
	KNX_LED_OFF,
	KNX_LED_ON,
	KNX_LED_BLINK,
};

/**
 * @brief Bring up the LED hardware.
 *
 * Call once before using the other LED APIs.
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_leds_init(void);

/**
 * @brief Set the mode of a single LED.
 *
 * @param led  Logical LED index (0 .. KNX_LEDS_COUNT - 1).
 * @param mode Mode to display.
 */
void knx_leds_set(uint8_t led, enum knx_led_mode mode);

/**
 * @brief Force every LED to one mode, ignoring per-LED state.
 *
 * Per-LED modes are remembered and restored by @ref knx_leds_clear_override.
 *
 * @param mode Mode applied to all LEDs.
 */
void knx_leds_override(enum knx_led_mode mode);

/**
 * @brief Drop the override and restore the per-LED modes.
 */
void knx_leds_clear_override(void);

#ifdef __cplusplus
}
#endif

#endif /* KNX_LEDS_H_ */
