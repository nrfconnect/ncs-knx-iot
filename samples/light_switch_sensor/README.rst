.. _knx_iot_light_switch_sensor_sample:

KNX IoT: Light Switch Sensor
############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a KNX IoT light switch sensor built on top of the |addon| for the |NCS|.
The sample implements the KNX **Light Switching Sensor Basic** (LSSB) functional block and transmits switch on/off S-mode messages over a Thread network using KNX IoT group communication.

When used with the :ref:`knx_iot_light_switch_actuator_sample` sample running on a second development kit, pressing an application button on the sensor toggles the corresponding load on the actuator.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

To test the communication with the light actuator, a second development kit running the :ref:`knx_iot_light_switch_actuator_sample` sample is required.

Overview
********

The sample acts as a KNX IoT sensor device.
In this context, the light switch sensor is the device that detects triggers for changing another device's light state, such as a physical light switch, occupancy sensor, or another input source.
It registers a single device profile that exposes the LSSB functional block (number ``421``) on two switching channels.
Both channels can be configured through ETS or the hardcoded development profile.
Each channel provides the following datapoints:

* Switch on/off (``soo``) - The control output datapoint (``GET``, ``if.o``).
  The sensor owns this value and announces it to the group.
* Info on/off (``ioo``) - The status input datapoint (``GET`` and ``PUT``, ``if.i``).
  It reflects the status reported back by the bound actuator.

When you press **Button 2** or **Button 3**, the sample sends the inverse of the ``ioo`` value of the corresponding channel on its ``soo`` datapoint, as an S-mode multicast write to every device bound to the same group address.
A bound actuator receives the S-mode message, drives its output, and announces its own status, which the sensor stores in its ``ioo`` datapoint.
Because the sensor toggles from the reported light state, a light that was switched by another device toggles correctly on the next press.

The sensor does not store the light state, because the state belongs to the actuator.
If the ``ioo`` group object of a channel has the Read on Init (I) flag set in ETS, the sensor reads the current light state from the group after it attaches to the network.
For more information, see :ref:`knx_iot_read_on_init`.

Commissioning
=============

The default configuration uses the OpenThread Joiner and ETS commissioning.
To join the Thread network and commission the device with ETS, follow the :ref:`knx_iot_quickstart`.

For a local test without ETS or a Thread Commissioner, build both samples with the hardcoded KNX and Thread profiles:

.. code-block:: console

   west build -b board_target -- -DEXTRA_CONF_FILE="../common/overlay-knx-hardcoded.conf;../common/overlay-thread-hardcoded.conf"

The hardcoded KNX profile binds the first channel to group address ``1/1/1`` and the second channel to group address ``1/1/2``.
The ``ioo`` datapoint of each channel also listens to the group address of the channel, so several sensors that control the same actuator stay in sync.
The hardcoded profiles are intended for development and demonstration only.

For more information about the commissioning methods, see :ref:`knx_iot_addon_commissioning`.

User interface
**************

.. tabs::

   .. group-tab:: Development kits

      LED 0:
         Indicates the Thread network status.
         The LED blinks while the device is attaching and remains on when it is attached.

      LED 1:
         Indicates the KNX status.
         The LED blinks in programming mode and remains on after commissioning.

      LED 2 and LED 3:
         Indicate the light state reported for the first and second channels.
         The LED shows a short flash once per second when the light is on, and is off when the light is off.
         The LED blinks while the sensor waits for the read-on-init response of the channel.

      Button 0:
         Short press starts or retries the Thread Joiner.
         Hold for six seconds to perform a Thread factory reset.

      Button 1:
         Short press toggles KNX programming mode.
         Use it after the device has attached to a Thread network (**LED 0** steadily lit).
         Hold for six seconds to perform a KNX factory reset.
         Available only with ETS commissioning (:option:`CONFIG_KNX_ETS_COMMISSIONING`).

      Button 2 and Button 3:
         Send the inverse of the reported light state (``ioo``) of the corresponding channel on the ``soo`` datapoint to the bound groups.

   .. group-tab:: nRF54L15 TAG

      The nRF54L15 TAG has one RGB LED and one button, and uses the :option:`CONFIG_KNX_UI_LAYOUT_SINGLE_BUTTON_RGB` layout.
      The color of the LED shows the current commissioning stage, and the button performs the action relevant to that stage.
      Only the first switching channel is available.

      .. list-table::
         :header-rows: 1
         :widths: auto

         * - Stage
           - RGB LED
           - Short press
           - Long press
         * - Not attached to Thread
           - Red, short flash
           - Start the Thread Joiner
           - --
         * - Attaching to Thread
           - Red, blinking
           - Retry the Thread Joiner
           - --
         * - Attached, KNX not commissioned
           - Blue, solid
           - Toggle KNX programming mode
           - Thread factory reset
         * - Attached, KNX not commissioned, programming mode on
           - Blue, blinking
           - Toggle KNX programming mode
           - Thread factory reset
         * - KNX commissioned
           - Green, short flash
           - Send the inverse of the reported light state (``ioo``) of the first channel on ``soo``
           - KNX factory reset
         * - KNX commissioned, programming mode on
           - Blue, blinking
           - Toggle KNX programming mode
           - KNX factory reset

      A long press means holding the button for six seconds.
      After three seconds, the LED blinks white to indicate that the long-press action is armed.
      Release the button while the LED blinks white to cancel the action.

      With hardcoded KNX commissioning, the device is commissioned as soon as it attaches to the Thread network, and a long press in the commissioned stage performs a Thread factory reset.

