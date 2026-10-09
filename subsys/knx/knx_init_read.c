/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Read on init: tracks datapoints whose group object has the Read on Init (I)
 * flag until a response arrives, resends the requests if none arrives in time
 * and notifies the application when a datapoint stops waiting.
 */

#include <knx/knx_device.h>

#include "knx_priv.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

#include "api/oc_knx_fp.h"
#include "oc_api.h"
#include "oc_knx.h"

LOG_MODULE_REGISTER(knx_init_read, LOG_LEVEL_INF);

static size_t init_read_pending_count;
static uint8_t init_read_retries_left;
static bool init_read_needed;

static void init_read_mark(knx_datapoint_t *dp)
{
	const oc_group_object_table_t *go = oc_core_find_sending_ga_in_pos_zero_for_href(dp->path);

	dp->init_read_pending = go != NULL && (go->cflags & OC_CFLAG_INIT);
	if (dp->init_read_pending) {
		init_read_pending_count++;
	}
}

void knx_init_read_finish(knx_datapoint_t *dp, bool received)
{
	if (!dp->init_read_pending) {
		return;
	}

	dp->init_read_pending = false;
	init_read_pending_count--;

	const knx_device_t *device = knx_device_get();

	if (device->on_init_read != NULL) {
		device->on_init_read(dp, received);
	}
}

static void init_read_give_up(knx_datapoint_t *dp)
{
	knx_init_read_finish(dp, false);
}

/* The stack sends one request per KNX_READ_ON_INIT_DELAY_MS, starting one
 * delay after the scan begins. Wait for the last one plus the response time.
 */
static oc_clock_time_t init_read_wait_ticks(void)
{
	const uint64_t wait_ms =
		(uint64_t)(init_read_pending_count + 1) * CONFIG_KNX_READ_ON_INIT_DELAY_MS +
		CONFIG_KNX_READ_ON_INIT_RESPONSE_TIMEOUT_MS;

	return (oc_clock_time_t)(wait_ms * OC_CLOCK_SECOND / MSEC_PER_SEC);
}

/* Returns instead of removing itself: the stack frees a timed event after its
 * callback returns OC_EVENT_DONE.
 */
static oc_event_callback_retval_t init_read_timeout(void *data)
{
	(void)data;

	if (init_read_pending_count == 0) {
		return OC_EVENT_DONE;
	}

	if (init_read_retries_left > 0) {
		init_read_retries_left--;
		LOG_WRN("No read-on-init response for %zu datapoints, sending the requests again",
			init_read_pending_count);
		oc_init_datapoints_at_initialization();
		return OC_EVENT_CONTINUE;
	}

	LOG_WRN("No read-on-init response for %zu datapoints", init_read_pending_count);
	knx_datapoint_for_each(init_read_give_up);

	return OC_EVENT_DONE;
}

void knx_init_read_begin(void)
{
	if (knx_device_get() == NULL) {
		LOG_ERR("read on init started before KNX device registration");
		return;
	}

	init_read_pending_count = 0;
	if (oc_is_device_in_runtime()) {
		knx_datapoint_for_each(init_read_mark);
	}

	init_read_needed = init_read_pending_count > 0;
	if (!init_read_needed) {
		return;
	}

	init_read_retries_left = 0;
	oc_ri_remove_timed_event_callback(NULL, init_read_timeout);
	oc_ri_add_timed_event_callback_ticks(NULL, init_read_timeout, init_read_wait_ticks());
}

void knx_init_read_start(void)
{
	if (!init_read_needed) {
		return;
	}

	init_read_needed = false;
	oc_ri_remove_timed_event_callback(NULL, init_read_timeout);

	oc_init_datapoints_at_initialization();

	if (init_read_pending_count > 0) {
		init_read_retries_left = CONFIG_KNX_READ_ON_INIT_RETRIES;
		oc_ri_add_timed_event_callback_ticks(NULL, init_read_timeout,
						     init_read_wait_ticks());
	}
}

void knx_init_read_cancel(void)
{
	if (knx_device_get() == NULL) {
		return;
	}

	init_read_needed = false;
	knx_datapoint_for_each(init_read_give_up);
}
