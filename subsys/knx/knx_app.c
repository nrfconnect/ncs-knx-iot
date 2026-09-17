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

#include <openthread.h>
#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/joiner.h>
#include <openthread/thread.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
#include <ram_pwrdn.h>
#endif

#include "port/dns-sd.h"
#include "oc_api.h"
#include "port/oc_clock.h"
#include "port/oc_connectivity.h"
#include "oc_core_res.h"
#include "oc_knx.h"
#include "oc_knx_dev.h"

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
	atomic_t thread_joiner_pending;
	atomic_t thread_factory_reset_pending;
	atomic_t knx_factory_reset_pending;
	atomic_t network_state_pending;
#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	atomic_t pm_toggle_pending;
#if defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
	struct k_timer pm_autostart_timer;
	atomic_t pm_autostart_expired;
#endif
#endif
} knx_ctx;

/* Stack hook (oc_handler_t.signal_event_loop): wake the poll loop when the
 * stack has work before the next scheduled timer.
 */
void signal_event_loop(void)
{
	k_sem_give(&knx_ctx.event_sem);
}

static void knx_network_state_changed(enum knx_network_state state)
{
	ARG_UNUSED(state);

	atomic_set(&knx_ctx.network_state_pending, 1);
	signal_event_loop();
}

static void knx_post_action(atomic_t *pending)
{
	atomic_set(pending, 1);
	signal_event_loop();
}

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
static void knx_set_programming_mode(bool enabled)
{
	oc_knx_device_set_programming_mode(enabled);
#if defined(CONFIG_DK_LIBRARY)
	knx_board_update_status();
#endif
}
#endif

#if defined(CONFIG_KNX_ETS_COMMISSIONING) && defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
static void knx_pm_autostart_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	atomic_set(&knx_ctx.pm_autostart_expired, 1);
	signal_event_loop();
}

static void knx_stop_pm_autostart_timer(void)
{
	k_timer_stop(&knx_ctx.pm_autostart_timer);
	atomic_set(&knx_ctx.pm_autostart_expired, 0);
}

static void knx_disable_programming_mode(void)
{
	if (!oc_knx_device_in_programming_mode()) {
		return;
	}

	knx_set_programming_mode(false);
	LOG_INF("programming mode OFF");
}

static void knx_autostart_programming_mode(void)
{
	if (oc_is_device_in_runtime()) {
		LOG_DBG("programming mode autostart skipped: device is commissioned");
		return;
	}

	if (oc_knx_device_in_programming_mode()) {
		return;
	}

	knx_set_programming_mode(true);
	k_timer_start(&knx_ctx.pm_autostart_timer,
		      K_MINUTES(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART_TIMEOUT_MINUTES), K_NO_WAIT);
	LOG_INF("programming mode ON for %d minutes",
		CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART_TIMEOUT_MINUTES);
}
#endif

static void knx_request_thread_joiner(void)
{
	knx_post_action(&knx_ctx.thread_joiner_pending);
}

#if defined(CONFIG_OPENTHREAD_JOINER_AUTOSTART)
static void knx_thread_joiner_callback(otError error, void *context)
{
	ARG_UNUSED(context);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Thread Joiner failed [%d]", error);
		return;
	}

	otInstance *instance = openthread_get_default_instance();
	if (instance == NULL) {
		LOG_ERR("Thread Joiner succeeded but OpenThread is unavailable");
		return;
	}

	error = otThreadSetEnabled(instance, true);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to start Thread after Joiner success [%d]", error);
		return;
	}

	LOG_INF("Thread Joiner succeeded; Thread network start requested");
}
#endif

static void knx_start_thread_joiner(void)
{
	otInstance *instance = openthread_get_default_instance();

	if (instance == NULL) {
		LOG_ERR("Cannot start Thread Joiner: OpenThread is unavailable");
		return;
	}

#if defined(CONFIG_OPENTHREAD_JOINER_AUTOSTART)
	otError error;

	openthread_mutex_lock();

	if (otJoinerGetState(instance) != OT_JOINER_STATE_IDLE) {
		LOG_INF("Thread Joiner is already active");
		openthread_mutex_unlock();
		return;
	}

	if (otDatasetIsCommissioned(instance)) {
		error = otThreadSetEnabled(instance, true);
		openthread_mutex_unlock();

		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to enable the commissioned Thread network [%d]", error);
		} else {
			LOG_INF("Commissioned Thread network start requested");
		}
		return;
	}

	error = otJoinerStart(instance, CONFIG_OPENTHREAD_JOINER_PSKD, NULL, NULL, NULL, NULL, NULL,
		knx_thread_joiner_callback, NULL);
	openthread_mutex_unlock();

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to start Thread Joiner [%d]", error);
	} else {
		LOG_INF("Thread Joiner start requested");
	}
