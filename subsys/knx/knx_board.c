/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_board.h>
#include <knx/knx_buttons.h>
#include <knx/knx_leds.h>
#include <knx/knx_transport.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "oc_core_res.h"
#include "oc_knx_dev.h"

LOG_MODULE_REGISTER(knx_board, LOG_LEVEL_INF);

#define THREAD_BUTTON_MASK DK_BTN1_MSK
#define KNX_BUTTON_MASK    DK_BTN2_MSK
#define APP_BUTTON_MASK	   (DK_BTN3_MSK | DK_BTN4_MSK)

#define FACTORY_RESET_TRIGGER_MS 3000
#define FACTORY_RESET_CANCEL_MS	 3000

enum board_led {
	LED_THREAD_STATUS,
	LED_KNX_STATUS,
	LED_APP_1,
	LED_APP_2,
};

static knx_board_app_button_handler_t app_button_handler;
static struct knx_board_system_handlers system_handlers;

static enum knx_led_mode net_state_to_mode(enum knx_network_state state)
{
	switch (state) {
	case KNX_NETWORK_ATTACHED:
		return KNX_LED_ON;
	case KNX_NETWORK_ATTACHING:
		return KNX_LED_BLINK;
	default:
		return KNX_LED_OFF;
	}
}

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
static bool knx_is_programmed(void)
{
	return oc_core_get_device_info()->ia != 0xFFFF;
}
#endif

void knx_board_update_status(void)
{
	enum knx_network_state net = knx_get_network_state();

	knx_leds_set(LED_THREAD_STATUS, net_state_to_mode(net));

	/* KNX is reachable only once the network is attached. */
	if (net != KNX_NETWORK_ATTACHED) {
		knx_leds_set(LED_KNX_STATUS, KNX_LED_OFF);
		return;
	}

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	if (oc_knx_device_in_programming_mode()) {
		knx_leds_set(LED_KNX_STATUS, KNX_LED_BLINK);
	} else {
		knx_leds_set(LED_KNX_STATUS, knx_is_programmed() ? KNX_LED_ON : KNX_LED_OFF);
	}
#else
	knx_leds_set(LED_KNX_STATUS, KNX_LED_ON);
#endif
}

static void on_app_button(uint8_t button)
{
	if (app_button_handler == NULL) {
		return;
	}

	if (BIT(button) == DK_BTN3_MSK) {
		app_button_handler(KNX_BOARD_APP_BUTTON_1);
	} else if (BIT(button) == DK_BTN4_MSK) {
		app_button_handler(KNX_BOARD_APP_BUTTON_2);
	}
}

static void on_thread_joiner_request(void)
{
	if (system_handlers.thread_joiner != NULL) {
		system_handlers.thread_joiner();
	}
}

static void on_programming_mode_request(void)
{
	if (system_handlers.programming_mode != NULL) {
		system_handlers.programming_mode();
	}
}

static void on_thread_factory_reset_warning(void)
{
	LOG_INF("Thread factory reset armed. Release button within %u ms to cancel.",
		FACTORY_RESET_CANCEL_MS);
	knx_leds_override(KNX_LED_BLINK);
}

static void on_knx_factory_reset_warning(void)
{
	LOG_INF("KNX factory reset armed. Release button within %u ms to cancel.",
		FACTORY_RESET_CANCEL_MS);
	knx_leds_override(KNX_LED_BLINK);
}

static void on_factory_reset_cancel(void)
{
	LOG_INF("Factory reset canceled");
	knx_leds_clear_override();
}

static void on_thread_factory_reset_execute(void)
{
	knx_leds_clear_override();
	if (system_handlers.thread_factory_reset != NULL) {
		system_handlers.thread_factory_reset();
	}
}

static void on_knx_factory_reset_execute(void)
{
	knx_leds_clear_override();
	if (system_handlers.knx_factory_reset != NULL) {
		system_handlers.knx_factory_reset();
	}
}

static const struct knx_system_button system_buttons[] = {
	{
		.mask = THREAD_BUTTON_MASK,
		.long_press_ms = FACTORY_RESET_TRIGGER_MS,
		.cancel_window_ms = FACTORY_RESET_CANCEL_MS,
		.on_short = on_thread_joiner_request,
		.on_arm = on_thread_factory_reset_warning,
		.on_cancel = on_factory_reset_cancel,
		.on_confirm = on_thread_factory_reset_execute,
	},
	{
		.mask = KNX_BUTTON_MASK,
		.long_press_ms = FACTORY_RESET_TRIGGER_MS,
		.cancel_window_ms = FACTORY_RESET_CANCEL_MS,
		.on_short = on_programming_mode_request,
		.on_arm = on_knx_factory_reset_warning,
		.on_cancel = on_factory_reset_cancel,
		.on_confirm = on_knx_factory_reset_execute,
	},
};

int knx_board_init(void)
{
	int err = knx_leds_init();

	if (err) {
		return err;
	}

	err = knx_buttons_init();
	if (err) {
		return err;
	}

	knx_buttons_set_application_button_callback(APP_BUTTON_MASK, on_app_button);
	err = knx_buttons_set_system_buttons(system_buttons, ARRAY_SIZE(system_buttons));
	if (err) {
		LOG_ERR("Failed to configure function buttons: %d", err);
		return err;
	}

	return 0;
}

void knx_board_set_app_led(enum knx_board_app_led led, bool on)
{
	enum board_led board_led;

	switch (led) {
	case KNX_BOARD_APP_LED_1:
		board_led = LED_APP_1;
		break;
	case KNX_BOARD_APP_LED_2:
		board_led = LED_APP_2;
		break;
	default:
		LOG_ERR("Invalid application LED: %d", led);
		return;
	}

	knx_leds_set(board_led, on ? KNX_LED_ON : KNX_LED_OFF);
}

void knx_board_set_app_button_handler(knx_board_app_button_handler_t handler)
{
	app_button_handler = handler;
}

void knx_board_set_system_handlers(const struct knx_board_system_handlers *handlers)
{
	system_handlers = handlers != NULL ? *handlers : (struct knx_board_system_handlers){0};
}
