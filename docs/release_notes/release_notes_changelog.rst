.. _release_notes_changelog:

Changelog for |addon| v0.1.99
#############################

The most relevant changes that are present on the main branch of the |addon|, as compared to the latest release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Changelog
*********

Changed:

   * Updated the KNX IoT Point API Stack to version 1.1.0.
     For information about adapting applications, see the :ref:`KNX IoT migration guide <knx_iot_migration>`.
   * Updated the |NCS| version to v3.4.1.
   * Replaced the :file:`samples/common/verbose_logging.conf` Kconfig fragment with the ``knx-verbose-logging`` snippet.
     The snippet also increases the console UART baud rate to 1000000.
   * Hardcoded commissioning now configures a complete group object table, so the samples can use both switching channels with hardcoded commissioning.
   * By default, devices with hardcoded commissioning derive their serial number and Individual Address from the hardware device ID (:option:`CONFIG_KNX_HARDCODED_IDENTITY_HWINFO`).
     You can flash the same image to several devices with the same role.
   * The board user interface is now described by a layout table selected with the ``CONFIG_KNX_UI_LAYOUT`` Kconfig choice.
   * The :ref:`knx_iot_light_switch_sensor_sample` sample now toggles from the light state reported by the actuator.
   * The |addon| now enables the :kconfig:option:`CONFIG_OPENTHREAD_MLR` Kconfig option by default, so that a Thread Border Router forwards KNX IoT site-local multicast messages from the infrastructure link to the device.

Added:

   * ETS commissioning and OpenThread Joiner support as the default configuration of the :ref:`knx_iot_samples`.
     Development configurations, hardcoded KNX, hardcoded Thread and manual Thread bring up via the shell are now optional overlays.
   * The :option:`CONFIG_KNX_SERIAL_NUMBER` Kconfig option for setting the KNX serial number.
   * Support for the nRF54LC10 DK in the :ref:`knx_iot_samples`.
   * Support for the nRF54L15 TAG in the :ref:`knx_iot_light_switch_sensor_sample` sample, with the single button and RGB LED user interface layout (:option:`CONFIG_KNX_UI_LAYOUT_SINGLE_BUTTON_RGB`).
   * Datapoint persistence.
     The :ref:`knx_iot_light_switch_actuator_sample` sample stores its light state and restores it after a reboot.
   * Read-on-init tracking for group objects with the Read on Init (I) flag, configured with the :option:`CONFIG_KNX_READ_ON_INIT_RESPONSE_TIMEOUT_MS` and :option:`CONFIG_KNX_READ_ON_INIT_RETRIES` Kconfig options.
   * The flash LED mode, which shows a short flash once per second.
   * Support for signed 32-bit (``value4Count``) datapoints.
   * The :ref:`knx_iot_certification_sample` sample for checking KNX IoT compliance with the |EITT| certification tests.

Fixed:

   * The light state of the :ref:`knx_iot_light_switch_actuator_sample` sample not persisting after a reboot (:ref:`KRKNWK-22216 <knx_iot_known_issue_actuator_state_not_persisted>`).
