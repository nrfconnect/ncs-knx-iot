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
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(knx_leds, LOG_LEVEL_INF);

BUILD_ASSERT(KNX_LEDS_COUNT < 32, "LED masks are 32 bits wide");

#define BLINK_PERIOD_MS 200
#define FLASH_ON_MS	50
#define FLASH_PERIOD_MS 1000

/* Timed on/off pattern shared by every LED displaying the same mode. */
struct knx_leds_animation {
	enum knx_led_mode mode;
	uint32_t on_ms;
	uint32_t off_ms;
	bool active;
	bool phase;
	struct k_work_delayable work;
};

static K_MUTEX_DEFINE(lock);

static enum knx_led_mode led_mode[KNX_LEDS_COUNT];
static enum knx_led_mode override_mode;
static uint32_t override_leds;
static bool override_active;

static struct knx_leds_animation animations[] = {
	{.mode = KNX_LED_BLINK, .on_ms = BLINK_PERIOD_MS, .off_ms = BLINK_PERIOD_MS},
	{.mode = KNX_LED_FLASH, .on_ms = FLASH_ON_MS, .off_ms = FLASH_PERIOD_MS - FLASH_ON_MS},
};

static bool mode_is_valid(enum knx_led_mode mode)
{
	return (unsigned int)mode < KNX_LED_MODE_COUNT;
}

static struct knx_leds_animation *animation_get(enum knx_led_mode mode)
{
	for (size_t i = 0; i < ARRAY_SIZE(animations); i++) {
		if (animations[i].mode == mode) {
			return &animations[i];
		}
	}

	return NULL;
}

static enum knx_led_mode effective_mode_locked(uint8_t led)
{
	if (override_active) {
		return (override_leds & BIT(led)) ? override_mode : KNX_LED_OFF;
	}

	return led_mode[led];
}

static bool mode_in_use_locked(enum knx_led_mode mode)
{
	for (size_t led = 0; led < KNX_LEDS_COUNT; led++) {
		if (effective_mode_locked(led) == mode) {
			return true;
		}
	}

	return false;
}

static bool led_is_lit_locked(uint8_t led)
{
	enum knx_led_mode mode = effective_mode_locked(led);
	const struct knx_leds_animation *animation;

	switch (mode) {
	case KNX_LED_OFF:
		return false;
	case KNX_LED_ON:
		return true;
	default:
		animation = animation_get(mode);
		return animation != NULL && animation->phase;
	}
}

static bool render_locked(void)
{
	bool success = true;

	for (size_t led = 0; led < KNX_LEDS_COUNT; led++) {
		int err = dk_set_led(led, led_is_lit_locked(led));

		if (err) {
			LOG_ERR("Failed to set LED %zu: %d", led, err);
			success = false;
		}
	}

	return success;
}

static void animation_stop_locked(struct knx_leds_animation *animation)
{
	animation->active = false;
	animation->phase = false;
	(void)k_work_cancel_delayable(&animation->work);
}

static void update_locked(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(animations); i++) {
		struct knx_leds_animation *animation = &animations[i];

		if (!mode_in_use_locked(animation->mode)) {
			animation_stop_locked(animation);
		} else if (!animation->active) {
			/* Start with the LED lit for immediate feedback. */
			animation->active = true;
			animation->phase = true;
			k_work_reschedule(&animation->work, K_MSEC(animation->on_ms));
		}
	}

	(void)render_locked();
}

static void animation_work_handler(struct k_work *work)
{
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct knx_leds_animation *animation =
		CONTAINER_OF(delayable, struct knx_leds_animation, work);

	k_mutex_lock(&lock, K_FOREVER);

	if (animation->active) {
		animation->phase = !animation->phase;
		if (render_locked()) {
			k_work_reschedule(&animation->work,
					  K_MSEC(animation->phase ? animation->on_ms
								  : animation->off_ms));
		} else {
			animation->active = false;
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

	for (size_t i = 0; i < ARRAY_SIZE(animations); i++) {
		k_work_init_delayable(&animations[i].work, animation_work_handler);
	}

	return 0;
}

void knx_leds_set(uint8_t led, enum knx_led_mode mode)
{
	if (led >= KNX_LEDS_COUNT || !mode_is_valid(mode)) {
		LOG_ERR("Invalid LED state: led=%u, mode=%d", led, mode);
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);
	led_mode[led] = mode;
	update_locked();
	k_mutex_unlock(&lock);
}

void knx_leds_override(uint32_t leds, enum knx_led_mode mode)
{
	if ((leds & ~BIT_MASK(KNX_LEDS_COUNT)) != 0U || !mode_is_valid(mode)) {
		LOG_ERR("Invalid LED override: leds=0x%x, mode=%d", leds, mode);
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);
	override_active = true;
	override_leds = leds;
	override_mode = mode;
	/* Restart the patterns so the override starts in a known phase. */
	for (size_t i = 0; i < ARRAY_SIZE(animations); i++) {
		animation_stop_locked(&animations[i]);
	}
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
