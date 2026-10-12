/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generic board UI engine. Derives the UI state from the Thread and KNX
 * state, renders it on the LEDs, and dispatches button gestures to actions,
 * all according to the layout selected by Kconfig (knx_ui_layout.h).
 */

#include <knx/knx_board.h>
#include <knx/knx_buttons.h>
#include <knx/knx_leds.h>
#include <knx/knx_transport.h>

#include "knx_ui_layout.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "oc_core_res.h"
#include "oc_knx_dev.h"

LOG_MODULE_REGISTER(knx_board, LOG_LEVEL_INF);

#define LONG_PRESS_MS	 3000
#define CANCEL_WINDOW_MS 3000

static knx_board_app_button_handler_t app_button_handler;
static struct knx_board_system_handlers system_handlers;
static atomic_t ui_state = ATOMIC_INIT(KNX_UI_STATE_DOWN);
/* LEDs used by any state indication. */
static uint32_t status_leds;
/* Binding resolved at press time, held until the press completes. */
static const struct knx_ui_binding *pressed_binding[KNX_BUTTONS_COUNT];

static const char *action_name(enum knx_ui_action action)
{
	switch (action) {
	case KNX_UI_ACTION_THREAD_JOIN:
		return "Thread join";
	case KNX_UI_ACTION_THREAD_FACTORY_RESET:
		return "Thread factory reset";
	case KNX_UI_ACTION_KNX_PROGRAMMING_MODE:
		return "KNX programming mode";
	case KNX_UI_ACTION_KNX_FACTORY_RESET:
		return "KNX factory reset";
	case KNX_UI_ACTION_APP_BUTTON_1:
		return "Application button 1";
	case KNX_UI_ACTION_APP_BUTTON_2:
		return "Application button 2";
	default:
		return "None";
	}
}

static knx_board_system_button_handler_t system_handler_get(enum knx_ui_action action)
{
	switch (action) {
	case KNX_UI_ACTION_THREAD_JOIN:
		return system_handlers.thread_joiner;
	case KNX_UI_ACTION_THREAD_FACTORY_RESET:
		return system_handlers.thread_factory_reset;
	case KNX_UI_ACTION_KNX_PROGRAMMING_MODE:
		return system_handlers.programming_mode;
	case KNX_UI_ACTION_KNX_FACTORY_RESET:
		return system_handlers.knx_factory_reset;
	default:
		return NULL;
	}
}

static bool app_button_get(enum knx_ui_action action, enum knx_board_app_button *button)
{
	switch (action) {
	case KNX_UI_ACTION_APP_BUTTON_1:
		*button = KNX_BOARD_APP_BUTTON_1;
		return true;
	case KNX_UI_ACTION_APP_BUTTON_2:
		*button = KNX_BOARD_APP_BUTTON_2;
		return true;
	default:
		return false;
	}
}

static bool action_is_available(enum knx_ui_action action)
{
	enum knx_board_app_button button;

	if (app_button_get(action, &button)) {
		return app_button_handler != NULL;
	}

	return system_handler_get(action) != NULL;
}

static void action_run(enum knx_ui_action action)
{
	enum knx_board_app_button button;
	knx_board_system_button_handler_t handler;

	if (app_button_get(action, &button)) {
		if (app_button_handler != NULL) {
			app_button_handler(button);
		}
		return;
	}

	handler = system_handler_get(action);
	if (handler != NULL) {
		handler();
	}
}

static const struct knx_ui_binding *binding_find(uint8_t button, enum knx_ui_state state)
{
	for (size_t i = 0; i < knx_ui_layout.num_bindings; i++) {
		const struct knx_ui_binding *binding = &knx_ui_layout.bindings[i];

		if (binding->button == button && (binding->states & BIT(state))) {
			return binding;
		}
	}

	return NULL;
}

static bool on_button_press(uint8_t button)
{
	const struct knx_ui_binding *binding =
		binding_find(button, (enum knx_ui_state)atomic_get(&ui_state));

	pressed_binding[button] = binding;

	return binding != NULL && action_is_available(binding->long_action);
}

