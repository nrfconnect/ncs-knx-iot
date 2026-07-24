.. _knx_iot_known_issues:

Known issues
############

.. contents::
   :local:
   :depth: 3

Known issues listed on this page *and* tagged with the :ref:`latest release version <release_notes>` are valid for the current state of development.
Use the drop-down filter to see known issues for previous releases and check if they are still valid.

A known issue can list one or both of the following entries:

* **Affected platforms:**

  If a known issue does not have any specific platforms listed, it is valid for all hardware platforms.

* **Workaround:**

  Some known issues have a workaround.
  Sometimes, they are discovered later and added over time.

.. version-filter::
   :default: v1-0-0
   :container: dl/dt
   :tags: [("wontfix", "Won't fix")]

.. page-filter::
   :name: issues

   wontfix Won't fix

List of known issues for v0.1.0 release
***************************************

The following release is `experimental <Software maturity levels_>`_.
Because of this, not all behavior is fully defined or validated.

.. _knx_iot_known_issue_actuator_state_not_persisted:

KRKNWK-22216: The actuator light state is not persisted after reboot
  The light switch actuator sample does not store the switched load state in non-volatile memory.
  After a reboot, **LED 2** is always off, even if the light was on before the reset.

  **Affected platforms:** All
