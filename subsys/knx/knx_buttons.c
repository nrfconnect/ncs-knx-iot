/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_buttons.h>

#include <dk_buttons_and_leds.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(knx_buttons, LOG_LEVEL_INF);

#define SYSTEM_BUTTON_COUNT 2

static uint32_t app_mask;
static knx_application_button_callback_t app_press_callback;

enum system_button_phase {
	SYSTEM_BUTTON_IDLE,
	SYSTEM_BUTTON_PRESSED, /* held, waiting to arm */
	SYSTEM_BUTTON_ARMED,   /* armed, waiting to confirm or cancel */
};

struct system_button_state {
	const struct knx_system_button *config;
	enum system_button_phase state;
	struct k_work_delayable work;
};

static struct system_button_state system_buttons[SYSTEM_BUTTON_COUNT];

static void system_button_timeout(struct k_work *work);

static void call_phase_callback(knx_system_button_phase_callback_t callback)
{
	if (callback != NULL) {
		callback();
	}
}

static void system_button_timeout(struct k_work *work)
{
	struct k_work_delayable *delayable = CONTAINER_OF(work, struct k_work_delayable, work);
	struct system_button_state *button =
		CONTAINER_OF(delayable, struct system_button_state, work);

	if (button->state == SYSTEM_BUTTON_PRESSED) {
		button->state = SYSTEM_BUTTON_ARMED;
		call_phase_callback(button->config->on_arm);
		k_work_reschedule(&button->work, K_MSEC(button->config->cancel_window_ms));
	} else if (button->state == SYSTEM_BUTTON_ARMED) {
		button->state = SYSTEM_BUTTON_IDLE;
		call_phase_callback(button->config->on_confirm);
	}
}

static void system_button_pressed(struct system_button_state *button)
{
	if (button->state == SYSTEM_BUTTON_IDLE) {
		button->state = SYSTEM_BUTTON_PRESSED;
		k_work_reschedule(&button->work, K_MSEC(button->config->long_press_ms));
	}
}

static void system_button_released(struct system_button_state *button)
{
	if (button->state == SYSTEM_BUTTON_PRESSED) {
		(void)k_work_cancel_delayable(&button->work);
		button->state = SYSTEM_BUTTON_IDLE;
		call_phase_callback(button->config->on_short);
	} else if (button->state == SYSTEM_BUTTON_ARMED) {
		(void)k_work_cancel_delayable(&button->work);
		button->state = SYSTEM_BUTTON_IDLE;
		call_phase_callback(button->config->on_cancel);
	}
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (app_press_callback != NULL) {
		uint32_t pressed = app_mask & has_changed & button_state;

		while (pressed != 0U) {
			uint8_t button = (uint8_t)(find_lsb_set(pressed) - 1);

			app_press_callback(button);
			pressed &= ~BIT(button);
		}
	}

	for (size_t i = 0; i < SYSTEM_BUTTON_COUNT; i++) {
		struct system_button_state *button = &system_buttons[i];

		if (button->config != NULL && (button->config->mask & has_changed)) {
			if (button->config->mask & button_state) {
				system_button_pressed(button);
			} else {
				system_button_released(button);
			}
		}
	}
}

int knx_buttons_init(void)
{
	int err = dk_buttons_init(button_changed);

	if (err) {
		LOG_ERR("dk_buttons_init failed: %d", err);
		return err;
	}

	for (size_t i = 0; i < SYSTEM_BUTTON_COUNT; i++) {
		k_work_init_delayable(&system_buttons[i].work, system_button_timeout);
	}

	return err;
}

void knx_buttons_set_application_button_callback(
	uint32_t mask, knx_application_button_callback_t callback)
{
	app_mask = mask;
	app_press_callback = callback;
}

int knx_buttons_set_system_buttons(const struct knx_system_button *configs, size_t count)
{
	if ((configs == NULL && count != 0U) || count > ARRAY_SIZE(system_buttons)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		if (!is_power_of_two(configs[i].mask) ||
		    (configs[i].mask & DK_ALL_BTNS_MSK) != configs[i].mask) {
			return -EINVAL;
		}

		for (size_t j = 0; j < i; j++) {
			if (configs[i].mask == configs[j].mask) {
				return -EINVAL;
			}
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(system_buttons); i++) {
		system_buttons[i].config = i < count ? &configs[i] : NULL;
		system_buttons[i].state = SYSTEM_BUTTON_IDLE;
	}

	return 0;
}
