.. _knx_iot_light_switch_actuator_sample:

KNX IoT: Light Switch Actuator
##############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a KNX IoT light switch actuator built on top of the |addon| for the |NCS|.
The sample implements the KNX **Light Switching Actuator Basic** (LSAB) functional block and drives two loads (shown on LEDs) in response to switch on/off S-mode messages received over a Thread network using KNX IoT group communication.

When used with the :ref:`knx_iot_light_switch_sensor_sample` sample running on a second development kit, the actuator turns its loads on and off following the switch states transmitted by the sensor.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

To test the communication with the light sensor, a second development kit running the :ref:`knx_iot_light_switch_sensor_sample` sample is required.

Overview
********

The sample acts as a KNX IoT actuator device.
In this context, the light switch actuator is the device that changes a light state in response to commands from another device, such as a relay, dimmer, smart light bulb, or another output controller.
It registers a single device profile that exposes the LSAB functional block (number ``417``) on two switching channels.
Both channels can be configured through ETS.
The hardcoded development profile configures only the first channel.
Each channel provides the following datapoints:

* Switch on/off (``soo``) - The control input datapoint (``GET`` and ``PUT``, ``if.i``).
  The actuator receives group writes on this datapoint and mirrors the new value onto the status output.
* Info on/off (``ioo``) - The status output datapoint (``GET``, ``if.o``).
  The actuator announces this value to report its current state back to the group.

When the actuator receives an S-mode multicast write on the ``soo`` datapoint of a channel, it drives its output, mirrors the value to the ``ioo`` datapoint, and announces the new status.
The switched loads of the two channels are represented by LED 2 and LED 3.

Commissioning
=============

The default configuration uses the OpenThread Joiner and ETS6 commissioning.
See :ref:`knx_iot_ets_commissioning` for the complete procedure.

For a local two-board test without ETS or a Thread Commissioner, build both samples with the hardcoded KNX and Thread profiles:

.. code-block:: console

   west build -b board_target -- -DEXTRA_CONF_FILE="../common/overlay-knx-hardcoded.conf;../common/overlay-thread-hardcoded.conf"

The hardcoded profiles are intended for development and demonstration only.

User interface
**************

LED 0:
   Indicates the Thread network status.
   The LED blinks while the device is attaching and remains on when it is attached.

LED 1:
   Indicates the KNX status.
   The LED blinks in programming mode and remains on after commissioning.

LED 2 and LED 3:
   Indicate the switched load state of the first and second channels.

Button 0:
   Short press starts or retries the Thread Joiner.
   Hold for six seconds to perform a Thread factory reset.

Button 1:
   Short press toggles KNX programming mode when attached to a Thread network.
   Hold for six seconds to perform a KNX factory reset.

Configuration
*************

|config|

The following Kconfig options are most relevant for this sample:

* :option:`CONFIG_KNX_IOT_PASSWORD` - Sets the password used to generate the SPAKE2+ verifier and onboarding QR code.
* :option:`CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART` - Enables programming mode temporarily after an uncommissioned device joins Thread.
* :option:`CONFIG_KNX_HARDCODED_COMMISSIONING` - Applies the development-only hardcoded KNX profile.

The default Thread profile starts the Joiner automatically with PSKd ``N0RD1C``.
Use :file:`../common/overlay-thread-hardcoded.conf` for a fixed Thread dataset or :file:`../common/overlay-thread-otshell.conf` to configure and start Thread manually.

Building and running
********************

Before you start, make sure you have set up the |addon| as described in the :ref:`knx_iot_setup` page.

You can build using either the |nRFVSC| or the command line.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      1. Open the sample folder in VS Code with the nRF Connect extension installed.
      2. Select a supported board target.
      3. Optionally add ``-DFILE_SUFFIX=release`` to :guilabel:`Extra CMake arguments` in your build configuration to select a power-optimized build without logs.
      4. Build and flash the sample.

   .. group-tab:: Command line

      To build the sample from the command line, run the following command from the sample directory, with *board_target* replaced by one of the supported board targets:

      .. code-block:: console

         west build -b board_target

      For example, to build for the nRF54L15 DK, run the following command:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp

      For a power-optimized release build without logging and shell support, add the ``release`` file suffix:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp -- -DFILE_SUFFIX=release

      After building, program the sample to the development kit:

      .. code-block:: console

         west flash --erase

To enable verbose KNX stack logging for debugging, add :file:`../common/verbose_logging.conf` as an additional Kconfig fragment.

Testing
*******

After commissioning both devices with ETS, or building both with the hardcoded profiles, test the samples as follows:

#. Wait until **LED 0** and **LED 1** are steadily lit on both kits.
#. Press **Button 2** on the sensor kit to test the first channel.

   The sensor toggles its switch datapoint and transmits it.

#. Observe that **LED 2** on the actuator kit lights up.
#. Press **Button 2** on the sensor kit again.
#. Observe that **LED 2** on the actuator kit turns off.

The hardcoded profile supports this first-channel test.
When using ETS, you can optionally configure and test the second channel with
**Button 3** on the sensor kit and **LED 3** on the actuator kit.

Dependencies
************

This sample uses the KNX IoT stack provided by the |addon|, together with the following |NCS| components:

* `DK Buttons and LEDs`_

It also uses the following Zephyr libraries:

* `Logging`_
* `Kernel API`_
