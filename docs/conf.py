# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import re
import sys
from pathlib import Path

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'KNX IoT Add-on for the nRF Connect SDK'
copyright = '2026, Nordic Semiconductor'
author = 'Nordic Semiconductor'
release = '0.1.0'
version = '0.1.0'

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
    'sphinx.ext.graphviz',
]

root_doc = 'index'

templates_path = ['_templates']
exclude_patterns = ['_build_sphinx', '_build_doxygen', 'venv', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_ncs_theme'

html_theme_options = {
    'docsets': {},
    "ncs_url": "https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/",
    "ncs_label": "nRF Connect SDK Docs",
    "addons_url": "https://nrfconnect.github.io/ncs-app-index/",
    "bare_metal_url": "",
    "logo_url": "https://docs.nordicsemi.com",
}

html_show_sphinx = False

# Copied into the HTML output for the hosted documentation version switcher.
html_extra_path = ['versions.json']

## -- Options for Breathe ----------------------------------------------------
# https://breathe.readthedocs.io/en/latest/index.html
#
# WARNING: please, check breathe maintainership status before using this
# extension in production!

breathe_projects = {'ncs-knx-iot': '_build_doxygen/xml'}
breathe_default_project = 'ncs-knx-iot'
breathe_default_members = ('members', )

# Include following files at the end of each .rst file.
# Substitutions such as |ncs_version| are not expanded in external link URIs by
# Sphinx, so apply them when loading links.txt.
def _read_rst_substitutions(path: Path) -> dict[str, str]:
    substitutions: dict[str, str] = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        match = re.match(r'\.\. \|([^|]+)\| replace:: (.*)', line)
        if match:
            substitutions[match.group(1)] = match.group(2)
    return substitutions


def _apply_substitutions(text: str, substitutions: dict[str, str]) -> str:
    for name, value in substitutions.items():
        text = text.replace(f'|{name}|', value)
    return text


_shortcuts_path = DOC_BASE / 'shortcuts.txt'
_substitutions = _read_rst_substitutions(_shortcuts_path)
_links = _apply_substitutions(
    (DOC_BASE / 'links.txt').read_text(encoding='utf-8'),
    _substitutions,
)

rst_epilog = f"""{_links}
.. include:: /shortcuts.txt
"""

# -- Options for graphviz -------------------------------------------------------

graphviz_output_format = 'svg'
graphviz_dot = 'dot'
graphviz_dot_args = []

# -- Options for table_from_rows (local add-on copy) -------------------------

table_from_rows_base_dir = KNX_IOT_BASE
table_from_sample_yaml_board_reference = '/includes/sample_board_rows.txt'

# -- Options for knx_iot_kconfig ---------------------------------------------

knx_iot_kconfig_base_dir = str(KNX_IOT_BASE)

# -- Options for options_from_kconfig (local add-on copy) -----------------------

options_from_kconfig_base_dir = str(KNX_IOT_BASE)
