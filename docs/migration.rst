.. _knx_iot_migration:

|addon| migration guide
#######################

.. contents::
   :local:
   :depth: 3

This page describes the changes required to migrate your application between |addon| versions.

Updating from |addon| v0.1.0
****************************

Stack API changes
=================

Applications that call the `KNX IoT Point API Stack`_ directly must review the following changes:

* ``oc_core_set_device()`` was replaced by individual identity and version setters.
  Version setters now take a pointer to an ``oc_knx_version_info_t`` structure.
* ``oc_core_set_device_res()`` must be called explicitly after the basic device identity is set and before the remaining device setup.
  It initializes the KNX resources and connectivity.
* ``app_get_precalculated_spake_data()`` is required for the v1.1.0 SPAKE2+ verifier.
  A generated record must match the password, salt, and iteration count used by the application.
* ``oc_device_info_t.serialnumber`` is a ``const char *`` rather than an ``oc_string_t``.
  Remove ``oc_string()`` wrappers around it.
* Include port-layer headers such as ``oc_clock.h``, ``oc_connectivity.h``, ``oc_storage.h``, and ``dns-sd.h`` with the ``port/`` prefix.
* ``knx_publish_service()`` was renamed to ``knx_dns_sd_update_service()``.

Kconfig changes
===============

Remove legacy stack settings that are no longer exposed:

* ``KNXIOT_SPAKE2P_PSA``
* ``KNXIOT_SPAKE2P_MBEDTLS``
* ``KNXIOT_LIB_DEMO_APPS``
* ``LIB_KNX_LOG_LEVEL``
* ``OC_PRINT_ENABLED``
* ``OC_USE_KNX_PORT_LAYER_ENABLED``
* ``OC_REPLAY_PROTECTION_ENABLED``
* ``KNX_LOG_TO_ZEPHYR``

Also remove ``MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS`` from application configuration files.
The SPAKE2+ implementation now handles the required Mbed TLS declarations internally.

For logging, use ``KNXIOT_DEBUG`` to compile the stack's debug code and ``KNXIOT_LOG_LEVEL`` to select the stack log level.
``KNX_THREAD_STACK_SIZE`` remains available and now controls the add-on's stack event-loop thread.

Add-on API changes
==================

Datapoint method flags
----------------------

The ``methods`` member of ``knx_datapoint_t`` now uses add-on flags:

.. code-block:: c

   .methods = KNX_DP_GET | KNX_DP_PUT,

Replace old uses of ``OC_GET`` and ``OC_PUT`` in datapoint definitions with ``KNX_DP_GET`` and ``KNX_DP_PUT``.

Serial number
-------------

Set the KNX serial number of the device with the :option:`CONFIG_KNX_SERIAL_NUMBER` Kconfig option.
The option has no default value, and the build fails if it does not have exactly 12 hexadecimal characters.
Replace any hardcoded serial number in your application with the new option:

.. code-block:: c

   const char sn_lower_case[] = CONFIG_KNX_SERIAL_NUMBER;

Hardcoded commissioning presets
-------------------------------

The ``preset`` member of ``knx_device_t`` was removed.
With :option:`CONFIG_KNX_HARDCODED_COMMISSIONING`, the application must define the ``knx_hardcoded_preset`` global variable instead.

For example, define it in a source file that is built only when the option is enabled:

.. code-block:: c

   const knx_preset_t knx_hardcoded_preset = {
           .fid = KNX_HC_FID,
           .iid = KNX_HC_IID,
           .grpid = KNX_HC_GRPID,
           .ia = 0x1102,
           .group_objects = sensor_group_objects,
           .num_group_objects = ARRAY_SIZE(sensor_group_objects),
           /* Group OSCORE key material */
   };

The ``knx_preset_t`` structure now describes a complete group object table:

* The ``ga``, ``got_href``, ``got_cflags``, and ``is_publisher`` members were replaced by the ``group_objects`` array of ``struct knx_preset_group_object`` entries and its ``num_group_objects`` length.
  Each entry binds one resource to one group address with its communication flags.
* The recipient and publisher tables are derived from the communication flags of the entries, so ``is_publisher`` is no longer needed.
* The ``ia`` member is used only with :option:`CONFIG_KNX_HARDCODED_IDENTITY_FIXED`.
  By default, :option:`CONFIG_KNX_HARDCODED_IDENTITY_HWINFO` derives the Individual Address and the serial number from the hardware device ID.

Device callbacks and datapoints
-------------------------------

* The ``on_write`` callback of ``knx_device_t`` is now also called after a KNX factory reset resets a persisted datapoint.
* The new optional ``on_init_read`` callback of ``knx_device_t`` reports when the device stops waiting for the read-on-init response of a datapoint.
* The new optional ``persist`` member of ``knx_datapoint_t`` selects the persistence policy of the datapoint.
  Set it to ``NULL`` to keep the value in RAM only.
* ``knx_datapoint_transmit()`` now returns ``0`` on success, ``-ENOENT`` for an unknown datapoint, or ``-ENOTCONN`` if the datapoint cannot be transmitted, for example because it is not bound to a group object with the Transmit flag.

Board support
-------------

The board user interface is now described by a layout table selected with the ``CONFIG_KNX_UI_LAYOUT`` Kconfig choice.
Applications that use the ``knx_board_*`` API do not need any changes.
Applications that use the lower-level button and LED API must apply the following changes:

* ``knx_buttons_set_application_button_callback()`` and ``knx_buttons_set_system_buttons()`` were removed.
  Pass a ``struct knx_buttons_config`` with the gesture callbacks to ``knx_buttons_init()`` instead.
* ``knx_leds_override()`` takes a bit mask of the LEDs that show the override mode.
  All other LEDs are switched off.
* ``KNX_LEDS_COUNT`` and ``KNX_BUTTONS_COUNT`` are derived from the ``/leds`` and ``/buttons`` devicetree nodes.

Programming mode and persisted state
------------------------------------

Programming mode is runtime-only in the v1.1.0 integration and does not survive a restart.
Applications must not rely on the stack restoring it from storage.
