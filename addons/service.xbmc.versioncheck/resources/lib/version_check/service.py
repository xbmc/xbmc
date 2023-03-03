# -*- coding: utf-8 -*-

"""

    Copyright (C) 2013-2014 Team-XBMC
    Copyright (C) 2014-2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

import platform
import sys

import xbmc  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from .common import ADDON
from .common import ADDON_NAME
from .common import ADDON_VERSION
from .common import dialog_yes_no
from .common import localise
from .common import log
from .common import wait_for_abort
from .common import message_restart
from .common import message_upgrade_success
from .common import upgrade_message
from .common import upgrade_message2
from .json_interface import get_version_file_list
from .json_interface import get_installed_version
from .versions import compare_version


DISTRIBUTION = ''

if sys.platform.startswith('linux'):
    if sys.version_info[0] == 3 and sys.version_info[1] >= 8:
        try:
            from .distro import distro

            DISTRIBUTION = distro.linux_distribution(full_distribution_name=False)[0].lower()

        except (AttributeError, ImportError):
            DISTRIBUTION = ''

    else:
        DISTRIBUTION = platform.linux_distribution(full_distribution_name=0)[0].lower()  # pylint: disable=no-member

if not DISTRIBUTION:
    DISTRIBUTION = platform.uname()[0].lower()


def _version_check():
    """ Check versions (non-linux)

    :return: old, current, available, and stable versions
    :rtype: bool / 'stable', dict, dict, dict
    """
    # retrieve version_lists from supplied version file
    version_list = get_version_file_list()
    # retrieve version installed
    version_installed = get_installed_version()
    # compare installed and available
    old_version, version_installed, version_available, version_stable = \
        compare_version(version_installed, version_list)
    return old_version, version_installed, version_available, version_stable


def _version_check_linux(packages):
    """ Check package version on linux

    :param packages: list of packages to check
    :type packages: list of str
    """
    if DISTRIBUTION in ['ubuntu', 'debian', 'linuxmint']:
        try:
            # try aptdaemon first
            # pylint: disable=import-outside-toplevel
            from .apt_daemon_handler import AptDaemonHandler
            handler = AptDaemonHandler()
        except:  # pylint: disable=bare-except
            # fallback to shell
            # since we need the user password, ask to check for new version first
            # pylint: disable=import-outside-toplevel
            from .shell_handler_apt import ShellHandlerApt
            handler = ShellHandlerApt(use_sudo=True)
            if dialog_yes_no(32015):
                pass
            elif dialog_yes_no(32009, 32010):
                log('disabling addon by user request')
                ADDON.setSetting('versioncheck_enable', 'false')
                return

        if handler:
            if handler.check_upgrade_available(packages[0]):
                if upgrade_message(32012):
                    if ADDON.getSetting('upgrade_system') == 'false':
                        result = handler.upgrade_package(packages[0])
                    else:
                        result = handler.upgrade_system()
                    if result:
                        message_upgrade_success()
                        message_restart()
                    else:
                        log('Error during upgrade')
                    return

            log('No upgrade available')
            return

        log('Error: no handler found')
        return

    log('Unsupported platform %s' % DISTRIBUTION)
    sys.exit(0)


def _check_cryptography():
    """ Check for cryptography package, and version

    Python cryptography < 1.7 (still shipped with Ubuntu 16.04) has issues with
    pyOpenSSL integration, leading to all sorts of weird bugs - check here to save
    on some troubleshooting. This check may be removed in the future (when switching
    to Python3?)
    See https://github.com/pyca/pyopenssl/issues/542
    """
    try:
        import cryptography  # pylint: disable=import-outside-toplevel
        ver = cryptography.__version__
    except ImportError:
        # If the module is not found - no problem
        return

    ver_parts = list(map(int, ver.split('.')))
    if len(ver_parts) < 2 or ver_parts[0] < 1 or (ver_parts[0] == 1 and ver_parts[1] < 7):
        log('Python cryptography module version %s is too old, at least version 1.7 needed' % ver)
        xbmcgui.Dialog().ok(
            ADDON_NAME,
            '[CR]'.join([localise(32040) % ver, localise(32041), localise(32042)])
        )


def run():
    """ Service entry-point
    """
    _check_cryptography()

    if ADDON.getSetting('versioncheck_enable') == 'false':
        log('Disabled')
    else:
        log('Version %s started' % ADDON_VERSION)

        if wait_for_abort(5):
            sys.exit(0)

        if (xbmc.getCondVisibility('System.Platform.Linux') and
                ADDON.getSetting('upgrade_apt') == 'true'):
            _version_check_linux(['kodi'])
        else:
            old_version, version_installed, version_available, version_stable = _version_check()
            if old_version:
                upgrade_message2(version_installed, version_available, version_stable, old_version)
