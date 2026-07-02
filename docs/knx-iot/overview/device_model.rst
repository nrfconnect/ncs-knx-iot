.. _knx_iot_device_model:

Device and data model
#####################

.. contents::
   :local:
   :depth: 2

KNX IoT reuses the KNX application model: the same functional blocks, datapoints, and datapoint types used on every other KNX medium.
The Point API expresses this model as a set of RESTful resources.
For more information, see the `KNX IoT device profile`_.

Functional blocks
*****************

A *functional block* is a standardized description of a single device function, defining which datapoints it has and how they behave.
Functional blocks are what make KNX devices from different manufacturers interwork: a device that implements a given functional block behaves in a predictable, interoperable way.
Each functional block type has a number defined by KNX.

The :ref:`knx_iot_samples` implement two basic lighting functional blocks:

* **Light Switching Actuator Basic (LSAB)**, number ``417`` - The actuator side, which drives a load in response to switch commands.
* **Light Switching Sensor Basic (LSSB)**, number ``421`` - The sensor side, which produces switch commands.

A device exposes its functional blocks as resources and groups them under a *device profile*.

Datapoints
**********

A *datapoint* is a single readable and/or writable value of a functional block, such as a switch state, a brightness level, or a status flag.
Datapoints are the actual application data that devices read, write, and exchange.

For example in the :ref:`knx_iot_samples`, each switching channel exposes two datapoints:

* **Switch on/off (soo)** - The on/off control value.
  On the sensor it is an output the device produces; on the actuator it is an input the device receives.
* **Info on/off (ioo)** - The status value reported back, so the rest of the system can see the actual state.

Datapoint direction is expressed with *interface* qualifiers, for example:

* ``if.o`` - output (the device produces and announces the value).
* ``if.i`` - input (the device receives and consumes the value).

Datapoint types (DPTs)
======================

Every datapoint has a *datapoint type* (DPT) that defines its exact data format, range, and unit, for example a 1-bit boolean for on/off, or a scaled percentage for a dimming level.
DPTs are the contract that guarantees a value written by one device is interpreted identically by another.
The catalog of standardized datapoint types and functional blocks is part of the KNX specifications and the KNX Information Model; see `KNX IoT downloads and schemas`_.

Group object bindings
=====================

Group objects bind a datapoint to one or more :ref:`KNX group addresses <knx_iot_addressing>`, so that writing the datapoint sends to the group and receiving from the group updates the datapoint.
How group objects are configured and used at runtime is covered in :ref:`knx_iot_group_communication` and :ref:`knx_iot_commissioning`.

Resource model
**************

The Point API maps everything above onto CoAP resources identified by URIs.
A client interacts with a device by issuing CoAP methods on these resources, or by subscribing with CoAP *Observe*.
The most relevant resource groups are listed below; for the complete list and their mandatory or optional status, see the `KNX IoT device profile`_.

.. list-table:: Key KNX IoT resources
   :header-rows: 1
   :widths: 25 75

   * - Resource
     - Purpose
   * - ``/.well-known/core``
     - CoAP resource discovery; returns links to the resources a device hosts, filterable by serial number, resource type, or group address.
   * - ``/.well-known/knx``
     - Device-level operations used during commissioning, such as reset, restart, setting the Individual Address, and the SPAKE2+ handshake.
   * - ``/f``
     - Functional block resources hosted by the device.
   * - ``/p``
     - Point (datapoint) resources, used for read, write, and subscribe access to values and their metadata.
   * - ``/fp/g``
     - Group Object Table, which maps datapoints to group addresses.
   * - ``/fp/r``
     - Recipient Table, which lists the destinations for outgoing group messages.
   * - ``/fp/p``
     - Publisher Table, which lists the expected sources of incoming group messages.
   * - ``/k``
     - Group communication endpoint, to which S-Mode group notifications are posted and where subscribers observe.
   * - ``/auth/at``
     - Access-token list (credentials and permission scopes) used for access control.
   * - ``/auth/crts``
     - Certificate trust list used for the optional (D)TLS-based access control.

The function point tables (``/fp/g``, ``/fp/r``, ``/fp/p``) and the ``/k`` endpoint are central to group communication and are described in :ref:`knx_iot_group_communication`.
The access-token resources are described in :ref:`knx_iot_security`.
