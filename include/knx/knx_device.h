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
 * The knx_device_t is the top level, it houses the identity of the device,
 * application parameters, functional blocks, callbacks and optional presets.
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
 * Channel UINT8_MAX is reserved for application parameters.
 */
#define KNX_DP_ID(channel, point) ((uint16_t)(((channel) << 8) | ((point) & 0xFF)))
#define KNX_DP_CHANNEL(id)	  ((uint8_t)(((id) >> 8) & 0xFF))
#define KNX_DP_POINT(id)	  ((uint8_t)((id) & 0xFF))
#define KNX_PARAMETER_ID(index)	  KNX_DP_ID(UINT8_MAX, index)
#define KNX_DP_IS_PARAMETER(id)	  (KNX_DP_CHANNEL(id) == UINT8_MAX)

/* Sentinel for knx_datapoint_t.mirror_to (no mirroring). */
#define KNX_DP_NONE (-1)

/* Access method flags for knx_datapoint_t.methods. */
#define KNX_DP_GET (1U << 0)
#define KNX_DP_PUT (1U << 1)

/* Supported datapoint value types. Add new types here and to the GET, PUT,
 * reset and persistence-size paths in knx_resources.c.
 */
typedef enum {
	KNX_DPT_BOOL = 0,	    /* :dpt.switch and other 1-bit booleans */
	KNX_DPT_VALUE_2_UCOUNT, /* :dpt.value2Ucount, 2-octet unsigned count */
} knx_dpt_kind_t;

typedef union {
	bool boolean;
	uint16_t value_2_ucount;
} knx_datapoint_data_t;

typedef struct {
	knx_dpt_kind_t kind;
	knx_datapoint_data_t data;
} knx_datapoint_value_t;

typedef struct knx_datapoint knx_datapoint_t;

/* Decision returned by a persistence policy for a single value change. */
typedef enum {
	KNX_PERSIST_SKIP = 0, /* keep the new value in RAM only */
	KNX_PERSIST_STORE,    /* write the new value to non-volatile storage now */
} knx_persist_action_t;

/*
 * Decides whether a datapoint value change is written to non-volatile storage.
 * Called on every knx_datapoint_set(), after the new value is applied.
 * Both values have dp->value.kind, so the policy can compare them by type.
 */
typedef knx_persist_action_t (*knx_persist_policy_t)(const knx_datapoint_t *dp,
						     const knx_datapoint_value_t *stored_value,
						     const knx_datapoint_value_t *new_value);

struct knx_datapoint {
	char *path; /* resource path, e.g. "/p/lsab/0/soo" (chosen by app)*/
	char *dpa;  /* resource type / DPA URN, e.g. "urn:knx:dpa.417.52" (from the
		       standard)*/
	char *dpt;  /* datapoint type, e.g. ":dpt.switch" (from the standard)*/

	uint16_t id; /* KNX_DP_ID or KNX_PARAMETER_ID, unique within the device */

	uint8_t methods;		     /* KNX_DP_GET and/or KNX_DP_PUT */
	oc_resource_properties_t properties; /* OC_DISCOVERABLE, OC_OBSERVABLE, etc. */
	oc_acl_mask_t get_acl;		     /* GET access scope */
	oc_interface_mask_t get_iface;	     /* GET interface */
	oc_acl_mask_t put_acl;		     /* PUT access scope */
	oc_interface_mask_t put_iface;	     /* PUT interface */
	int32_t mirror_to; /* datapoint id mirrored + announced on write, or KNX_DP_NONE */

	/* Restored at boot and written on change according to the policy, or
	 * NULL to keep the value in RAM only. Persisted values survive a KNX
	 * restart and are cleared by a KNX factory reset.
	 */
	knx_persist_policy_t persist;

	knx_datapoint_value_t value; /* current value, with its datapoint type */

	bool init_read_pending;	     /* Runtime state managed by the add-on */
	knx_datapoint_data_t stored; /* value in storage, has value.kind */
};

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

/* Invoked on the KNX thread after a PUT or a KNX factory reset updated a
 * datapoint.
 */
typedef void (*knx_write_cb_t)(const knx_datapoint_t *dp);

/* Invoked on the KNX thread when the device stops waiting for the read-on-init
 * response of a datapoint. received is true if a response or another write
 * updated the value, and false if the requests timed out or the device did not
 * attach in time. A response that arrives later is reported through on_write.
 */
typedef void (*knx_init_read_cb_t)(const knx_datapoint_t *dp, bool received);

/* Generic lifecycle hook. (on_init, on_ready etc.) */
typedef void (*knx_lifecycle_cb_t)(void);

struct knx_preset; /* defined in knx_presets.h */

typedef struct {
	const knx_identity_t *identity;

	knx_datapoint_t *parameters;
	size_t num_parameters;

	const knx_functional_block_t *functional_blocks;
	size_t num_functional_blocks;

	/* All optional (may be NULL): */
	knx_lifecycle_cb_t on_init;  /* after stack init + presets, before the KNX thread starts */
	knx_lifecycle_cb_t on_ready; /* network up + service published (KNX thread) */
	knx_write_cb_t on_write;     /* after a PUT or factory reset (KNX thread) */
	knx_init_read_cb_t
		on_init_read; /* read-on-init finished waiting for a datapoint value (KNX thread) */

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

/**
 * @brief Persistence policy that stores every write.
 *
 * Use as knx_datapoint_t.persist for datapoints that change rarely.
 */
knx_persist_action_t knx_persist_always(const knx_datapoint_t *dp,
					const knx_datapoint_value_t *stored_value,
					const knx_datapoint_value_t *new_value);

/** @brief Look up a datapoint by id (NULL if not found). */
knx_datapoint_t *knx_datapoint_by_id(uint16_t id);

/**
 * @brief Check whether the device still waits for a datapoint's read-on-init response.
 *
 * A commissioned device waits from startup for every datapoint whose group
 * object has the Read on Init (I) flag, until a response arrives or the
 * requests time out.
 */
bool knx_datapoint_init_read_pending(uint16_t id);

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

/** @brief Read a value2Ucount (2-octet unsigned) datapoint value. */
int knx_datapoint_get_u16(uint16_t id, uint16_t *value);

/** @brief Set a value2Ucount (2-octet unsigned) datapoint value. */
int knx_datapoint_set_u16(uint16_t id, uint16_t value);

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
