.. _knx_iot_quickstart:

Quick start guide with ETS
##########################

.. contents::
   :local:
   :depth: 2

This guide demonstrates the basic KNX IoT workflow using the |addon| for the |NCS| and the `ETS tool`_.
It guides you through programming the light switch samples, joining them to a Thread network, commissioning them with ETS, and testing the communication between them.

Overview
********

As part of this guide, you are going to program two KNX IoT samples onto two development kits, attach both devices to a Thread network, and use ETS to link them so that one device controls the other.

Each sample uses its default configuration, which enables the OpenThread Joiner and ETS commissioning, and has a clearly defined role:

* :ref:`knx_iot_light_switch_sensor_sample` implements the Light Switching Sensor Basic (LSSB) functional block and transmits switch on/off messages when you press its buttons.
* :ref:`knx_iot_light_switch_actuator_sample` implements the Light Switching Actuator Basic (LSAB) functional block and switches its loads, shown on LEDs, in response to messages from the sensor.

ETS acts as the Management and Commissioning Client and reaches the devices through a Thread Border Router.
For an overview of the protocol flow, see :ref:`knx_iot_commissioning`.

Requirements
************

Before you start, make sure you have set up the |addon| as described in :ref:`knx_iot_setup`.

You also need:

* Two supported development kits, one for each sample.
* `ETS tool`_ version 6.4 or later, with compatible product entries for the light switch actuator and sensor.
  You can find these entries in the ETS :guilabel:`Catalog` under :guilabel:`KNX Association` -> :guilabel:`KNX Virtual` -> :guilabel:`IoT Demo Sensor/Actuator`, or download them from the `KNX IoT virtual LSxB product entry`_.
* A Thread Border Router that makes the Thread network reachable from the ETS host.
  This guide uses an OpenThread Border Router (OTBR), but another Border Router can be used.

The product entry uses manufacturer code ``0x00FA``, which is assigned to the KNX Association.
The samples use this code for demonstration purposes.
The following serial numbers identify the devices:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Sample
     - Serial number
   * - Actuator
     - ``00fa10020900``
   * - Sensor
     - ``00fa10020700``

Program the samples
*******************

To program the samples, complete the following steps:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      1. Open the :file:`samples/light_switch_actuator` folder in VS Code with the nRF Connect extension installed.
      #. Create a build configuration for a supported board target, for example ``nrf54l15dk/nrf54l15/cpuapp``.
      #. Build the sample and flash it to the first development kit.
      #. Repeat the previous steps for the :file:`samples/light_switch_sensor` folder and flash the sensor sample to the second development kit.

   .. group-tab:: Command line

      From the |addon| directory, build and flash the default configuration of each sample:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp -d build/actuator samples/light_switch_actuator
         west flash -d build/actuator --erase

         west build -b nrf54l15dk/nrf54l15/cpuapp -d build/sensor samples/light_switch_sensor
         west flash -d build/sensor --erase


Join the Thread network
***********************

The samples start the Joiner automatically with PSKd ``N0RD1C``.
The following steps use OTBR and its ``ot-ctl`` interface.
If you use another Border Router, follow its equivalent commissioning procedure and then verify the attachment.

Complete the following steps for each device:

#. Optionally, retrieve the device EUI-64 by running the following command in the device shell.
   If the shell is disabled (for example, in the release configuration), skip this step and authorize the Joiner using the wildcard method described later.

   .. code-block:: console

      ot eui64

#. On the OTBR host, start the Commissioner and authorize the Joiner:

   .. code-block:: console

      ot-ctl commissioner start
      ot-ctl commissioner joiner add <EUI64> N0RD1C

   It is also possible to authorize the Joiner without specifying its EUI-64.
   Just make sure only one device is trying to join the network in this scenario.

   .. code-block:: console

      ot-ctl commissioner joiner add * N0RD1C

#. Wait until **LED 0** remains on, indicating that the device has attached to the Thread network.
   If the device does not attach, press **Button 0** to retry the Joiner.

If the Commissioner authorization expires before the device joins, repeat the authorization and retry the Joiner.

.. note::

   ETS requires the samples to be attached to a Thread network that is reachable from the ETS host.
   Thread commissioning is not required if you provision the Thread Operational Dataset manually.
   For the available Thread configuration overlays, see the Configuration section of the sample pages.

Commission the devices with ETS
*******************************

After both devices have attached to the Thread network, complete the following steps:

#. Create an ETS project and add an IoT line.
#. Add the actuator and sensor product entries to the line.
#. Read each device's onboarding QR string from its shell:

   .. code-block:: console

      knx_iot qr_code

#. Enter the corresponding QR string in ETS when prompted for the device certificate.
#. Link the switch and status group objects of the sensor and actuator.
#. In the ETS topology view, right-click each device and select :guilabel:`Download` > :guilabel:`Download All`.
#. Press **Button 1** to enable programming mode or select an option to program the device via the serial number.
   **LED 1** blinks while programming mode is active.
   You can also enable programming mode from the shell with ``knx_iot pm 1``.

After commissioning, **LED 1** remains on.

Test the network
****************

After commissioning both devices, test the interaction between them:

1. Press **Button 2** on the development kit programmed with the sensor sample to turn on the actuator's **LED 2**.
#. Press **Button 2** on the sensor again to turn off the actuator's **LED 2**.
#. If you linked the second channel in ETS, repeat the previous steps with **Button 3** on the sensor and **LED 3** on the actuator.

Next steps
**********

After you complete this quick start guide, we recommend that you get familiar with the following topics:

* If you want to learn more about the KNX IoT protocol:

  * :ref:`knx_iot_overview`
  * :ref:`knx_iot_commissioning`
  * :ref:`knx_iot_group_communication`
  * :ref:`knx_iot_security`

* If you want to start configuring the samples:

  * Alternative Thread and KNX commissioning profiles, and the power-optimized release build - see the :ref:`knx_iot_light_switch_actuator_sample` and :ref:`knx_iot_light_switch_sensor_sample` pages.
  * :ref:`knx_iot_config`

* If you want to learn how the |addon| integrates the KNX IoT stack, see :ref:`knx_iot_addon`.
