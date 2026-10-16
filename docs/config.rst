.. _knx_iot_config:

Shared configurations in |addon|
################################

The |addon| provides a set of Kconfig options that are shared across all of its samples.

Use these options to enable the KNX IoT stack, set the device identity, control commissioning behavior, select the board user interface layout, and tune the runtime and logging of the shared KNX integration library.

Configuration options
*********************

.. options-from-kconfig:: /../subsys/Kconfig
   :show-type:

KNX IoT stack configuration options
***********************************

The KNX IoT stack provides the following Kconfig options for tuning its protocol support, table sizes, networking, security, and logging.

.. options-from-kconfig:: /../../modules/lib/sdk-knx-iot-stack/zephyr/Kconfig
   :show-type:
