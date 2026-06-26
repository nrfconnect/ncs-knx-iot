.. _knx_iot_commissioning:

Commissioning
#############

.. contents::
   :local:
   :depth: 2

*Commissioning* is the process of turning a factory-fresh device into a configured member of a KNX installation: giving it an identity, telling it which groups it belongs to, and installing the security material it needs.
This page explains the standard KNX IoT commissioning flow and how the |addon| currently provides a simplified alternative for development.
For more information, see `KNX IoT device bootstrapping`_.

The Management and Commissioning Client
***************************************

Commissioning is driven by a Management and Commissioning Client (MaC), the tool that discovers devices, assigns addresses, downloads configuration, and installs credentials.
In a standard KNX workflow this role is performed by `ETS6 tool`_, the same engineering tool used for every KNX medium.
ETS holds the project: the list of devices, their group addresses, and the bindings between them.

.. note::
   Reusing ETS and the same project model is a core KNX IoT benefit: an installer can add IP-native KNX IoT devices to an existing installation without learning a new tool or re-planning group addresses.

Standard commissioning flow
***************************

Standard KNX IoT commissioning runs over OSCORE-secured CoAP and is usually split into two parts, as described in `KNX IoT device bootstrapping`_.

Part 1: Individualization
=========================

The MaC authenticates the device and gives it an identity:

#. **Discovery** - The installer puts the device into programming mode (typically with a physical button), and the MaC locates it by serial number or Individual Address through discovery (see :ref:`knx_iot_group_communication`).
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

Hardcoded commissioning in the |addon|
**************************************

The |addon| does not yet integrate with ETS or a full MaC.
Instead, it provides *hardcoded commissioning* for development and demonstration, enabled with the :option:`CONFIG_KNX_HARDCODED_COMMISSIONING` Kconfig option.

When enabled, a device applies a fixed commissioning profile on boot instead of receiving it from a MaC.
The profile includes:

* The device's Individual Address.
* The group object, recipient, and publisher tables for a single shared group.
* A shared group OSCORE key, so devices can exchange protected S-Mode messages without a PASE handshake.

In the samples, these shared parameters (group address ``1/1/1`` and the group OSCORE key) are defined in :file:`samples/common/knx_hardcoded.h` and must be identical on every device that communicates.
This lets two development kits talk to each other directly, without `ETS6 tool`_ or a Thread commissioner.

.. note::
   Hardcoded commissioning is intended for development and demonstration only.
   It is not a substitute for standard, secure commissioning in a real installation.

The Thread network itself is also fixed for the samples: a hardcoded Thread dataset is provided through :file:`samples/common/thread_hardcoded.conf`, so every device built with it forms or joins the same Thread network on boot without a Thread commissioner.
See the :ref:`knx_iot_samples` for how this is used in practice.

A more flexible approach is standard Thread commissioning, where a Joiner is authorized by a Commissioner and receives the active operational dataset for the Thread network.
In ETS-based installations, a manufacturer-specific Thread Border Router Device Configuration App configures the Thread Border Router, the Thread subsystem, and device assignment to the Thread network.
This is not yet supported by the |addon|.
