.. _knx_iot_architecture:

KNX IoT architecture
####################

.. contents::
   :local:
   :depth: 2

The KNX IoT Point API is an *application layer* that runs on top of standard IPv6-based transport protocols.
This separation lets KNX application semantics stay the same regardless of the underlying network, while reusing widely available IoT building blocks for transport, encoding, and security.

This page describes the protocol stack from the application down to the radio.
For more details, see the `KNX IoT Point API Stack`_ introduction.

Protocol stack
**************

The following diagram shows the KNX IoT protocol stack as used by the |addon|, where the IPv6 transport is provided by Thread.

.. code-block:: none

   +--------------------------------------------------------------+
   |  KNX application                                             |
   |  Functional blocks, datapoints, datapoint types (DPTs)       |
   +--------------------------------------------------------------+
   |  KNX IoT Point API resource model                            |
   |  REST resources and URIs, S-Mode group communication         |
   +--------------------------------------------------------------+
   |  Data encoding                                               |
   |  CBOR (primary) / JSON, CoRE Link Format for discovery       |
   +--------------------------------------------------------------+
   |  Application messaging          |  Application security      |
   |  CoAP (RFC 7252) over UDP       |  OSCORE (RFC 8613)         |
   |                                 |  PASE / SPAKE2+ (RFC 9383) |
   +--------------------------------------------------------------+
   |  Transport: UDP                                              |
   +--------------------------------------------------------------+
   |  Network: IPv6                                               |
   +--------------------------------------------------------------+
   |  IPv6 link: Thread (IEEE 802.15.4)                           |
   +--------------------------------------------------------------+

.. note::
   Cursor generated placeholder, to be and drawn.

Data flow
=========

On the transmitting device, an application value travels *down* the stack: it is mapped to a Point API resource, encoded as CBOR, wrapped in a CoAP message, protected with OSCORE, and sent in a UDP/IPv6 datagram over the Thread network.
On the receiving device, the payload travels *up* the stack in the reverse order, ending as a value delivered to the application.

Layers
******

The following sections describe each layer of the protocol stack shown above, from the KNX application down to the Thread radio link.

Application
===========

The application layer defines the device behavior in terms of standardized KNX *functional blocks* and *datapoints*, for example a light-switching sensor or a light-switching actuator.
This is the part of the stack that a |addon| application implements.
Functional blocks, datapoints, and datapoint types are described in :ref:`knx_iot_device_model`.

KNX IoT Point API resource model
================================

The Point API exposes the application as a set of RESTful resources, each identified by a URI and accessed with CoAP methods (``GET``, ``PUT``, ``POST``, ``DELETE``) or CoAP *Observe* for subscriptions.
Datapoints, functional blocks, configuration tables, and security material are all represented as resources under standardized paths.
This resource model also provides the basis for KNX *S-Mode* group communication, the message pattern that mirrors classic KNX group telegrams.
See :ref:`knx_iot_device_model` for the resource structure and :ref:`knx_iot_group_communication` for S-Mode communication.

Data encoding
=============

Structured application data is encoded as CBOR (`RFC 8949`_), a compact binary format well suited to constrained devices; JSON (`RFC 8259`_) is an optional alternative.
Resource discovery responses use the CoRE Link Format.
Using a standard encoding is what allows KNX IoT data to be used by non-KNX systems as well.

Application messaging: CoAP
===========================

The Constrained Application Protocol (CoAP, `RFC 7252`_) is a lightweight, RESTful web-transfer protocol designed for constrained nodes and networks.
It provides the request/response semantics, the *Observe* extension used for subscriptions, multicast support used for S-Mode, and resource discovery through ``/.well-known/core``.
CoAP runs over UDP.
The KNX IoT stack provides its own CoAP handling tuned to the Point API.

Application security: OSCORE and SPAKE2+
========================================

Security in KNX IoT is applied at the application layer, so messages stay protected end to end even when forwarded by routers or border routers.
OSCORE (`RFC 8613`_) encrypts and authenticates CoAP payloads using a shared security context, and the initial context is established with Password Authenticated Session Establishment (PASE), using the SPAKE2+ handshake (`RFC 9383`_).
Security is described in detail in :ref:`knx_iot_security`.

Transport and network: UDP/IPv6
===============================

KNX IoT uses UDP over IPv6, with mandatory multicast support at link-local, realm-local, and site-local scopes (multicast is what carries one-to-many S-Mode group messages).
Because the protocol only assumes IPv6, the same device firmware concepts apply on Thread, Wi-Fi, or Ethernet.

IPv6 link: Thread
=================

In the |addon|, the IPv6 link is Thread, a low-power wireless mesh built on IEEE 802.15.4 and maintained by the `Thread Group`_.
The |NCS| provides Thread through the OpenThread stack.
For how Thread fits into the system, see :ref:`knx_iot_network_topology`.
For Thread in the |NCS|, see `OpenThread overview`_ and `Configuring Thread in the nRF Connect SDK`_.