static void on_button_event(uint8_t button, enum knx_button_event event)
{
	const struct knx_ui_binding *binding = pressed_binding[button];
	const struct knx_ui_indication *armed = &knx_ui_layout.long_press_armed;

	if (binding == NULL) {
		return;
	}

	switch (event) {
	case KNX_BUTTON_SHORT:
		action_run(binding->short_action);
		break;
	case KNX_BUTTON_ARM:
		LOG_INF("%s armed. Release button within %u ms to cancel.",
			action_name(binding->long_action), CANCEL_WINDOW_MS);
		knx_leds_override(armed->leds, armed->mode);
		break;
	case KNX_BUTTON_CANCEL:
		LOG_INF("%s canceled", action_name(binding->long_action));
		knx_leds_clear_override();
		break;
	case KNX_BUTTON_CONFIRM:
		knx_leds_clear_override();
		action_run(binding->long_action);
		break;
	}
}

static enum knx_ui_state ui_state_derive(void)
{
	switch (knx_get_network_state()) {
	case KNX_NETWORK_ATTACHED:
		break;
	case KNX_NETWORK_ATTACHING:
		return KNX_UI_STATE_ATTACHING;
	default:
		return KNX_UI_STATE_DOWN;
	}

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	bool programming = oc_knx_device_in_programming_mode();

	if (oc_core_get_device_info()->ia != 0xFFFF) {
		return programming ? KNX_UI_STATE_COMMISSIONED_PM : KNX_UI_STATE_COMMISSIONED;
	}

	return programming ? KNX_UI_STATE_ATTACHED_PM : KNX_UI_STATE_ATTACHED;
#else
	return KNX_UI_STATE_COMMISSIONED;
#endif
}

static void status_render(enum knx_ui_state state)
{
	const struct knx_ui_indication *indications = knx_ui_layout.indications[state];

	for (uint8_t led = 0; led < KNX_LEDS_COUNT; led++) {
		enum knx_led_mode mode = KNX_LED_OFF;

		if (!(status_leds & BIT(led))) {
			continue;
		}

		for (size_t i = 0; i < KNX_UI_MAX_INDICATIONS; i++) {
			if (indications[i].leds & BIT(led)) {
				mode = indications[i].mode;
				break;
			}
		}

		knx_leds_set(led, mode);
	}
}

void knx_board_update_status(void)
{
	enum knx_ui_state state = ui_state_derive();

	atomic_set(&ui_state, state);
	status_render(state);
}

int knx_board_init(void)
{
	const struct knx_buttons_config buttons_config = {
		.long_press_ms = LONG_PRESS_MS,
		.cancel_window_ms = CANCEL_WINDOW_MS,
		.on_press = on_button_press,
		.on_event = on_button_event,
	};
	int err;

	status_leds = 0U;
	for (size_t state = 0; state < KNX_UI_STATE_COUNT; state++) {
		for (size_t i = 0; i < KNX_UI_MAX_INDICATIONS; i++) {
			status_leds |= knx_ui_layout.indications[state][i].leds;
		}
	}

	err = knx_leds_init();
	if (err) {
		return err;
	}

	err = knx_buttons_init(&buttons_config);
	if (err) {
		LOG_ERR("Failed to initialize buttons: %d", err);
		return err;
	}

	return 0;
}

void knx_board_set_app_led(enum knx_board_app_led led, bool on)
{
	knx_board_set_app_led_mode(led, on ? KNX_LED_ON : KNX_LED_OFF);
}

void knx_board_set_app_led_mode(enum knx_board_app_led led, enum knx_led_mode mode)
{
	if ((size_t)led >= knx_ui_layout.num_app_leds) {
		LOG_DBG("Application LED %d is not available in this layout", led);
		return;
	}

	knx_leds_set(knx_ui_layout.app_leds[led], mode);
}

void knx_board_set_app_button_handler(knx_board_app_button_handler_t handler)
{
	app_button_handler = handler;
}

void knx_board_set_system_handlers(const struct knx_board_system_handlers *handlers)
{
	system_handlers = handlers != NULL ? *handlers : (struct knx_board_system_handlers){0};
}
