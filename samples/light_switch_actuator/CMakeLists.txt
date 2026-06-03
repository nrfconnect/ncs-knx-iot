# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(EXTRA_CONF_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/knx.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(light_switch_actuator)

# Shared sample headers (knx_hardcoded.h).
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

target_sources(app PRIVATE
  main.c
  knx_actuator.c
)
