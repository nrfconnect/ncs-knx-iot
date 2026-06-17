/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Device-agnostic KNX worker thread and event loop. Drives the registered
 * device profile, then runs
 * oc_main_poll() forever. All device-specific behaviour is delegated to the
 * profile's callbacks.
 */

#include <knx/knx_app.h>
#include <knx/knx_device.h>
#include <knx/knx_transport.h>
#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
#include <knx/knx_presets.h>
#endif
#if defined(CONFIG_DK_LIBRARY)
#include <knx/knx_board.h>
#endif

#include "knx_priv.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "dns-sd.h"
#include "oc_api.h"
#include "oc_clock.h"
#include "oc_connectivity.h"
#include "oc_core_res.h"

LOG_MODULE_REGISTER(knx_app, LOG_LEVEL_INF);

K_THREAD_STACK_DEFINE(knx_thread_stack, CONFIG_KNX_THREAD_STACK_SIZE);

static struct {
  struct k_thread thread_data;
  k_tid_t thread_id;
  atomic_t running;
  /* The stack calls signal_event_loop() (from its own/network threads) to
   * wake the poll loop early.
   */
  struct k_sem event_sem;
  /* Generic deferred work: an application input source posts work from another
   * context; the loop drains it on the KNX thread via work_handler.
   */
  atomic_t work_pending;
  knx_lifecycle_cb_t work_handler;
} knx_ctx;

/* Stack hook (oc_handler_t.signal_event_loop): wake the poll loop when the
 * stack has work before the next scheduled timer.
 */
void signal_event_loop(void) { k_sem_give(&knx_ctx.event_sem); }

void knx_app_post_work(void) {
  atomic_set(&knx_ctx.work_pending, 1);
  signal_event_loop();
}

void knx_app_set_work_handler(knx_lifecycle_cb_t handler) {
  knx_ctx.work_handler = handler;
}

static void knx_thread_entry(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  const knx_device_t *dev = knx_device_get();

  LOG_INF("KNX thread started");

  knx_wait_for_network_ready();

  const oc_device_info_t *device = oc_core_get_device_info();

  LOG_INF("serial number: %s", oc_string(device->serialnumber));
  LOG_INF("host name: %s", oc_string(device->iot_hostname));

  (void)oc_connectivity_get_endpoints();
  knx_publish_service(oc_string(device->serialnumber), device->iid, device->ia,
                      device->pm);
  LOG_INF("KNX service published; entering event loop");

  /* Network is up and the service is announced: light the shared status LED. */
#if defined(CONFIG_DK_LIBRARY)
  knx_board_set_status_led(true);
#endif
  if (dev->on_ready != NULL) {
    dev->on_ready();
  }

  atomic_set(&knx_ctx.running, 1);

  while (atomic_get(&knx_ctx.running)) {
    if (knx_ctx.work_handler != NULL &&
        atomic_cas(&knx_ctx.work_pending, 1, 0)) {
      knx_ctx.work_handler();
    }

    /* Run all ready stack processes; returns the absolute time of the
     * next scheduled event (0 = nothing scheduled).
     */
    oc_clock_time_t next_event = oc_main_poll();

    if (next_event == 0) {
      k_sem_take(&knx_ctx.event_sem, K_FOREVER);
    } else {
      int64_t time_diff = (int64_t)next_event - (int64_t)oc_clock_time();
      k_timeout_t timeout;

      if (time_diff <= 0) {
        timeout = K_NO_WAIT;
      } else if (time_diff > INT32_MAX) {
        timeout = K_MSEC(INT32_MAX);
      } else {
        timeout = K_MSEC((uint32_t)time_diff);
      }
      k_sem_take(&knx_ctx.event_sem, timeout);
    }
  }

  oc_main_shutdown();
  LOG_INF("KNX thread stopped");
}

int knx_app_start(void) {
  const knx_device_t *dev = knx_device_get();

  if (dev == NULL || dev->identity == NULL) {
    LOG_ERR("No KNX device registered");
    return -EINVAL;
  }

  atomic_set(&knx_ctx.running, 0);
  atomic_set(&knx_ctx.work_pending, 0);
  k_sem_init(&knx_ctx.event_sem, 0, 1);

  int ret = knx_stack_init(NULL);

  if (ret < 0) {
    LOG_ERR("knx_stack_init failed: %d", ret);
    return ret;
  }
  LOG_INF("KNX stack initialized");

#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
  if (dev->preset != NULL) {
    ret = knx_apply_presets(dev->preset);
    if (ret < 0) {
      LOG_ERR("failed to apply KNX presets: %d", ret);
      return ret;
    }
  }
#endif

#if defined(CONFIG_DK_LIBRARY)
  ret = knx_board_init();
  if (ret < 0) {
    LOG_ERR("board init failed: %d", ret);
    return ret;
  }
#endif

  /* Sample-owned setup: app input handlers, initial output state. */
  if (dev->on_init != NULL) {
    dev->on_init();
  }

  knx_ctx.thread_id = k_thread_create(&knx_ctx.thread_data, knx_thread_stack,
                                      K_THREAD_STACK_SIZEOF(knx_thread_stack),
                                      knx_thread_entry, NULL, NULL, NULL,
                                      CONFIG_KNX_THREAD_PRIORITY, 0, K_NO_WAIT);
  if (knx_ctx.thread_id == NULL) {
    LOG_ERR("Failed to create KNX thread");
    return -EAGAIN;
  }
  k_thread_name_set(knx_ctx.thread_id, "knx_app");

  return 0;
}
