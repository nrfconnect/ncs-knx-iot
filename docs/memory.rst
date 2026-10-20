.. _knx_iot_memory:

Memory requirements
###################

.. contents::
   :local:
   :depth: 2

This page provides information about the amount of flash memory and RAM that is required by the :ref:`knx_iot_samples`.
Use it to check if your application has enough space for a given configuration.

.. _knx_iot_memory_requirements_ram_flash:

RAM and flash memory requirements
*********************************

RAM and flash memory requirement values differ depending on the DK and the programmed sample.

The following tables and bar charts list memory requirement values for KNX IoT samples.

Memory layout is taken from the DTS files used by each sample variant, while memory usage is taken from the build output.
Values are provided in kilobytes (KB).

Table columns are grouped by internal NVM, external NVM (when used), and RAM.
Application, MCUboot, upgrade slot, and RAM cells show used and free space separated by ``/``.
Other NVM columns list the reserved partition size for that region.

.. tabs::

   .. group-tab:: Charts

      .. tabs::

         .. group-tab:: nRF54LC10 DK

            Memory requirements for samples running on the `nrf54lc10dk`_.

            .. memory-board::
               :board: nrf54lc10

         .. group-tab:: nRF54L15 DK

            Memory requirements for samples running on the `nrf54l15dk`_.

            .. memory-board::
               :board: nrf54l15

         .. group-tab:: nRF54L15 TAG

            Memory requirements for samples running on the `nrf54l15tag`_.

            .. memory-board::
               :board: nrf54l15tag

         .. group-tab:: nRF54LM20 DK

            Memory requirements for samples running on the `nrf54lm20dk`_.

            .. memory-board::
               :board: nrf54lm20

   .. group-tab:: Tables

      .. tabs::

         .. group-tab:: nRF54LC10 DK

            The following table lists memory requirements for samples running on the `nrf54lc10dk`_.

            .. memory-table::
               :board: nrf54lc10

         .. group-tab:: nRF54L15 DK

            The following table lists memory requirements for samples running on the `nrf54l15dk`_.

            .. memory-table::
               :board: nrf54l15

         .. group-tab:: nRF54L15 TAG

            The following table lists memory requirements for samples running on the `nrf54l15tag`_.

            .. memory-table::
               :board: nrf54l15tag

         .. group-tab:: nRF54LM20 DK

            The following table lists memory requirements for samples running on the `nrf54lm20dk`_.

            .. memory-table::
               :board: nrf54lm20

.. note::

   The ``release`` configurations are built with Link-Time Optimization (LTO) and allocate a fixed-size heap.
   Because of the fixed heap, the RAM usage reported for a ``release`` build can be higher than for the corresponding ``debug`` build.

.. _knx_iot_memory_requirements_layouts:

Reference memory layouts
************************

The following tables and bar charts show how the :ref:`knx_iot_architecture` and :ref:`knx_iot_addon` software stack maps to actual memory partitions on each supported development kit.
The memory values match the :ref:`RAM and flash memory requirements <knx_iot_memory_requirements_ram_flash>` listed above.

Each tab shows the memory maps for the development kits supported by the :ref:`knx_iot_samples`.

.. tabs::

   .. group-tab:: Charts

      .. tabs::

         .. group-tab:: nRF54LC10 DK

            The following memory map is valid for KNX IoT samples running on the `nrf54lc10dk`_.

            .. memory-layout-board::
               :board: nrf54lc10

         .. group-tab:: nRF54L15 DK

            The following memory map is valid for KNX IoT samples running on the `nrf54l15dk`_.

            .. memory-layout-board::
               :board: nrf54l15

         .. group-tab:: nRF54L15 TAG

            The following memory map is valid for KNX IoT samples running on the `nrf54l15tag`_.

            .. memory-layout-board::
               :board: nrf54l15tag

         .. group-tab:: nRF54LM20 DK

            The following memory map is valid for KNX IoT samples running on the `nrf54lm20dk`_.

            .. memory-layout-board::
               :board: nrf54lm20

   .. group-tab:: Tables

      .. tabs::

         .. group-tab:: nRF54LC10 DK

            The following table lists memory partitions for KNX IoT samples running on the `nrf54lc10dk`_.

            .. memory-layout-table::
               :board: nrf54lc10

         .. group-tab:: nRF54L15 DK

            The following table lists memory partitions for KNX IoT samples running on the `nrf54l15dk`_.

            .. memory-layout-table::
               :board: nrf54l15

         .. group-tab:: nRF54L15 TAG

            The following table lists memory partitions for KNX IoT samples running on the `nrf54l15tag`_.

            .. memory-layout-table::
               :board: nrf54l15tag

         .. group-tab:: nRF54LM20 DK

            The following table lists memory partitions for KNX IoT samples running on the `nrf54lm20dk`_.

            .. memory-layout-table::
               :board: nrf54lm20
