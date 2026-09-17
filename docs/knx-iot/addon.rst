.. _knx_iot_addon:

|addon| architecture
####################

.. contents::
   :local:
   :depth: 2

This page describes what the |addon| provides and how it integrates the KNX IoT Point API stack into the |NCS|.
For the protocol concepts referenced here, see the :ref:`knx_iot_overview`.

Add-on components
*****************

The |addon| brings the KNX IoT Point API to Nordic Semiconductor devices running the |NCS|.
It consists of the following parts:

* The third-party **KNX IoT Point API stack** (the upstream open-source stack), pulled in as a west module and built for the |NCS|.
  See the `KNX IoT Point API Stack repository`_.
* A small **integration library** under :file:`subsys/knx/` that adapts the stack to the |NCS|, drives its event loop, and provides shared board support.
* **Samples** that demonstrate basic KNX IoT functional blocks over Thread (see the :ref:`knx_iot_samples`).
* This **documentation**.

The |addon| is enabled with the :option:`CONFIG_KNX_IOT_ADD_ON` Kconfig option, which brings in the stack together with the integration library and selects the required dependencies (for example, the PSA Crypto features used by KNX IoT security).
See :ref:`knx_iot_config` for the configuration options.

Component overview
******************

The following diagram shows how a |addon| application is layered on top of the KNX IoT Point API stack, the OpenThread stack, and the |NCS|.

.. graphviz:: images/knx_iot_component_overview.dot
   :alt: KNX IoT add-on application layered on top of the stack and the nRF Connect SDK
   :align: center
   :layout: dot

Integration library
===================

The integration library under :file:`subsys/knx/` allows for easy integration of the KNX stack to user applications.
Its main source files are:

* :file:`knx_app.c`, :file:`knx_stack.c` - Initialize and run the KNX IoT stack, including the event loop.
* :file:`knx_resources.c` - Register the application's KNX resources (functional blocks and datapoints).
* :file:`knx_transport.c` - Adapt the stack to the underlying IPv6/Thread transport.
* :file:`knx_board.c`, :file:`knx_buttons.c`, :file:`knx_leds.c` - Shared board support (status LEDs, button handling), built when :kconfig:option:`CONFIG_DK_LIBRARY` is enabled.
* :file:`knx_presets.c` - The hardcoded commissioning mechanism, built when :option:`CONFIG_KNX_HARDCODED_COMMISSIONING` is enabled (see :ref:`knx_iot_commissioning`).

The stack runs its event loop on a dedicated worker thread whose scheduling priority is set with the :option:`CONFIG_KNX_THREAD_PRIORITY` Kconfig option.

SPAKE2+ credentials
===================

During the build, the |addon| generates a SPAKE2+ verifier from :option:`CONFIG_KNX_IOT_PASSWORD` and compiles it into the application.
The stack uses the verifier for PASE authentication, while the samples retain the password to print their onboarding QR code on demand.

The default password and salt are intended for demonstration only.
Production applications must use device-specific credentials and a suitable provisioning process.
For more information about SPAKE2+, see :ref:`knx_iot_security`.

Relationship to the upstream stack
**********************************

The KNX IoT functionality comes from the upstream Point API stack.
The |addon| provides the |NCS| integration and a device-agnostic application layer under :file:`subsys/knx/`.
For upstream documentation, the protocol specifications, and schemas, see the `KNX IoT documentation`_ and `KNX IoT downloads and schemas`_.

For the current feature coverage and limitations, see :ref:`knx_iot_software_maturity` and :ref:`knx_iot_known_issues`.
