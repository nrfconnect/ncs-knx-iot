# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Register add-on Kconfig symbols discovered from local Kconfig files."""

import re
from pathlib import Path

from sphinx.application import Sphinx

from kconfig_domain import KconfigDomain

__version__ = '0.0.1'

_CONFIG_RE = re.compile(r'^\s*config\s+(\w+)', re.MULTILINE)


def _is_kconfig_file(path: Path) -> bool:
    if '__pycache__' in path.parts:
        return False
    name = path.name.lower()
    return name == 'kconfig' or name.startswith('kconfig.')


def discover_kconfig_options(base_dir: Path) -> list[str]:
    options = set()
    for kconfig in base_dir.rglob('Kconfig*'):
        if not kconfig.is_file() or not _is_kconfig_file(kconfig):
            continue
        for name in _CONFIG_RE.findall(kconfig.read_text(encoding='utf-8')):
            options.add(f'CONFIG_{name}')
    return sorted(options)


def _register_addon_options(app: Sphinx, *args) -> None:
    domain = app.env.get_domain('kconfig')
    base_dir = Path(app.config.knx_iot_kconfig_base_dir)
    for option in discover_kconfig_options(base_dir):
        if any(name == option for name, *_ in domain.get_objects()):
            continue
        domain.add_option(option, docname='config')


def setup(app: Sphinx):
    import kconfig_domain

    kconfig_domain.setup(app)
    app.add_config_value('knx_iot_kconfig_base_dir', None, 'env')
    app.add_domain(KconfigDomain)
    app.connect('env-before-read-docs', _register_addon_options)

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
