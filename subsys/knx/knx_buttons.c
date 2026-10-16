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

BUILD_ASSERT(KNX_BUTTONS_COUNT <= 32, "Button state is a 32-bit mask");

enum button_phase {
	BUTTON_IDLE,
	BUTTON_PRESSED, /* held, waiting to arm */
	BUTTON_ARMED,	/* armed, waiting to confirm or cancel */
};

struct button_state {
	uint8_t index;
	enum button_phase phase;
	struct k_work_delayable work;
};

static struct knx_buttons_config buttons_config;
static struct button_state buttons[KNX_BUTTONS_COUNT];

static void button_timeout(struct k_work *work)
{
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct button_state *button = CONTAINER_OF(delayable, struct button_state, work);

	if (button->phase == BUTTON_PRESSED) {
		button->phase = BUTTON_ARMED;
		buttons_config.on_event(button->index, KNX_BUTTON_ARM);
		k_work_reschedule(&button->work, K_MSEC(buttons_config.cancel_window_ms));
	} else if (button->phase == BUTTON_ARMED) {
		button->phase = BUTTON_IDLE;
		buttons_config.on_event(button->index, KNX_BUTTON_CONFIRM);
	}
}

static void button_pressed(struct button_state *button)
{
	if (button->phase != BUTTON_IDLE) {
		return;
	}

	if (buttons_config.on_press(button->index)) {
		button->phase = BUTTON_PRESSED;
		k_work_reschedule(&button->work, K_MSEC(buttons_config.long_press_ms));
	} else {
		buttons_config.on_event(button->index, KNX_BUTTON_SHORT);
	}
}

static void button_released(struct button_state *button)
{
	enum button_phase phase = button->phase;

	if (phase == BUTTON_IDLE) {
		return;
	}

	(void)k_work_cancel_delayable(&button->work);
	button->phase = BUTTON_IDLE;
	buttons_config.on_event(button->index,
				phase == BUTTON_PRESSED ? KNX_BUTTON_SHORT : KNX_BUTTON_CANCEL);
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	for (size_t i = 0; i < KNX_BUTTONS_COUNT; i++) {
		if (!(has_changed & BIT(i))) {
			continue;
		}

		if (button_state & BIT(i)) {
			button_pressed(&buttons[i]);
		} else {
			button_released(&buttons[i]);
		}
	}
}

int knx_buttons_init(const struct knx_buttons_config *config)
{
	int err;

	if (config == NULL || config->on_press == NULL || config->on_event == NULL) {
		return -EINVAL;
	}

	buttons_config = *config;

	for (size_t i = 0; i < KNX_BUTTONS_COUNT; i++) {
		buttons[i].index = i;
		buttons[i].phase = BUTTON_IDLE;
		k_work_init_delayable(&buttons[i].work, button_timeout);
	}

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("dk_buttons_init failed: %d", err);
	}

	return err;
}
