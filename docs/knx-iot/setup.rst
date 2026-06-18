.. _knx_iot_setup:

Requirements and setup
######################

This page outlines the requirements that you need to meet before you start working with the |addon| and KNX IoT protocol.

Hardware requirements
*********************

The KNX IoT add-on supports the nRF54L15 and nRF54LM20A/B SoCs.

Software requirements
*********************

To work with the |addon|, you need to install the |NCS|, including all its prerequisites and the |NCS| toolchain.
Follow the `Installing the nRF Connect SDK`_ instructions, with the following exception:

.. tabs::

   .. group-tab:: |nRFVSC|

      In the `Get the nRF Connect SDK code <Installing the nRF Connect SDK_>`_ section, click :guilabel:`Create a new application`. 
      Select :guilabel:`Browse nRF Connect SDK Add-on Index`, then choose :guilabel:`KNX IoT`. 
      Select v\ |addon_version| of the |addon|. 
      This step also installs nRF Connect SDK v\ |ncs_version|.

   .. group-tab:: command line

      You can initialize workspace in two alternative ways:

      **Initialize a new workspace:**

      1. In the `Get the nRF Connect SDK code <Installing the nRF Connect SDK_>`_, run the following command to initialize west with |addon|, which also initializes nRF Connect SDK v\ |ncs_version|:

         a. Initialize ``ncs`` for the add-on:

            .. code-block:: console

               west init -m "http://github.com/nrfconnect/ncs-knx-iot"

         #. Update the nRF Connect SDK modules:

            .. code-block:: console

               west update

      **Include add-on in the existing nRF Connect SDK workspace:**

      1. Assuming you have an existing nRF Connect SDK workspace in the :file:`ncs` folder, run the following commands:

         a. Navigate to the workspace folder:

            .. code-block:: console

               cd ncs

         #. Clone the add-on repository:

            .. code-block:: console

               git clone https://github.com/nrfconnect/ncs-knx-iot

         #. Set manifest path to the add-on directory:

            .. code-block:: console

               west config manifest.path ncs-knx-iot

         #. Update the nRF Connect SDK modules:

            .. code-block:: console

               west update
         
      2. Optionally, run these commands in case you need to go back to work on the nRF Connect SDK without the add-on:

         a. Configure the manifest path back to the nRF Connect SDK directory

            .. code-block:: console

               west config manifest.path nrf

         #. Update nRF Connect SDK modules

            .. code-block:: console

               west update
         
         #. Check the current manifest path with the following command:

            .. code-block:: console

               west config manifest.path

            The output should be:

            .. code-block:: console

               nrf

           This means that the current workspace is using the nRF Connect SDK.
 
