/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <knx/knx_transport.h>

#include <openthread.h>
#include <openthread/joiner.h>
#include <openthread/thread.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_transport, LOG_LEVEL_INF);

static knx_network_state_handler_t state_handler;
static bool state_handler_registered;

enum knx_network_state knx_get_network_state(void)
{
	otInstance *instance = openthread_get_default_instance();
	otDeviceRole current_device_role;
#if defined(CONFIG_OPENTHREAD_JOINER)
	otJoinerState current_joiner_state;
#endif

	if (instance == NULL) {
		return KNX_NETWORK_DOWN;
	}

	openthread_mutex_lock();
	current_device_role = otThreadGetDeviceRole(instance);
#if defined(CONFIG_OPENTHREAD_JOINER)
	current_joiner_state = otJoinerGetState(instance);
#endif
	openthread_mutex_unlock();

	switch (current_device_role) {
	case OT_DEVICE_ROLE_CHILD:
	case OT_DEVICE_ROLE_ROUTER:
	case OT_DEVICE_ROLE_LEADER:
		return KNX_NETWORK_ATTACHED;
	case OT_DEVICE_ROLE_DETACHED:
		return KNX_NETWORK_ATTACHING;
	case OT_DEVICE_ROLE_DISABLED:
#if defined(CONFIG_OPENTHREAD_JOINER)
		return current_joiner_state != OT_JOINER_STATE_IDLE ? KNX_NETWORK_ATTACHING
								    : KNX_NETWORK_DOWN;
#else
		return KNX_NETWORK_DOWN;
#endif
	default:
		return KNX_NETWORK_DOWN;
	}
}

bool knx_is_network_connected(void)
{
	return knx_get_network_state() == KNX_NETWORK_ATTACHED;
}

static void ot_state_changed(otChangedFlags flags, void *user_data)
{
	ARG_UNUSED(user_data);

	if ((flags & (OT_CHANGED_THREAD_ROLE | OT_CHANGED_JOINER_STATE)) && state_handler != NULL) {
		state_handler(knx_get_network_state());
	}
}

static struct openthread_state_changed_callback ot_cb = {
	.otCallback = ot_state_changed,
};

int knx_set_network_state_handler(knx_network_state_handler_t handler)
{
	int err;

	state_handler = handler;

	if (handler == NULL) {
		if (state_handler_registered) {
			err = openthread_state_changed_callback_unregister(&ot_cb);
			if (err) {
				return err;
			}
			state_handler_registered = false;
		}
		return 0;
	}

	if (!state_handler_registered) {
		err = openthread_state_changed_callback_register(&ot_cb);
		if (err) {
			state_handler = NULL;
			return err;
		}
		state_handler_registered = true;
	}

	handler(knx_get_network_state());

	return 0;
}
