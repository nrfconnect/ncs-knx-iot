/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_board.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_board, LOG_LEVEL_INF);

#define STATUS_LED		   DK_LED1
#define STATUS_LED_BLINK_PERIOD_MS 200
#define APP_LED			   DK_LED2
#define APP_BUTTON_MASK		   DK_BTN2_MSK
/* DK_BTN1 is reserved for the system button (factory reset, future). */

static knx_board_button_handler_t app_button_handler;

static bool status_led_on;

static void status_led_blink(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	status_led_on = !status_led_on;
	dk_set_led(STATUS_LED, status_led_on);
}

static K_TIMER_DEFINE(status_led_timer, status_led_blink, NULL);

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if ((APP_BUTTON_MASK & has_changed) & button_state) {
		if (app_button_handler != NULL) {
			app_button_handler();
		}
	}
}

int knx_board_init(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("dk_leds_init failed: %d", err);
		return err;
	}

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("dk_buttons_init failed: %d", err);
		return err;
	}

	return 0;
}

void knx_board_set_net_state(enum knx_board_net_state state)
{
	switch (state) {
	case KNX_BOARD_NET_DETACHED:
		k_timer_stop(&status_led_timer);
		status_led_on = false;
		dk_set_led(STATUS_LED, false);
		break;
	case KNX_BOARD_NET_JOINING:
		status_led_on = true;
		dk_set_led(STATUS_LED, true);
		k_timer_start(&status_led_timer, K_MSEC(STATUS_LED_BLINK_PERIOD_MS),
			      K_MSEC(STATUS_LED_BLINK_PERIOD_MS));
		break;
	case KNX_BOARD_NET_JOINED:
		k_timer_stop(&status_led_timer);
		status_led_on = true;
		dk_set_led(STATUS_LED, true);
		break;
	}
}

void knx_board_set_app_led(bool on)
{
	dk_set_led(APP_LED, on);
}

void knx_board_set_button_handler(knx_board_button_handler_t handler)
{
	app_button_handler = handler;
}