Configuration
*************

|config|

The following Kconfig options are most relevant for this sample:

* :option:`CONFIG_KNX_SERIAL_NUMBER` - Sets the KNX serial number of the device (``00fa10020700`` by default for this sample).
* :option:`CONFIG_KNX_PROGRAMMING_MODE_AUTOSTART` - Enables programming mode temporarily after an uncommissioned device joins Thread.
* :option:`CONFIG_KNX_HARDCODED_COMMISSIONING` - Applies the development-only hardcoded KNX profile.

.. include:: /includes/configuration/spake2_credentials.txt

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
      3. Optionally add ``-DEXTRA_CONF_FILE=release.conf -DFILE_SUFFIX=release`` to :guilabel:`Extra CMake arguments` in your build configuration to select a power-optimized build without logs.
      4. Build and flash the sample.

   .. group-tab:: Command line

      To build the sample from the command line, run the following command from the sample directory, with *board_target* replaced by one of the supported board targets:

      .. code-block:: console

         west build -b board_target

      For example, to build for the nRF54L15 DK, run the following command:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp

      For a power-optimized release build without logging and shell support, add the release configuration:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE=release.conf -DFILE_SUFFIX=release

      After building, program the sample to the development kit:

      .. code-block:: console

         west flash --erase

To enable verbose KNX stack logging for debugging, build the sample with the ``knx-verbose-logging`` snippet:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -S knx-verbose-logging

In the |nRFVSC|, add ``-DSNIPPET=knx-verbose-logging`` to :guilabel:`Extra CMake arguments` in your build configuration.

The snippet enables :option:`CONFIG_KNX_VERBOSE_STACK_LOGS`, enlarges the log buffer, and increases the console UART baud rate from the default 115200 to 1000000 to avoid dropping logs.

Building for the nRF54L15 TAG
=============================

The nRF54L15 TAG has no UART, so the sample routes its logs and shell to SEGGER RTT when built for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
To program the TAG and read its RTT output, connect it to the **DEBUG OUT** header of an nRF54L DK and power it as described in the `nrf54l15tag`_ board documentation.

Testing
*******

After commissioning both devices with ETS as described in the :ref:`knx_iot_quickstart`, or building both with the hardcoded profiles, test the samples as follows:

#. Wait until **LED 0** and **LED 1** are steadily lit on both kits.
#. Press **Button 2** on the sensor kit to test the first channel.

   The sensor toggles its switch datapoint and transmits it.

#. Observe that **LED 2** on the actuator kit lights up, and **LED 2** on the sensor kit starts flashing briefly to show the reported light state.
#. Press **Button 2** on the sensor kit again.
#. Observe that **LED 2** on both kits turns off.
#. Repeat the previous steps for the second channel with **Button 3** and **LED 3**.
   When using ETS, the second channel must be linked in the ETS project.
   Otherwise, the sensor logs that the channel is not configured.

If you use the nRF54L15 TAG as the sensor, wait until the RGB LED flashes green, and then press the button to toggle **LED 2** on the actuator kit.

Dependencies
************

This sample uses the KNX IoT stack provided by the |addon|, together with the following |NCS| components:

* `DK Buttons and LEDs`_

It also uses the following Zephyr libraries:

* `Logging`_
* `Kernel API`_