#if defined(CONFIG_DK_LIBRARY)
	knx_board_update_status();
#endif
#else
	LOG_WRN("Automatic Thread Joiner is not enabled in this build");
#endif
}

void knx_app_post_work(void)
{
	knx_post_action(&knx_ctx.work_pending);
}

void knx_app_set_work_handler(knx_lifecycle_cb_t handler)
{
	knx_ctx.work_handler = handler;
}

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
/* Request a programming-mode toggle on the KNX thread. */
static void knx_request_pm_toggle(void)
{
	knx_post_action(&knx_ctx.pm_toggle_pending);
}

static void knx_toggle_programming_mode(void)
{
	bool pm = !oc_knx_device_in_programming_mode();

#if defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
	knx_stop_pm_autostart_timer();
#endif
	knx_set_programming_mode(pm);
	LOG_INF("programming mode %s", pm ? "ON" : "OFF");
}

static void knx_programming_mode_cb(bool pm, void *data)
{
	ARG_UNUSED(data);

#if defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
	knx_stop_pm_autostart_timer();
#endif
	knx_set_programming_mode(pm);
	LOG_INF("programming mode %s (ETS)", pm ? "ON" : "OFF");
}
#endif /* CONFIG_KNX_ETS_COMMISSIONING */

static void knx_request_thread_factory_reset(void)
{
	knx_post_action(&knx_ctx.thread_factory_reset_pending);
}

static void knx_request_knx_factory_reset(void)
{
	knx_post_action(&knx_ctx.knx_factory_reset_pending);
}

static void knx_thread_factory_reset(void)
{
	otInstance *instance = openthread_get_default_instance();

	LOG_INF("Thread factory reset requested");
	if (instance == NULL) {
		LOG_ERR("Cannot factory reset Thread: OpenThread is unavailable");
		return;
	}

	openthread_mutex_lock();
	otInstanceFactoryReset(instance);
	openthread_mutex_unlock();
}

static void knx_knx_factory_reset(void)
{
	LOG_INF("KNX factory reset requested");
	oc_knx_device_reset(RESET_TO_DEFAULT_STATE);
}

static void knx_process_pending_actions(void)
{
	if (atomic_cas(&knx_ctx.thread_joiner_pending, 1, 0)) {
		knx_start_thread_joiner();
	}

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	if (atomic_cas(&knx_ctx.pm_toggle_pending, 1, 0)) {
		knx_toggle_programming_mode();
	}
#if defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
	if (atomic_cas(&knx_ctx.pm_autostart_expired, 1, 0)) {
		knx_disable_programming_mode();
	}
#endif
#endif

	if (atomic_cas(&knx_ctx.thread_factory_reset_pending, 1, 0)) {
		knx_thread_factory_reset();
	}
	if (atomic_cas(&knx_ctx.knx_factory_reset_pending, 1, 0)) {
		knx_knx_factory_reset();
#if defined(CONFIG_DK_LIBRARY)
		knx_board_update_status();
#endif
	}
}

static void knx_on_network_ready(const knx_device_t *dev)
{
#if defined(CONFIG_KNX_ETS_COMMISSIONING) && defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
	knx_autostart_programming_mode();
#endif

	const oc_device_info_t *device = oc_core_get_device_info();

	LOG_DBG("serial number: %s", device->serialnumber);
	LOG_DBG("host name: %s", oc_string(device->iot_hostname));

	(void)oc_connectivity_get_endpoints();
	LOG_INF("Thread network attached");
	if (knx_dns_sd_update_service(device->serialnumber, device->iid, device->ia, device->pm) < 0) {
		LOG_ERR("Failed to publish KNX service");
	} else {
		LOG_INF("KNX service published");
	}

	if (dev->on_ready != NULL) {
		dev->on_ready();
	}
}

