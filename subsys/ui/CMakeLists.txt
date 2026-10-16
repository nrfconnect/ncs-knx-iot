#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(knx_leds.c)
zephyr_library_sources(knx_buttons.c)
zephyr_library_sources(knx_board.c)
zephyr_library_sources_ifdef(CONFIG_KNX_UI_LAYOUT_DK knx_ui_layout_dk.c)
zephyr_library_sources_ifdef(CONFIG_KNX_UI_LAYOUT_SINGLE_BUTTON_RGB
  knx_ui_layout_single_button_rgb.c)
