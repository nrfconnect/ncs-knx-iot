/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Single button and RGB LED layout. The LED colour shows the commissioning
 * stage, and the button performs the action relevant to that stage:
 *
 *   Stage                          LED          Short press    Long press
 *   Thread down                    red flash    Thread join    -
 *   Thread attaching               red blink    Thread join    -
 *   KNX not commissioned           blue on      programming    Thread reset
 *   KNX not commissioned, PM on    blue blink   programming    Thread reset
 *   KNX commissioned               green flash  app button 1   KNX reset (1)
 *   KNX commissioned, PM on        blue blink   programming    KNX reset
 *
 * (1) Thread reset with hardcoded KNX commissioning.
 *
 * While a long press is armed, the LED blinks white.
 *
 * Devicetree requirements: the knx-ui-rgb-led alias points to a
 * leds-group-multicolor node whose LEDs are children of /leds, and the sw0
 * alias points to a child of /buttons.
 */

#include "knx_ui_layout.h"

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/sys/util.h>

#define RGB_NODE    DT_ALIAS(knx_ui_rgb_led)
#define BUTTON_NODE DT_ALIAS(sw0)

#if !DT_NODE_EXISTS(RGB_NODE)
#error "The single button RGB layout requires the knx-ui-rgb-led devicetree alias"
#endif

#if !DT_NODE_EXISTS(BUTTON_NODE)
#error "The single button RGB layout requires the sw0 devicetree alias"
#endif

BUILD_ASSERT(DT_NODE_HAS_COMPAT(RGB_NODE, leds_group_multicolor),
	     "knx-ui-rgb-led must point to a leds-group-multicolor node");
BUILD_ASSERT(DT_SAME_NODE(DT_PARENT(BUTTON_NODE), DT_PATH(buttons)),
	     "sw0 must be a child of /buttons");

#define LED_IN_LEDS_NODE(node_id, prop, idx)                                                       \
	&&DT_SAME_NODE(DT_PARENT(DT_PHANDLE_BY_IDX(node_id, prop, idx)), DT_PATH(leds))

BUILD_ASSERT(1 DT_FOREACH_PROP_ELEM(RGB_NODE, leds, LED_IN_LEDS_NODE),
	     "knx-ui-rgb-led LEDs must be children of /leds");

/* DK LED mask of the RGB channel mapped to a colour, or 0 if none is. */
#define CHANNEL_IF_COLOR(node_id, prop, idx, color)                                                \
	((DT_PROP_BY_IDX(node_id, prop, idx) == (color))                                           \
		 ? BIT(DT_NODE_CHILD_IDX(DT_PHANDLE_BY_IDX(node_id, leds, idx)))                   \
		 : 0) |
#define RGB_CHANNEL(color)                                                                         \
	(DT_FOREACH_PROP_ELEM_VARGS(RGB_NODE, color_mapping, CHANNEL_IF_COLOR, color) 0)

#define LED_RED	  RGB_CHANNEL(LED_COLOR_ID_RED)
#define LED_GREEN RGB_CHANNEL(LED_COLOR_ID_GREEN)
#define LED_BLUE  RGB_CHANNEL(LED_COLOR_ID_BLUE)
#define LED_WHITE (LED_RED | LED_GREEN | LED_BLUE)

BUILD_ASSERT(LED_RED != 0 && LED_GREEN != 0 && LED_BLUE != 0,
	     "knx-ui-rgb-led must map red, green, and blue");

#define BUTTON DT_NODE_CHILD_IDX(BUTTON_NODE)

/*
 * With hardcoded KNX commissioning the device is commissioned as soon as it
 * attaches, so the Thread factory reset is reachable only from here.
 */
#if defined(CONFIG_KNX_ETS_COMMISSIONING)
#define COMMISSIONED_LONG_ACTION KNX_UI_ACTION_KNX_FACTORY_RESET
#else
#define COMMISSIONED_LONG_ACTION KNX_UI_ACTION_THREAD_FACTORY_RESET
#endif

static const struct knx_ui_binding bindings[] = {
	{
		.button = BUTTON,
		.states = KNX_UI_STATES(KNX_UI_STATE_DOWN, KNX_UI_STATE_ATTACHING),
		.short_action = KNX_UI_ACTION_THREAD_JOIN,
	},
	{
		.button = BUTTON,
		.states = KNX_UI_STATES(KNX_UI_STATE_ATTACHED, KNX_UI_STATE_ATTACHED_PM),
		.short_action = KNX_UI_ACTION_KNX_PROGRAMMING_MODE,
		.long_action = KNX_UI_ACTION_THREAD_FACTORY_RESET,
	},
	{
		.button = BUTTON,
		.states = KNX_UI_STATES(KNX_UI_STATE_COMMISSIONED),
		.short_action = KNX_UI_ACTION_APP_BUTTON_1,
		.long_action = COMMISSIONED_LONG_ACTION,
	},
	{
		.button = BUTTON,
		.states = KNX_UI_STATES(KNX_UI_STATE_COMMISSIONED_PM),
		.short_action = KNX_UI_ACTION_KNX_PROGRAMMING_MODE,
		.long_action = KNX_UI_ACTION_KNX_FACTORY_RESET,
	},
};

const struct knx_ui_layout knx_ui_layout = {
	.bindings = bindings,
	.num_bindings = ARRAY_SIZE(bindings),
	.indications = {
		[KNX_UI_STATE_DOWN] = {
			{LED_RED, KNX_LED_FLASH},
		},
		[KNX_UI_STATE_ATTACHING] = {
			{LED_RED, KNX_LED_BLINK},
		},
		[KNX_UI_STATE_ATTACHED] = {
			{LED_BLUE, KNX_LED_ON},
		},
		[KNX_UI_STATE_ATTACHED_PM] = {
			{LED_BLUE, KNX_LED_BLINK},
		},
		[KNX_UI_STATE_COMMISSIONED] = {
			{LED_GREEN, KNX_LED_FLASH},
		},
		[KNX_UI_STATE_COMMISSIONED_PM] = {
			{LED_BLUE, KNX_LED_BLINK},
		},
	},
	.long_press_armed = {LED_WHITE, KNX_LED_BLINK},
};
