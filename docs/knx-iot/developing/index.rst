.. _knx_iot_developing:

Developing with the |addon|
###########################

This section describes how to develop your own KNX IoT application with the |addon|.

If you are new to the |addon|, start with the :ref:`knx_iot_quickstart`.
It guides you through programming two samples, joining them to a Thread network, and commissioning them with ETS.

Use the :ref:`knx_iot_samples` as reference designs for your application.
Each sample defines the following in its own source code:

* Device identity
* Functional blocks
* Datapoints
* Callbacks and handlers that connect them to the application

For everything else, such as running the stack, commissioning, and datapoint persistence, the samples rely on the integration library under :file:`subsys/knx/`.
The board user interface comes from the shared board support under the :file:`subsys/ui/` directory.

For the Kconfig options shared by all samples, see :ref:`knx_iot_config`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages

   commissioning
   integration_library
