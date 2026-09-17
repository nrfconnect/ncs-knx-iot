/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_leds.h>

#include <dk_buttons_and_leds.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_leds, LOG_LEVEL_INF);

#define BLINK_PERIOD_MS 200

static K_MUTEX_DEFINE(lock);

static enum knx_led_mode led_mode[KNX_LEDS_COUNT];
static enum knx_led_mode override_mode;
static bool override_active;
static bool blink_phase;
static struct k_work_delayable blink_work;

static bool has_blinking_led_locked(void)
{
	if (override_active) {
		return override_mode == KNX_LED_BLINK;
	}

	for (uint8_t led = 0; led < KNX_LEDS_COUNT; led++) {
		if (led_mode[led] == KNX_LED_BLINK) {
			return true;
		}
	}

	return false;
}

static bool render_locked(void)
{
	bool success = true;

	for (uint8_t led = 0; led < KNX_LEDS_COUNT; led++) {
		enum knx_led_mode mode = override_active ? override_mode : led_mode[led];
		bool on = (mode == KNX_LED_BLINK) ? blink_phase : (mode == KNX_LED_ON);
		int err = dk_set_led(led, on);

		if (err) {
			LOG_ERR("Failed to set LED %u: %d", led, err);
			success = false;
		}
	}

	return success;
}

static void update_locked(void)
{
	bool rendered = render_locked();

	if (rendered && has_blinking_led_locked()) {
		k_work_schedule(&blink_work, K_MSEC(BLINK_PERIOD_MS));
	} else {
		k_work_cancel_delayable(&blink_work);
		blink_phase = false;
	}
}

static void blink_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

	if (has_blinking_led_locked()) {
		blink_phase = !blink_phase;
		if (render_locked()) {
			k_work_schedule(&blink_work, K_MSEC(BLINK_PERIOD_MS));
		}
	}

	k_mutex_unlock(&lock);
}

int knx_leds_init(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("dk_leds_init failed: %d", err);
		return err;
	}

	k_work_init_delayable(&blink_work, blink_work_handler);

	return 0;
}

void knx_leds_set(uint8_t led, enum knx_led_mode mode)
{
	if (led >= KNX_LEDS_COUNT || (unsigned int)mode > KNX_LED_BLINK) {
		LOG_ERR("Invalid LED state: led=%u, mode=%d", led, mode);
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);
	led_mode[led] = mode;
	update_locked();
	k_mutex_unlock(&lock);
}

void knx_leds_override(enum knx_led_mode mode)
{
	if ((unsigned int)mode > KNX_LED_BLINK) {
		LOG_ERR("Invalid LED override mode: %d", mode);
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);
	override_active = true;
	override_mode = mode;
	blink_phase = false;
	update_locked();
	k_mutex_unlock(&lock);
}

void knx_leds_clear_override(void)
{
	k_mutex_lock(&lock, K_FOREVER);
	override_active = false;
	update_locked();
	k_mutex_unlock(&lock);
}
