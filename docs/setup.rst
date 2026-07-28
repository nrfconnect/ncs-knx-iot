.. _knx_iot_setup:

Requirements and setup
######################

This page outlines the requirements that you need to meet before you start working with the |addon| and KNX IoT protocol.

Hardware requirements
*********************

To use the |addon|, you need one of the following development kits:

.. table-from-sample-yaml::

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

   .. group-tab:: Command line

      You can initialize the workspace in two alternative ways:

      **Initialize a new workspace:**

      1. In the `Get the nRF Connect SDK code <Installing the nRF Connect SDK_>`_, run the following command to initialize west with the |addon|, which also initializes nRF Connect SDK v\ |ncs_version|:

         a. Initialize ``ncs`` for the |addon|:

            .. code-block:: console

               west init -m "https://github.com/nrfconnect/ncs-knx-iot"

         #. Update the nRF Connect SDK modules:

            .. code-block:: console

               west update

      **Include the add-on in the existing nRF Connect SDK workspace:**

      1. Assuming you have an existing nRF Connect SDK workspace in the :file:`ncs` folder, run the following commands:

         a. Navigate to the workspace folder:

            .. code-block:: console

               cd ncs

         #. Clone the |addon| repository:

            .. code-block:: console

               git clone https://github.com/nrfconnect/ncs-knx-iot

         #. Set manifest path to the |addon| directory:

            .. code-block:: console

               west config manifest.path ncs-knx-iot

         #. Update the nRF Connect SDK modules:

            .. code-block:: console

               west update

      2. If you ever need to go back to using the nRF Connect SDK without the |addon|, run these commands:

         a. Configure the manifest path back to the nRF Connect SDK directory.

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
