.. _knx_iot_overview:

KNX and KNX IoT overview
########################

.. contents::
   :local:
   :depth: 2

This section introduces the KNX building-automation standard and the KNX IoT Point API that the |addon| is built on.

What is KNX
***********

KNX is an open, vendor-independent standard for home and building automation. Classic KNX is internationally standardized as ISO/IEC 14543-3 (parts 3-1 through 3-7).
It defines how devices such as switches, sensors, actuators, thermostats, and gateways exchange data to control lighting, shading, heating, ventilation, access, and energy management.
A key property of KNX is *interworking*: certified devices from different manufacturers can be combined in the same installation and configured with a single tool.

Traditionally, KNX devices communicate over dedicated media, most commonly Twisted Pair (TP), but also Powerline (PL), Radio Frequency (RF), and IP (KNXnet/IP).
Regardless of the medium, KNX reuses the same application concepts: devices expose standardized *functional blocks* and *datapoints*, and they exchange values by writing to shared *group addresses*.

For a general introduction to the standard, see `What is KNX`_ and the `KNX Association`_ website.

What is KNX IoT
***************

KNX IoT extends the KNX ecosystem onto native IPv6 networks, so KNX devices can run directly on common Internet technologies such as Thread, Wi-Fi®, and Ethernet, without a dedicated KNX bus.
It is defined by the KNX Association on top of established IETF standards, and it keeps the existing KNX application model so that KNX IoT devices interwork with classic KNX installations and are configured with the same tools.

KNX IoT consists of two complementary APIs:

* **Point API** - A device-to-device interface standardized in the KNX specification chapter 3/10/5.
  It defines how an individual KNX IoT device communicates over IPv6 and is the API that the |addon| implements.
  See `KNX IoT Point API Stack`_.
* **3rd Party API** - A higher-level, gateway-oriented interface (chapter 3/10/4) aimed at external applications and cloud services.
  It is out of scope for the |addon|.

.. _knx_iot_overview_features:

Key characteristics of the KNX IoT Point API
============================================

The Point API maps KNX application data to a RESTful resource model carried over standard IoT protocols.
The most important building blocks are summarized below and described in detail on the following pages.

* **IPv6 transport** - Devices communicate over any IPv6-capable network.
  In the |addon|, the transport is Thread.
  See :ref:`knx_iot_network_topology`.
* **CoAP messaging** - The Constrained Application Protocol (`RFC 7252`_) provides the request/response and observe model used for all interactions, running over UDP.
* **CBOR/JSON payloads** - Application data is encoded primarily as Concise Binary Object Representation (CBOR, `RFC 8949`_), with optional JSON (`RFC 8259`_).
* **Application-layer security** - Object Security for Constrained RESTful Environments (OSCORE, `RFC 8613`_) protects messages end to end, with password-authenticated key establishment using SPAKE2+ (`RFC 9383`_).
  See :ref:`knx_iot_security`.
* **Discovery** - Devices and their resources are found through DNS-SD/mDNS (`RFC 6762`_, `RFC 6763`_) and CoAP resource discovery.
  See :ref:`knx_iot_group_communication`.
* **S-Mode group communication** - The same group-address-based messaging model as classic KNX, so KNX IoT devices interwork with the rest of a KNX system.
  See :ref:`knx_iot_group_communication`.
* **Reused KNX application model** - The same functional blocks, datapoint types, and ETS-based configuration as other KNX media.
  See :ref:`knx_iot_device_model`.

For the canonical description of these building blocks, see the `KNX IoT documentation`_ and the `KNX IoT Point API Stack`_ introduction.

Typical use cases
*****************

KNX IoT targets the same markets as classic KNX, primarily commercial and residential building automation, but with IP-native, often wireless, devices.
Representative use cases include:

* Lighting control, such as switching and dimming luminaires from wall switches, occupancy sensors, or others.
* Shading and sunblind control.
* Heating, ventilation, and air conditioning (HVAC) control.
* Energy monitoring and load management.
* Integrating new wireless KNX IoT devices into an existing wired KNX installation, reusing the same ETS project and group addresses.

The :ref:`knx_iot_samples` demonstrate the most basic case: a switch (:ref:`sensor <knx_iot_light_switch_sensor_sample>`) controlling a light (:ref:`actuator <knx_iot_light_switch_actuator_sample>`) over Thread.

Further Reading
***************

The remaining pages go in depth into their respective concepts:

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   architecture
   network_topology
   device_model
   group_communication
   commissioning
   security
