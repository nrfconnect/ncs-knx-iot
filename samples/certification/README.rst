.. _knx_iot_certification_sample:

KNX IoT: Certification
######################

.. contents::
   :local:
   :depth: 2

This sample provides a KNX IoT device that matches the device expected by the KNX IoT EITT certification test template.
Use it to run the KNX IoT certification tests with EITT on a Nordic Semiconductor device running the |addon| for the |NCS|.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You also need the following:

* EITT with the KNX IoT certification test template.
* A Thread Border Router that makes the Thread network reachable from the EITT host.

Overview
********

The sample acts as a KNX IoT device that combines one Light Switching Actuator Basic (LSAB) functional block (number ``417``) and one Light Switching Sensor Basic (LSSB) functional block (number ``421``).
Both functional blocks use instance ``1``.
The device identity, resource paths, datapoint types, and the application parameter are those expected by the EITT test template:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Resource
     - Datapoint
     - Description
   * - ``/p/1``
     - ``urn:knx:dpa.417.61``
     - LSAB switch on/off, the control input (``GET`` and ``PUT``, ``if.i``).
       A write is mirrored onto ``/p/2``.
   * - ``/p/2``
     - ``urn:knx:dpa.417.62``
     - LSAB info on/off, the status output (``GET``, ``if.o``).
   * - ``/p/3``
     - ``urn:knx:dpa.421.61``
     - LSSB switch on/off, the control output (``GET``, ``if.o``).
   * - ``/p/4``
     - ``urn:knx:dpa.421.62``
     - LSSB info on/off, the status input (``GET`` and ``PUT``, ``if.i``).
   * - ``/p/p1``
     - ``urn:knx:dpa.65500.201``
     - Global test parameter, handled as a signed 32-bit value.

The device uses the following identity:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Property
     - Value
   * - Serial number
     - ``00fa10020800``
   * - Manufacturer ID
     - ``667``
   * - Hardware type
     - ``Windows``
   * - Device model
     - ``KNX Certification``
   * - SPAKE2+ password
     - ``2X4W3TE0DFLLS19Y1FCH``

The device runs as a router-capable Full Thread Device (FTD) with device power management disabled, so it stays reachable during the tests.

The sample does not support software updates or hardcoded KNX commissioning.

User interface
**************

LED 0:
   Indicates the Thread network status.
   The LED blinks while the device is attaching and remains on when it is attached.

LED 1:
   Indicates the KNX status.
   The LED blinks in programming mode and remains on after commissioning.

LED 2:
   Indicates the LSAB switch state (``/p/1``).

LED 3:
   Indicates the LSSB status reported by the group (``/p/4``).

Button 0:
   Short press starts or retries the Thread Joiner.
   Hold for six seconds to perform a Thread factory reset.

Button 1:
   Short press toggles KNX programming mode.
   Use it after the device has attached to a Thread network (**LED 0** steadily lit).
   Hold for six seconds to perform a KNX factory reset.

Button 2:
   Sends the inverse of the LSSB status (``/p/4``) on the LSSB switch output (``/p/3``) to the bound groups.

Configuration
*************

|config|

The following Kconfig options are most relevant for this sample:

* :option:`CONFIG_KNX_SERIAL_NUMBER` - Sets the KNX serial number to the value expected by the EITT test template.
* :option:`CONFIG_KNX_IOT_PASSWORD` - Sets the SPAKE2+ password to the value expected by the EITT test template.

.. include:: /includes/configuration/spake2_credentials.txt

The default Thread profile starts the Joiner automatically with PSKd ``N0RD1C``.
Use :file:`../common/overlay-thread-otshell.conf` to configure and start Thread manually.

Building and running
********************

Before you start, make sure you have set up the |addon| as described in the :ref:`knx_iot_setup` page.

You can build using either the |nRFVSC| or the command line.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      1. Open the sample folder in VS Code with the nRF Connect extension installed.
      2. Select a supported board target.
      3. Build and flash the sample.

   .. group-tab:: Command line

      To build the sample from the command line, run the following command from the sample directory, with *board_target* replaced by one of the supported board targets:

      .. code-block:: console

         west build -b board_target

      For example, to build for the nRF54L15 DK, run the following command:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp

      After building, program the sample to the development kit:

      .. code-block:: console

         west flash --erase

To enable verbose KNX stack logging for debugging, build the sample with the ``knx-verbose-logging`` snippet:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -S knx-verbose-logging

In the |nRFVSC|, add ``-DSNIPPET=knx-verbose-logging`` to :guilabel:`Extra CMake arguments` in your build configuration.

Testing
*******

To prepare the device for the certification tests, complete the following steps:

#. Join the device to the Thread network as described in the :ref:`knx_iot_quickstart`.
#. Wait until **LED 0** remains on.
#. If the test tool asks for the device certificate, read the onboarding QR string from the device shell:

   .. code-block:: console

      knx_iot qr_code

#. Run the KNX IoT certification test template in EITT.

Dependencies
************

This sample uses the KNX IoT stack provided by the |addon|, together with the following |NCS| components:

* `DK Buttons and LEDs`_

It also uses the following Zephyr libraries:

* `Logging`_
* `Kernel API`_
