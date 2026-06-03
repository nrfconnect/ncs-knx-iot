/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_transport.h>

#include <openthread.h>
#include <openthread/thread.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_transport, LOG_LEVEL_INF);

#define WAIT_HEARTBEAT_PERIOD K_SECONDS(3)

static K_SEM_DEFINE(network_ready_sem, 0, 1);

bool knx_is_network_connected(void)
{
	otInstance *instance = openthread_get_default_instance();

	if (instance == NULL) {
		return false;
	}

	return (otThreadGetDeviceRole(instance) >= OT_DEVICE_ROLE_CHILD);
}

static void ot_state_changed(otChangedFlags flags, void *user_data)
{
	ARG_UNUSED(user_data);

	if ((flags & OT_CHANGED_THREAD_ROLE) && knx_is_network_connected()) {
		k_sem_give(&network_ready_sem);
	}
}

static struct openthread_state_changed_callback ot_cb = {
	.otCallback = ot_state_changed,
};

void knx_wait_for_network_ready(void)
{
	if (knx_is_network_connected()) {
		return;
	}

	openthread_state_changed_callback_register(&ot_cb);

	while (!knx_is_network_connected()) {
		LOG_INF("Waiting for Thread network to attach...");
		k_sem_take(&network_ready_sem, WAIT_HEARTBEAT_PERIOD);
	}

	openthread_state_changed_callback_unregister(&ot_cb);
	LOG_INF("Thread network attached");
}
