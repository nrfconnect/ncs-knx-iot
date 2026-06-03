#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

# Device-agnostic application layer (replaces the stack's bundled demo apps,
# which are disabled via CONFIG_KNXIOT_LIB_DEMO_APPS=n in Kconfig.defconfig).
zephyr_library_sources(knx_app.c)
zephyr_library_sources(knx_stack.c)
zephyr_library_sources(knx_resources.c)
zephyr_library_sources(knx_transport.c)

# Shared board support (status LED, button dispatch, future factory reset).
zephyr_library_sources_ifdef(CONFIG_DK_LIBRARY knx_board.c)

# Hardcoded "automatic" commissioning mechanism (parameterised by the device).
zephyr_library_sources_ifdef(CONFIG_KNX_HARDCODED_COMMISSIONING knx_presets.c)
