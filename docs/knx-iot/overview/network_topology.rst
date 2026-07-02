.. _knx_iot_network_topology:

Network topology and infrastructure
###################################

.. contents::
   :local:
   :depth: 2

A KNX IoT installation consists of KNX IoT devices, commissioning and management clients, and the IPv6 network that connects them.
This page first places KNX IoT in the wider KNX ecosystem, then describes the infrastructure the |addon| relies on: the Thread network it runs over, the Thread Border Router that connects that network to other IPv6 networks, and how devices are addressed and discovered.

Typical KNX application topology
********************************

KNX IoT is one part of the wider KNX ecosystem.
A real installation can combine several KNX media (KNX TP, KNX RF, and KNXnet/IP) with KNX IoT devices, joined by couplers, routers, and IP infrastructure.
Understanding this landscape makes it clear which part the |addon| provides: a KNX IoT Point API device running over Thread.

For a visual overview, see the `KNX ecosystem landscape (photo) <https://support.knx.org/hc/article_attachments/11225049984018>`_ published by the KNX Association.

.. note::
   The link above points to a KNX-hosted photo of the ecosystem landscape and is used only as a placeholder.
   Replace it with an original or permission-cleared diagram before release.

The following sections group the components that can appear in a KNX installation by the role they play.

KNX IoT over Thread
===================

These are the components used by a Thread-based KNX IoT installation, including the part provided by the |addon|.

.. list-table:: KNX IoT components
   :header-rows: 1
   :widths: 25 45 30

   * - Component
     - Description
     - When it is required
   * - KNX IoT Point API device (Thread)
     - A KNX IoT end device running the Point API stack; in the |addon| it communicates over Thread. **This is the component the |addon| provides.**
     - Required in any KNX IoT network.
   * - Thread Border Router (IPv6 Border Router)
     - Connects the Thread mesh to a Wi-Fi or Ethernet network, similar to how a Wi-Fi access point connects wireless clients to the LAN.
     - For Thread-based KNX IoT, whenever devices must be reached from outside the mesh (for example, by a commissioning tool).
   * - IP Wi-Fi router / access point
     - Provides the IP backbone (Wi-Fi or Ethernet) that links the border router, tools, and IP-based devices.
     - Whenever an IP backbone network is used, which is almost always the case.

Integration with other systems
==============================

These components connect KNX IoT to classic KNX installations or to external software.

.. list-table:: Integration components
   :header-rows: 1
   :widths: 25 45 30

   * - Component
     - Description
     - When it is required
   * - KNX IoT Router
     - A gateway that translates group communication between KNX IoT and classic KNX (KNX TP, KNX RF, or KNXnet/IP).
     - Only when KNX IoT devices must interwork with classic KNX installations.
   * - KNX IoT API Server
     - Exposes a standardized API, based on the KNX Information Model, for external software.
     - Only to integrate the installation with a third-party application.
   * - KNX IoT API client (application)
     - A third-party application that interacts with the installation through the KNX IoT API Server.
     - Only for third-party integration, it is the counterpart of the API Server.
   * - KNX IP interface / router
     - Connects classic KNX TP installations to IP using KNXnet/IP.
     - Only in installations that bridge classic KNX TP over an IP backbone.

Classic KNX infrastructure
==========================

These components belong to classic KNX TP or KNX RF installations.

.. list-table:: Classic KNX components
   :header-rows: 1
   :widths: 25 45 30

   * - Component
     - Description
     - When it is required
   * - Line or area coupler
     - Couples KNX TP lines and areas on the classic KNX TP backbone.
     - Only in classic KNX TP topologies.
   * - Media coupler (TP/RF)
     - Connects a KNX RF segment to a KNX TP line.
     - Only when mixing KNX TP and KNX RF.
   * - KNX power supply
     - Powers the KNX TP bus cable.
     - Only for KNX TP segments.
   * - KNX TP device
     - A classic KNX device that uses twisted-pair cabling.
     - Only in classic KNX TP installations.
   * - KNX RF device
     - A classic KNX device that uses radio frequency.
     - Only in classic KNX RF installations.
   * - Segment proxy
     - Bridges secure and plain KNX communication on a KNX TP segment.
     - Only in classic KNX TP installations that mix secure and plain devices.

Thread as the IPv6 link
***********************

KNX IoT runs over any IPv6 network, so it is not bound to a specific medium.
The |addon| uses Thread as that IPv6 link, but the same KNX IoT concepts apply equally to other IPv6 media such as Wi-Fi or Ethernet.

Thread is a low-power, self-healing wireless mesh network built on IEEE 802.15.4 radios and IPv6.
It is maintained by the `Thread Group`_, and the |NCS| implements it with the OpenThread stack (see `OpenThread overview`_ and `Configuring Thread in the nRF Connect SDK`_).

Thread devices take one of several roles in the mesh (router, end device, or sleepy end device).
For low-power KNX IoT sensors and actuators, the *sleepy end device* role lets a battery-powered node keep its radio off most of the time and poll its parent router for messages.
The mesh structure and roles are described in the `OpenThread`_ documentation.

A standalone Thread mesh is reachable only from within the mesh.
To let clients on another IPv6 link, such as a commissioning tool on Wi-Fi or Ethernet, reach the KNX IoT devices, the mesh is connected to the rest of the network through a Thread Border Router (TBR).
The Thread Border Router routes IPv6 traffic between the Thread mesh and the adjacent network.
It also makes the (possibly sleepy) Thread devices discoverable from outside the mesh: because flooding the low-power mesh with mDNS multicast would be wasteful, Thread devices register their services with the border router using the unicast Service Registration Protocol (SRP), and the border router re-advertises them with mDNS on the infrastructure link on their behalf.
The discovery mechanisms that build on this are described in :ref:`knx_iot_group_communication`.
For more about this network element, see the `Thread Border Router`_ guide.

.. _knx_iot_addressing:

Addressing
**********

KNX IoT uses different addresses for network routing, device identity, and group communication:

* **IPv6 address** - The network address used to route packets to a device on the Thread mesh.
  It can change, for example after a reboot or a topology change, and it is not meaningful to the KNX application.
* **KNX Individual Address (IA)** - The stable logical identity of a device within a KNX installation, written in the ``area.line.device`` form (for example, ``1.1.1``).
  The IA is assigned during commissioning, remains constant even when the IPv6 address changes, and is used for point-to-point operations such as commissioning and diagnostics.
  An unconfigured device uses the default IA ``15.15.255``.
* **KNX Group Address (GA)** - A logical group of datapoints across devices, written as ``main/middle/sub`` (for example, ``1/1/1``).
  Group communication targets a GA rather than a specific device, which is what allows one sensor to drive many actuators, and many sensors to feed one actuator.
  Internally, KNX IoT also represents group addresses as integers (for example, GA ``1/1/1`` corresponds to ``2305``).

.. note::
   Because the OSCORE security context is tied to the KNX identity rather than the IPv6 address, a device keeps its secure association across IPv6 address changes.
   See :ref:`knx_iot_security`.
