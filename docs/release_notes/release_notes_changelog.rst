.. _release_notes_changelog:

Changelog for |addon| v0.1.99
#############################

The most relevant changes that are present on the main branch of the |addon|, as compared to the latest release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Changelog
*********

Changed:

   * Updated the KNX IoT Point API Stack to version 1.1.0.
     For information about adapting applications, see the :ref:`KNX IoT migration guide <knx_iot_migration>`.
   * Replaced the :file:`samples/common/verbose_logging.conf` Kconfig fragment with the ``knx-verbose-logging`` snippet.
     The snippet also increases the console UART baud rate to 1000000.

Added:

   * ETS commissioning and OpenThread Joiner support as the default configuration of the :ref:`knx_iot_samples`.
     Development configurations, hardcoded KNX, hardcoded Thread and manual Thread bring up via the shell are now optional overlays.
