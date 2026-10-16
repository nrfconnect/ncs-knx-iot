/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Board UI layout description. A layout maps the device state to LED
 * indications and maps button presses, per state, to actions. Exactly one
 * layout is linked, selected by the KNX_UI_LAYOUT Kconfig choice; it defines
 * knx_ui_layout. The generic engine in knx_board.c interprets it.
 *
 * To support a new board UI, add a layout source file and a Kconfig choice
 * entry for it.
 */

#ifndef KNX_UI_LAYOUT_H_
#define KNX_UI_LAYOUT_H_

#include <knx/knx_leds.h>

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Device state as presented by the UI. */
enum knx_ui_state {
	/** Thread disabled or idle; not attached and not joining. */
	KNX_UI_STATE_DOWN,
	/** Joining or attaching to a Thread network. */
	KNX_UI_STATE_ATTACHING,
	/** Attached to Thread, KNX not commissioned. */
	KNX_UI_STATE_ATTACHED,
	/** Attached to Thread, KNX not commissioned, programming mode on. */
	KNX_UI_STATE_ATTACHED_PM,
	/** Attached to Thread and KNX commissioned. */
	KNX_UI_STATE_COMMISSIONED,
	/** Attached to Thread, KNX commissioned, programming mode on. */
	KNX_UI_STATE_COMMISSIONED_PM,
	KNX_UI_STATE_COUNT,
};

/** @brief Bit mask of one or more UI states. */
#define KNX_UI_STATES(...) (FOR_EACH(BIT, (|), __VA_ARGS__))

/** @brief Bit mask of every UI state. */
#define KNX_UI_STATES_ALL BIT_MASK(KNX_UI_STATE_COUNT)

/** @brief Action triggered by a button gesture. */
enum knx_ui_action {
	KNX_UI_ACTION_NONE,
	KNX_UI_ACTION_THREAD_JOIN,
	KNX_UI_ACTION_THREAD_FACTORY_RESET,
	KNX_UI_ACTION_KNX_PROGRAMMING_MODE,
	KNX_UI_ACTION_KNX_FACTORY_RESET,
	KNX_UI_ACTION_APP_BUTTON_1,
	KNX_UI_ACTION_APP_BUTTON_2,
};

/**
 * @brief Actions of one button while the device is in one of @ref states.
 *
 * The binding is resolved when the button is pressed. Without a long action,
 * the short action runs immediately on press. With a long action, the short
 * action runs on release and the long action after a confirmed long press.
 */
struct knx_ui_binding {
	uint8_t button;	 /**< DK button index. */
	uint32_t states; /**< Mask of states in which the binding applies. */
	enum knx_ui_action short_action;
	enum knx_ui_action long_action;
};

/** @brief One LED mode shown on a set of LEDs. */
struct knx_ui_indication {
	uint32_t leds; /**< Mask of DK LED indices. Zero marks an unused entry. */
	enum knx_led_mode mode;
};

/** @brief Maximum number of indications shown at the same time. */
#define KNX_UI_MAX_INDICATIONS 2

/** @brief Complete board UI description. */
struct knx_ui_layout {
	const struct knx_ui_binding *bindings;
	size_t num_bindings;
	/**
	 * LED indications per state. LEDs used by any state are owned by the
	 * status display and switched off when the current state does not
	 * light them.
	 */
	struct knx_ui_indication indications[KNX_UI_STATE_COUNT][KNX_UI_MAX_INDICATIONS];
	/** Shown while a long-press action is armed. Other LEDs are off. */
	struct knx_ui_indication long_press_armed;
	/** DK LED index per application LED (enum knx_board_app_led). */
	const uint8_t *app_leds;
	size_t num_app_leds;
};

/** @brief The layout selected by Kconfig. */
extern const struct knx_ui_layout knx_ui_layout;

#ifdef __cplusplus
}
#endif

#endif /* KNX_UI_LAYOUT_H_ */
