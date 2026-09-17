#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(KNX_IOT_SPAKE_RECORD_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/knx_spake_record.c")
add_custom_command(
  OUTPUT ${KNX_IOT_SPAKE_RECORD_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${KNX_IOT_SPAKE_RECORD_GENERATOR}
          --stack-generator ${KNX_IOT_STACK_SPAKE_RECORD_GENERATOR}
          --password "${CONFIG_KNX_IOT_PASSWORD}"
          --symbol knx_dev_spake_record
          --output ${KNX_IOT_SPAKE_RECORD_SOURCE}
  DEPENDS ${KNX_IOT_SPAKE_RECORD_GENERATOR}
          ${KNX_IOT_STACK_SPAKE_RECORD_GENERATOR}
  COMMENT "Generating the KNX IoT SPAKE2+ verifier record"
  VERBATIM
)

# Device-agnostic application layer.
zephyr_library_sources(knx_app.c)
zephyr_library_sources(knx_stack.c)
zephyr_library_sources(knx_resources.c)
zephyr_library_sources(knx_transport.c)
zephyr_library_sources(${KNX_IOT_SPAKE_RECORD_SOURCE})

# Board UI
zephyr_library_sources_ifdef(CONFIG_DK_LIBRARY knx_leds.c)
zephyr_library_sources_ifdef(CONFIG_DK_LIBRARY knx_buttons.c)
zephyr_library_sources_ifdef(CONFIG_DK_LIBRARY knx_board.c)

# Hardcoded KNX commissioning for development.
zephyr_library_sources_ifdef(CONFIG_KNX_HARDCODED_COMMISSIONING knx_presets.c)
