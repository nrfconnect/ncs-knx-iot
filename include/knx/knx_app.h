/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KNX_APP_H_
#define KNX_APP_H_

#include <knx/knx_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the KNX IoT stack and start the KNX worker thread.
 *
 * Intended to be the single entry point a sample's main() calls after it has
 * registered its device profile with knx_device_register(). Wraps stack
 * initialization, optional hardcoded commissioning, the device's on_init hook
 * and the background event loop.
 *
 * @return 0 on success, negative errno on failure.
 */
int knx_app_start(void);

/**
 * @brief Wake the KNX event loop to run the registered work handler.
 *
 * Safe to call from any context (ISR-free work queue, another thread). The work
 * handler set with knx_app_set_work_handler() runs once on the KNX thread. Used
 * by application input sources (e.g. a button) to defer work onto the thread
 * that owns the stack.
 */
void knx_app_post_work(void);

/**
 * @brief Register the handler run on the KNX thread when work is posted.
 *
 * Optional. Can be for example used to handle KNX stack behaviour on button
 * press.
 */
void knx_app_set_work_handler(knx_lifecycle_cb_t handler);

#ifdef __cplusplus
}
#endif

#endif /* KNX_APP_H_ */
