.. _knx_iot_commissioning:

Commissioning
#############

.. contents::
   :local:
   :depth: 2

*Commissioning* is the process of turning a factory-fresh device into a configured member of a KNX installation: giving it an identity, telling it which groups it belongs to, and installing the security material it needs.
This page explains the standard KNX IoT commissioning flow.
For more information, see `KNX IoT device bootstrapping`_.

.. note::
   For how the |addon| implements commissioning, including Thread commissioning, ETS commissioning, and the development-only hardcoded commissioning, see :ref:`knx_iot_addon_commissioning`.

The Management and Commissioning Client
***************************************

Commissioning is driven by a Management and Commissioning Client (MaC), the tool that discovers devices, assigns addresses, downloads configuration, and installs credentials.
In a standard KNX workflow this role is performed by the `ETS tool`_, the same engineering tool used for every KNX medium.
ETS holds the project: the list of devices, their group addresses, and the bindings between them.

.. note::
   Reusing ETS and the same project model is a core KNX IoT benefit: an installer can add IP-native KNX IoT devices to an existing installation without learning a new tool or re-planning group addresses.

Standard commissioning flow
***************************

Standard KNX IoT commissioning runs over OSCORE-secured CoAP and is usually split into two parts, as described in `KNX IoT device bootstrapping`_.

Before commissioning can start, the device must be connected to an IPv6 network, such as Thread, Wi-Fi, or Ethernet, through which the MaC can discover and reach it.
Connecting the device to that network is not part of KNX IoT commissioning.
On Thread, for example, it is handled by Thread commissioning.

Part 1: Individualization
=========================

The MaC authenticates the device and gives it an identity:

#. **Discovery** - The MaC locates the device by serial number or Individual Address.
   Alternatively, the installer can put a device into programming mode and discover it by that state (see :ref:`knx_iot_group_communication`).
#. **PASE handshake** - The MaC and the device run the password-authenticated SPAKE2+ handshake to establish a temporary secure (OSCORE) session from a shared password. See :ref:`knx_iot_security`.
#. **Tool key installation** - Over that secure session, the MaC installs a long-lived *tool key* access token, which it will use for all further configuration.
#. **Address assignment** - The MaC assigns the device its Individual Address and clears programming mode.

Part 2: Configuration download
==============================

Using the tool-key session, the MaC downloads the actual configuration:

#. **Read Device Information** - Verify the assigned Individual Address and read basic device information, such as the Manufacturer ID and Hardware Type.
#. **Factory reset** - Any previous configuration is cleared.
#. **Load State Machine** - The device is moved through its loading lifecycle so that tables can be written safely.
#. **Tables and parameters** - The MaC writes the function point tables (``/fp/g``, ``/fp/r``, ``/fp/p``; see :ref:`knx_iot_group_communication`) and application parameters.
#. **Security material** - The MaC installs the access tokens and keys the device needs for group communication (see :ref:`knx_iot_security`).
#. **Loaded** - The device transitions to the loaded state and begins normal operation.
