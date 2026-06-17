/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_H_
#define APP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register this sample's KNX device profile with the add-on.
 *
 * Defined by the sample's device source (knx_actuator.c). Called from main()
 * before knx_app_start().
 */
void app_register_device(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_H_ */
