.. _knx_iot_integration_library:

Integration library
###################

.. contents::
   :local:
   :depth: 2

The integration library under :file:`subsys/knx/` directory adapts the KNX IoT Point API stack to the |NCS| platform and provides the features that applications use to build a KNX IoT device.
This page describes these features.
For an overview of the library source files and how the library fits into the |addon|, see :ref:`knx_iot_addon`.

Device identity
***************

The application describes its device with a ``knx_device_t`` structure, which includes the device identity, the application parameters, the functional blocks with their datapoints, and the optional callbacks.
The samples set the KNX serial number from the :option:`CONFIG_KNX_SERIAL_NUMBER` Kconfig option.
The serial number consists of 12 lowercase hexadecimal characters and starts with the manufacturer ID.
It is also included in the onboarding QR code and the DNS-SD hostname of the device.
With hardcoded commissioning, the device can derive its serial number from the hardware device ID instead (see :ref:`knx_iot_addon_commissioning`).

.. _knx_iot_datapoint_persistence:

Datapoint persistence
*********************

A datapoint value is kept in RAM only, unless the datapoint has a persistence policy set in the ``persist`` member of its definition.
For a persisted datapoint, the integration library performs the following actions:

* Restores the stored value at boot, before the application ``on_init`` callback runs.
  If the datapoint mirrors its value onto another datapoint, the mirror target is restored as well.
* Calls the policy on every value change.
  The policy compares the stored value with the new one and decides whether the new value is written to non-volatile storage.
* Keeps the stored value over a KNX restart, and erases it during a KNX factory reset.
  After a factory reset, the datapoint returns to its default value and the application ``on_write`` callback is called.

The ``knx_persist_always()`` policy stores every change and is suitable for datapoints that change rarely, such as the light state of the :ref:`knx_iot_light_switch_actuator_sample` sample.
Values are stored through the Zephyr settings subsystem.

.. _knx_iot_read_on_init:

Read on init
************

When a commissioned device starts, it sends a group read request for every datapoint whose group object has the Read on Init (I) flag set (see :ref:`knx_iot_group_communication`).
The requests are sent after the device attaches to the network, one every :option:`CONFIG_KNX_READ_ON_INIT_DELAY_MS` milliseconds.

The integration library tracks each of these datapoints until a response arrives.
If some responses are still missing after the :option:`CONFIG_KNX_READ_ON_INIT_RESPONSE_TIMEOUT_MS` timeout, it sends the requests again, up to :option:`CONFIG_KNX_READ_ON_INIT_RETRIES` times.
When the device stops waiting for a datapoint, the library calls the application ``on_init_read`` callback and reports whether a value was received.
While a datapoint is still waiting, ``knx_datapoint_init_read_pending()`` returns ``true``, so the application can show that the state is not known yet.

The stack applies a response only if the group object also has the Update (U) and Write (W) flags set.
Otherwise, the response is ignored, the requests are resent until the retries run out, and ``on_init_read`` reports that no value was received.

Board user interface
********************

The board support under :file:`subsys/ui/` is built when :kconfig:option:`CONFIG_DK_LIBRARY` is enabled.
It maps the device state to the LEDs and the button presses to Thread, KNX, and application actions.
The mapping is described by a layout table, selected with the ``CONFIG_KNX_UI_LAYOUT`` Kconfig choice:

* :option:`CONFIG_KNX_UI_LAYOUT_DK` - The default layout for development kits with four LEDs and four buttons.
  It uses dedicated status LEDs and buttons for Thread and KNX, and two application buttons and LEDs.
* :option:`CONFIG_KNX_UI_LAYOUT_SINGLE_BUTTON_RGB` - The default layout for the nRF54L15 TAG.
  One RGB LED shows the current commissioning stage with different colors, and one button performs the action relevant to that stage.
  The devicetree must provide the ``knx-ui-rgb-led`` alias pointing to a ``leds-group-multicolor`` node, and the ``sw0`` alias for the button.

The application LEDs can show the off, on, blink, and flash modes.
Application LEDs that the selected layout does not provide are ignored.
To support another board user interface, add a layout source file in :file:`subsys/ui/` and an entry for it in the ``CONFIG_KNX_UI_LAYOUT`` Kconfig choice.
