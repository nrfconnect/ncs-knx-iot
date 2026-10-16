/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Development kit layout:
 *
 *   LED 0  Thread status             LED 1  KNX status
 *   LED 2  application output 1      LED 3  application output 2
 *   Button 0  Thread actions         Button 1  KNX actions
 *   Button 2  application input 1    Button 3  application input 2
 *
 * Button 1 is active only with CONFIG_KNX_ETS_COMMISSIONING.
 */

#include "knx_ui_layout.h"

#include <knx/knx_buttons.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(KNX_BUTTONS_COUNT >= 4, "The DK layout requires four buttons");
BUILD_ASSERT(KNX_LEDS_COUNT >= 4, "The DK layout requires four LEDs");

#define LED_THREAD DK_LED1_MSK
#define LED_KNX	   DK_LED2_MSK

static const struct knx_ui_binding bindings[] = {
	{
		.button = DK_BTN1,
		.states = KNX_UI_STATES_ALL,
		.short_action = KNX_UI_ACTION_THREAD_JOIN,
		.long_action = KNX_UI_ACTION_THREAD_FACTORY_RESET,
	},
#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	{
		.button = DK_BTN2,
		.states = KNX_UI_STATES_ALL,
		.short_action = KNX_UI_ACTION_KNX_PROGRAMMING_MODE,
		.long_action = KNX_UI_ACTION_KNX_FACTORY_RESET,
	},
#endif
	{
		.button = DK_BTN3,
		.states = KNX_UI_STATES_ALL,
		.short_action = KNX_UI_ACTION_APP_BUTTON_1,
	},
	{
		.button = DK_BTN4,
		.states = KNX_UI_STATES_ALL,
		.short_action = KNX_UI_ACTION_APP_BUTTON_2,
	},
};

static const uint8_t app_leds[] = {
	DK_LED3,
	DK_LED4,
};

const struct knx_ui_layout knx_ui_layout = {
	.bindings = bindings,
	.num_bindings = ARRAY_SIZE(bindings),
	.indications = {
		[KNX_UI_STATE_ATTACHING] = {
			{LED_THREAD, KNX_LED_BLINK},
		},
		[KNX_UI_STATE_ATTACHED] = {
			{LED_THREAD, KNX_LED_ON},
		},
		[KNX_UI_STATE_ATTACHED_PM] = {
			{LED_THREAD, KNX_LED_ON},
			{LED_KNX, KNX_LED_BLINK},
		},
		[KNX_UI_STATE_COMMISSIONED] = {
			{LED_THREAD, KNX_LED_ON},
			{LED_KNX, KNX_LED_ON},
		},
		[KNX_UI_STATE_COMMISSIONED_PM] = {
			{LED_THREAD, KNX_LED_ON},
			{LED_KNX, KNX_LED_BLINK},
		},
	},
	.long_press_armed = {DK_ALL_LEDS_MSK, KNX_LED_BLINK},
	.app_leds = app_leds,
	.num_app_leds = ARRAY_SIZE(app_leds),
};
