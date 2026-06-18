# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
from pathlib import Path

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'KNX IoT add-on for nRF Connect SDK'
copyright = '2026, Nordic Semiconductor'
author = 'Nordic Semiconductor'
release = '1.0.0'

# Paths

KNX_IOT_BASE = Path(__file__).resolve().parents[1]
DOC_BASE = Path(__file__).resolve().parent

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

sys.path.insert(0, str(DOC_BASE / '_extensions'))

extensions = [
    'table_from_rows',
    'breathe',
    'sphinxcontrib.mscgen',
    'sphinx_tabs.tabs',
    'sphinx_togglebutton',
    'sphinx_copybutton',
    'page_filter',
    'knx_iot_kconfig',
    'options_from_kconfig',
]

root_doc = 'index'

templates_path = ['_templates']
exclude_patterns = ['_build_sphinx', '_build_doxygen', 'venv', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_ncs_theme'

## -- Options for Breathe ----------------------------------------------------
# https://breathe.readthedocs.io/en/latest/index.html
#
# WARNING: please, check breathe maintainership status before using this
# extension in production!

breathe_projects = {'ncs-knx-iot': '_build_doxygen/xml'}
breathe_default_project = 'ncs-knx-iot'
breathe_default_members = ('members', )

# Include following files at the end of each .rst file.
rst_epilog = """
.. include:: /links.txt
.. include:: /shortcuts.txt
"""

# -- Options for table_from_rows (local add-on copy) -------------------------

table_from_rows_base_dir = KNX_IOT_BASE
table_from_sample_yaml_board_reference = '/includes/sample_board_rows.txt'

# -- Options for knx_iot_kconfig ---------------------------------------------

knx_iot_kconfig_base_dir = str(KNX_IOT_BASE)

# -- Options for options_from_kconfig (local add-on copy) -----------------------

options_from_kconfig_base_dir = str(KNX_IOT_BASE)