static void knx_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const knx_device_t *dev = knx_device_get();
	bool network_services_started = false;

	LOG_INF("KNX thread started");

	atomic_set(&knx_ctx.running, 1);

	while (atomic_get(&knx_ctx.running)) {
		if (knx_ctx.work_handler != NULL && atomic_cas(&knx_ctx.work_pending, 1, 0)) {
			knx_ctx.work_handler();
		}

		knx_process_pending_actions();

		if (atomic_cas(&knx_ctx.network_state_pending, 1, 0)) {
#if defined(CONFIG_DK_LIBRARY)
			knx_board_update_status();
#endif
		}

		if (!network_services_started && knx_is_network_connected()) {
			knx_on_network_ready(dev);
			network_services_started = true;
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

int knx_app_start(void)
{
	const knx_device_t *dev = knx_device_get();

	if (dev == NULL || dev->identity == NULL) {
		LOG_ERR("No KNX device registered");
		return -EINVAL;
	}

	atomic_set(&knx_ctx.running, 0);
	atomic_set(&knx_ctx.work_pending, 0);
	atomic_set(&knx_ctx.thread_joiner_pending, 0);
	atomic_set(&knx_ctx.thread_factory_reset_pending, 0);
	atomic_set(&knx_ctx.knx_factory_reset_pending, 0);
	atomic_set(&knx_ctx.network_state_pending, 0);
#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	atomic_set(&knx_ctx.pm_toggle_pending, 0);
#if defined(CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART)
	atomic_set(&knx_ctx.pm_autostart_expired, 0);
	k_timer_init(&knx_ctx.pm_autostart_timer, knx_pm_autostart_timer_expiry, NULL);
#endif
#endif
	k_sem_init(&knx_ctx.event_sem, 0, 1);

#if defined(CONFIG_KNX_ETS_COMMISSIONING)
	oc_set_programming_mode_cb(knx_programming_mode_cb, NULL);
#endif

	int ret = knx_stack_init(NULL);

	if (ret < 0) {
		LOG_ERR("KNX stack initialization failed: %d", ret);
		return ret;
	}
	LOG_INF("KNX stack initialized");

#if defined(CONFIG_KNX_HARDCODED_COMMISSIONING)
	if (dev->preset != NULL) {
		ret = knx_apply_presets(dev->preset);
		if (ret < 0) {
			LOG_ERR("Failed to apply KNX presets: %d", ret);
			return ret;
		}
	}
#endif

#if defined(CONFIG_DK_LIBRARY)
	ret = knx_board_init();
	if (ret < 0) {
		LOG_ERR("Board initialization failed: %d", ret);
		return ret;
	}

	const struct knx_board_system_handlers system_handlers = {
		.thread_joiner = knx_request_thread_joiner,
		.thread_factory_reset = knx_request_thread_factory_reset,
#if defined(CONFIG_KNX_ETS_COMMISSIONING)
		.programming_mode = knx_request_pm_toggle,
#endif
		.knx_factory_reset = knx_request_knx_factory_reset,
	};

	knx_board_set_system_handlers(&system_handlers);
#endif

	ret = knx_set_network_state_handler(knx_network_state_changed);
	if (ret < 0) {
		LOG_ERR("Failed to register network state handler: %d", ret);
		return ret;
	}

	/* Sample-owned setup: app input handlers, initial output state. */
	if (dev->on_init != NULL) {
		dev->on_init();
	}

	knx_ctx.thread_id = k_thread_create(
		&knx_ctx.thread_data, knx_thread_stack, K_THREAD_STACK_SIZEOF(knx_thread_stack),
		knx_thread_entry, NULL, NULL, NULL, CONFIG_KNX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (knx_ctx.thread_id == NULL) {
		LOG_ERR("Failed to create KNX thread");
		return -EAGAIN;
	}
	ret = k_thread_name_set(knx_ctx.thread_id, "knx_app");
	if (ret < 0) {
		LOG_WRN("Failed to name KNX thread: %d", ret);
	}

#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
	power_down_unused_ram();
#endif

	return 0;
}
