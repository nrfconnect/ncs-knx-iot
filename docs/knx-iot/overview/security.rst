.. _knx_iot_security:

Security
########

.. contents::
   :local:
   :depth: 2

Security in KNX IoT is applied primarily at the application layer, so messages remain protected end to end even when relayed by routers or a border router.
This page introduces the security mechanisms and how access to a device is controlled.
For more information, see `KNX IoT device access control`_ and `KNX IoT device bootstrapping`_.

OSCORE
******

Object Security for Constrained RESTful Environments (OSCORE, `RFC 8613`_) protects CoAP messages by encrypting and authenticating their payload and relevant header fields using a shared security context.
Because protection is bound to the message rather than to a transport connection, OSCORE works for unicast and multicast, survives forwarding by intermediaries, and is well suited to low-latency, constrained communication.

In KNX IoT, OSCORE secures both group communication (S-Mode) and direct access to datapoints and configuration.
An OSCORE security context is identified independently of the device's IPv6 address, so a device keeps its secure association even when its IPv6 address changes (for example, after a reboot).

.. note::
   Group (multicast) S-Mode messages are protected with a *group* OSCORE context shared by all members of the group.
   In the :ref:`knx_iot_samples`, this group key is provided through hardcoded commissioning; see :ref:`knx_iot_commissioning`.

PASE and SPAKE2+
****************

Before two parties can use OSCORE, they need a shared security context.
KNX IoT establishes the first context with Password Authenticated Session Establishment (PASE), built on the SPAKE2+ protocol (`RFC 9383`_).

SPAKE2+ lets a commissioning tool and a device derive a strong shared key from a device password (for example, from a label or QR code) without ever sending the password over the air, and it provides mutual authentication.
This bootstraps the secure session that the Management and Commissioning Client (MaC) uses to install long-lived credentials during commissioning (see :ref:`knx_iot_commissioning`).

Access control
**************

Once a secure context exists, the device still must decide what a given peer is allowed to do.
KNX IoT controls this with *access tokens* stored in the ``/auth/at`` resource, as described in `KNX IoT device access control`_.

Each access token binds a credential to a *scope* that defines which resources or group addresses the peer may access. Typical scopes are:

* **Tool key** - Full device access, used by the MaC during commissioning.
* **Configuration/security scope** - Restricted access, for example to security configuration only.
* **Group access** - Access limited to one or more group addresses, used for S-Mode traffic.

When a request arrives, the device authenticates the peer (by matching its OSCORE context or, for the TLS case, by validating its certificate) and then checks that an access token grants the requested operation.

Optional transport security (TLS)
*********************************

For deployments that require transport-layer encryption or certificate-based identities, KNX IoT optionally supports CoAP over (D)TLS, with trust anchors stored in the ``/auth/crts`` resource and device certificates (LDevID) enrolled during onboarding.

.. note::
   TLS-based access is an optional feature of the KNX IoT specification.
   The |addon| focuses on OSCORE-based security and doesn't support it.
