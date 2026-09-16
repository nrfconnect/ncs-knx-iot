# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# This CMake file is picked by the Zephyr build system because it is defined
# as the module CMake entry point (see zephyr/module.yml).

if (CONFIG_KNX_IOT_ADD_ON)
  zephyr_include_directories(include)

  set(KNX_IOT_SPAKE_RECORD_GENERATOR
      "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_spake_record.py")
  set(KNX_IOT_STACK_SPAKE_RECORD_GENERATOR
      "${ZEPHYR_SDK_KNX_IOT_STACK_MODULE_DIR}/tests/runtime/gen_spake_record.py")

  add_subdirectory(subsys)
endif()
