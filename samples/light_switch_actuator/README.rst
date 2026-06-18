.. _knx_iot_light_switch_actuator_sample:

KNX IoT: Light Switch Actuator
##############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a KNX IoT light switch actuator built on top of the |addon| for the |NCS|.
The sample implements the KNX **Light Switching Actuator Basic** (LSAB) functional block and drives a load (shown on an LED) in response to switch on/off s-mode messages received over a Thread network using KNX IoT group communication.

When paired with the :ref:`knx_iot_light_switch_sensor_sample` sample running on a second development kit, the actuator turns its load on and off following the switch state transmitted by the sensor.

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
Each channel provides the following datapoints:

* Switch on/off (``soo``) - The control input datapoint (``GET`` and ``PUT``, ``if.i``).
  The actuator receives group writes on this datapoint and mirrors the new value onto the status output.
* Info on/off (``ioo``) - The status output datapoint (``GET``, ``if.o``).
  The actuator announces this value to report its current state back to the group.

When the actuator receives an s-mode multicast write on the ``soo`` datapoint of a channel, it drives its output, mirrors the value to the ``ioo`` datapoint, and announces the new status.
The switched load of the first channel is represented by an LED.

Commissioning
=============

The sample currently works only with hardcoded commissioning enabled (the :kconfig:option:`CONFIG_KNX_HARDCODED_COMMISSIONING` Kconfig option).
On boot, the sample applies a fixed KNX commissioning profile, which includes the individual address, the group object and publisher tables, and a shared group OSCORE (Object Security for Constrained RESTful Environments) key.
OSCORE protects KNX IoT messages by encrypting and authenticating them at the application layer.
In this sample, the shared group OSCORE key lets the actuator and sensor exchange protected messages without the `ETS6 tool`_ or a Thread commissioner.

The shared fabric parameters (group address ``1/1/1`` and the group OSCORE key) are defined in the :file:`samples/common/knx_hardcoded.h` file and must be identical on both devices.
The actuator uses the individual address ``1.1.1``.

.. note::
   Hardcoded commissioning is intended for development and demonstration only.

User interface
**************

LED 1:
   Indicates the network and KNX service status.
   The LED turns on when the Thread network is up and the KNX service has been published.

LED 2:
   Indicates the switched load (light) state of the first channel.
   The LED follows the received ``soo`` value: it turns on when the switch state is on, and off when it is off.

Configuration
*************

|config|

The following Kconfig options are most relevant for this sample:

* :kconfig:option:`CONFIG_KNX_HARDCODED_COMMISSIONING` - Applies the hardcoded commissioning profile on boot.

The Thread network configuration is provided through the :file:`samples/common/thread_hardcoded.conf` file.
This file configures a fixed Thread dataset (network name, PAN ID, channel, and network key) so that every device built with it forms or joins the same Thread network on boot, without a Thread commissioner.
In the current state of the sample, this file is required for the device to function correctly.

Building and running
********************

Before you start, make sure you have set up the |addon| as described in the :ref:`knx_iot_setup` page.

To build the sample from the command line, run the following command from the sample directory, with *board_target* replaced by one of the supported board targets:

.. code-block:: console

   west build -b board_target -- -DEXTRA_CONF_FILE=../common/thread_hardcoded.conf

For example, to build for the nRF54L15 DK, run the following command:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE=../common/thread_hardcoded.conf

After building, program the sample to the development kit:

.. code-block:: console

   west flash --erase

Testing
*******

After programming the sample to your development kit, you can test it together with the :ref:`knx_iot_light_switch_sensor_sample` sample running on a second kit.

1. Program the actuator sample to one development kit and the sensor sample to another.
#. Reset both kits.
#. Wait until **LED 1** turns on for both kits, which indicates that the Thread network is up and the KNX service is published.
#. Press **Button 2** on the sensor kit.

   The sensor toggles its switch datapoint and transmits it.

#. Observe that **LED 2** on the actuator kit lights up.
#. Press **Button 2** on the sensor kit again.
#. Observe that **LED 2** on the actuator kit turns off.

Dependencies
************

This sample uses the KNX IoT stack provided by the |addon|, together with the following |NCS| components:

* `DK Buttons and LEDs`_

It also uses the following Zephyr libraries:

* `Logging`_
* `Kernel API`_
