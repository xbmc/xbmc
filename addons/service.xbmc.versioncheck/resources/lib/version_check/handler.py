# -*- coding: utf-8 -*-

"""

    Copyright (C) 2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

from .common import get_password_from_user
from .common import log


class Handler:
    """ Base handler class for apt_daemon_handler, and shell_handler_apt
    """

    def __init__(self):
        self._pwd = ''
        self._update = True

    @property
    def pwd(self):
        """ password property

        :return: password
        :rtype: str
        """
        return self._pwd

    @pwd.setter
    def pwd(self, value):
        """ password setter

        :param value: password
        :type value: str
        """
        self._pwd = value

    @property
    def update(self):
        """ update apt-cache property

        :return: whether to update apt-cache or not when checking for upgrades
        :rtype: bool
        """
        return self._update

    @update.setter
    def update(self, value):
        """ update apt-cache setter

        :param value: whether to update apt-cache or not when checking for upgrades
        :type value: bool
        """
        self._update = value

    def _check_versions(self, package):
        raise NotImplementedError

    def check_upgrade_available(self, package):
        """ Check if package upgrade is available

        :param package: package to check for upgrade availability
        :type package: str
        :return: whether an upgrade exists for the provided package
        :rtype: bool
        """
        installed, candidate = self._check_versions(package)
        if installed and candidate:
            if installed != candidate:
                log('Version installed  %s' % installed)
                log('Version available  %s' % candidate)
                return True
            log('Already on newest version')
            return False

        if not installed:
            log('No installed package found')

        return False

    def _get_password(self):
        """ Get password, ask user for password if not known

        :return: password
        :rtype: str
        """
        if not self.pwd:
            self.pwd = get_password_from_user()
        return self.pwd
