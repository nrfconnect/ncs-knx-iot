/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_transport.h>

#include <openthread.h>
#include <openthread/thread.h>
#include <zephyr/kernel.h>

#define TIMEOUT 100

bool knx_is_network_connected(void) {
  otInstance *instance = openthread_get_default_instance();

  if (instance == NULL) {
    return false;
  }

  return (otThreadGetDeviceRole(instance) >= OT_DEVICE_ROLE_CHILD);
}

void knx_wait_for_network_ready(void) {
  otInstance *instance = NULL;

  do {
    instance = openthread_get_default_instance();
    k_msleep(TIMEOUT);
  } while (instance == NULL ||
           otThreadGetDeviceRole(instance) < OT_DEVICE_ROLE_CHILD);
}
