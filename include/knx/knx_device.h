/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generic KNX IoT application model.
 *
 * This file defines the common device model used by both the application stack,
 * as well as by the sample-level logic.
 * The structure is the following:
 * The knx_device_t is the top level, it houses the identity of the device, the
 * functional block, callbacks and optional presets.
 *
 * Functional blocks are standard defined, for example FB number 417 is LSAB
 * (Light Switch Actuator Basic) (Kind of like a Matter Cluster) They consist of
 * datapoints, specific values for example a datapoints can expose switch state
 * (Kind of like a Attribute)
 */

#ifndef KNX_DEVICE_H_
#define KNX_DEVICE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "oc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A datapoint id packs the functional-block channel and the point index into a
 * stable handle used for mirror targets and the by-id accessors below.
 */
#define KNX_DP_ID(channel, point) ((uint16_t)(((channel) << 8) | ((point) & 0xFF)))
#define KNX_DP_CHANNEL(id)	  ((uint8_t)(((id) >> 8) & 0xFF))
#define KNX_DP_POINT(id)	  ((uint8_t)((id) & 0xFF))

/* Sentinel for knx_datapoint_t.mirror_to (no mirroring). */
#define KNX_DP_NONE (-1)

/* The light switch sample currently only requires boolean data; the code is prepared for easy
 * extension to other types in the future.
 */
typedef enum {
	KNX_DPT_BOOL = 0,
} knx_dpt_kind_t;

typedef union {
	bool boolean;
} knx_datapoint_data_t;

typedef struct {
	knx_dpt_kind_t kind;
	knx_datapoint_data_t data;
} knx_datapoint_value_t;

typedef struct {
	char *path; /* resource path, e.g. "/p/lsab/0/soo" (chosen by app)*/
	char *dpa;  /* resource type / DPA URN, e.g. "urn:knx:dpa.417.52" (from the
		       standard)*/
	char *dpt;  /* datapoint type, e.g. ":dpt.switch" (from the standard)*/

	uint16_t id; /* KNX_DP_ID(channel, point), unique within the device */

	uint8_t methods;	   /* OC_GET and/or OC_PUT */
	oc_acl_mask_t acl;	   /* access scope for the handler(s) */
	oc_interface_mask_t iface; /* interface for the handler(s) */
	int32_t mirror_to;	   /* datapoint id mirrored + announced on write, or
				      KNX_DP_NONE */

	knx_datapoint_value_t value; /* current value, with its datapoint type */
} knx_datapoint_t;

typedef struct {
	uint16_t number;  /* KNX functional-block number, e.g. 417 (LSAB) */
	uint8_t instance; /* KNX functional-block instance, an identification number*/

	knx_datapoint_t *datapoints;
	uint8_t num_datapoints;
} knx_functional_block_t;

typedef struct {
	const char *serialnumber;     /* lower-case, e.g. "00fa10020700" */
	const char *application_name; /* human readable */
	const char *hw_type;	      /* hardware type bytes as a string */
	const char *dev_model;	      /* device model */
	uint32_t mid;		      /* manufacturer id */
} knx_identity_t;

/* Invoked on the KNX thread after a PUT updated a datapoint. */
typedef void (*knx_write_cb_t)(const knx_datapoint_t *dp);

/* Generic lifecycle hook. (on_init, on_ready etc.) */
typedef void (*knx_lifecycle_cb_t)(void);

struct knx_preset; /* defined in knx_presets.h */

typedef struct {
	const knx_identity_t *identity;
	const knx_functional_block_t *functional_blocks;
	size_t num_functional_blocks;

	/* All optional (may be NULL): */
	knx_lifecycle_cb_t on_init;  /* after stack init + presets, before the KNX thread starts */
	knx_lifecycle_cb_t on_ready; /* network up + service published (KNX thread) */
	knx_write_cb_t on_write;     /* after a PUT updated a datapoint (KNX thread) */

	const struct knx_preset *preset; /* hardcoded commissioning, or NULL */
} knx_device_t;

/**
 * @brief Register the device profile.
 *
 * Must be called exactly once, before knx_app_start().
 */
void knx_device_register(const knx_device_t *device);

/** @brief Get the registered device profile (NULL if none registered). */
const knx_device_t *knx_device_get(void);

/** @brief Look up a datapoint by id (NULL if not found). */
knx_datapoint_t *knx_datapoint_by_id(uint16_t id);

/* Those generic functions should be used in the common implementation. Whenever
 * data type is known, the typed helpers should be used*/
/**
 * @brief Read a typed datapoint value.
 *
 * The caller sets value->kind to the expected type. The function returns
 * -EINVAL if the datapoint exists but has a different type.
 */
int knx_datapoint_get(uint16_t id, knx_datapoint_value_t *value);

/** @brief Set a typed datapoint value. */
int knx_datapoint_set(uint16_t id, knx_datapoint_value_t value);

/** @brief Read a boolean datapoint value. */
int knx_datapoint_get_bool(uint16_t id, bool *value);

/** @brief Set a boolean datapoint value. */
int knx_datapoint_set_bool(uint16_t id, bool value);

/**
 * @brief Announce a datapoint value as an s-mode multicast write.
 *
 * Call from the KNX thread (e.g. a work handler).
 */
void knx_datapoint_transmit(uint16_t id);

#ifdef __cplusplus
}
#endif

#endif /* KNX_DEVICE_H_ */
