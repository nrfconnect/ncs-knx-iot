.. _knx_iot_addon_commissioning:

Commissioning in the |addon|
############################

.. contents::
   :local:
   :depth: 2

A |addon| device is commissioned in two independent steps.
Thread commissioning connects the device to the Thread network, and KNX commissioning makes it a configured member of the KNX installation.
This page describes how the |addon| and its samples implement both steps.
For the standard KNX IoT commissioning flow, see :ref:`knx_iot_commissioning`.

Thread commissioning
********************

A device must be attached to a Thread network before KNX commissioning can start.
By default, the samples use Thread Joiner commissioning.
For step-by-step instructions, see the :ref:`knx_iot_quickstart`.

For development without a Commissioner, the samples provide two options for Thread Out-of-band commissioning enabled by including an additional configuration file:

#. :file:`samples/common/overlay-thread-hardcoded.conf` automatically starts Thread with a predefined network data.
#. :file:`samples/common/overlay-thread-otshell.conf` disables automatic Joiner startup and allows the Thread dataset to be configured manually from the OpenThread shell.

In ETS-based installations, a manufacturer-specific Thread Border Router Device Configuration App can configure the Thread Border Router, the Thread subsystem, and device assignment to the Thread network.

ETS commissioning
*****************

The samples use `ETS tool`_ commissioning by default.
ETS identifies devices by their serial number, so it can find an uncommissioned sample without programming mode.
Each device in the same ETS project needs a unique serial number, which is set with the :option:`CONFIG_KNX_SERIAL_NUMBER` Kconfig option.
Programming mode is not enabled automatically; it can be enabled with the corresponding board button or from the shell.
For step-by-step instructions, see the :ref:`knx_iot_quickstart`.

Hardcoded commissioning
***********************

For development and demonstration without ETS, the |addon| also provides *hardcoded commissioning*, enabled with the :option:`CONFIG_KNX_HARDCODED_COMMISSIONING` Kconfig option.
The samples enable it with the :file:`samples/common/overlay-knx-hardcoded.conf` configuration file.
Combined with :file:`samples/common/overlay-thread-hardcoded.conf`, it lets development kits exchange KNX messages without ETS or a Thread Commissioner.

When enabled, a device applies a fixed commissioning profile on boot instead of receiving it from a MaC.
The profile includes:

* The device's Individual Address.
* The group object table, with one entry for each bound datapoint, and the recipient and publisher tables derived from it.
* A shared group OSCORE key, so devices can exchange protected S-Mode messages without a PASE handshake.

The shared parameters are defined in :file:`samples/common/knx_hardcoded.h` and must be identical on every device that communicates.
They include one group address for each switching channel (``1/1/1`` for the first channel and ``1/1/2`` for the second) and the group OSCORE key.
Each sample defines its group object table in its own preset file, for example :file:`samples/light_switch_sensor/knx_sensor_hardcoded.c`.

.. note::
   Hardcoded commissioning is intended for development and demonstration only.
   It is not a substitute for standard, secure commissioning in a real installation.

Device identity in hardcoded setups
===================================

Every device in a hardcoded setup must have a unique serial number, because the serial number forms the device's DNS-SD hostname and the OSCORE ID Context of its group messages.
Receivers keep a separate security context and replay window for each sender of the shared group key.

The ``CONFIG_KNX_HARDCODED_IDENTITY`` Kconfig choice selects where the device takes its serial number and Individual Address from:

* :option:`CONFIG_KNX_HARDCODED_IDENTITY_HWINFO` (default) - The device keeps the manufacturer ID (the first four characters) of :option:`CONFIG_KNX_SERIAL_NUMBER` and derives the rest of the serial number and its Individual Address from the hardware device ID.
  You can flash the same image to any number of devices with the same role, for example several sensors controlling one actuator.
* :option:`CONFIG_KNX_HARDCODED_IDENTITY_FIXED` - The device uses :option:`CONFIG_KNX_SERIAL_NUMBER` and the Individual Address from its hardcoded preset.
  Each device in the setup needs its own build with a unique serial number.
