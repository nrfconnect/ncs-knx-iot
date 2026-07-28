.. _release_notes_v010:

Release notes for |addon| v0.1.0
################################

|addon| v0.1.0 is the first release of the KNX IoT add-on for the |NCS|.
It integrates the KNX IoT Point API stack (v1.0.0) and enables development of KNX IoT applications over Thread on nRF54L Series devices.

This is a first `experimental <Software maturity levels_>`_ release that includes Light Switch Sensor and Light Switch Actuator samples, hardcoded commissioning for lab use, and documentation covering the protocol, add-on architecture, and setup.

.. contents::
   :local:
   :depth: 2

This |addon| release is compatible with |NCS| v\ |ncs_version|.

For the list of current issues, see :ref:`knx_iot_known_issues`.

Changelog
*********

Added:

   * The :ref:`knx_iot_light_switch_actuator_sample` sample.
   * The :ref:`knx_iot_light_switch_sensor_sample` sample.
   * Integration of the KNX IoT Point API stack (version 1.0.0).
   * Support for the nRF54L15 and nRF54LM20A/B SoCs.
   * Support for KNX commissioning using hardcoded credentials.
