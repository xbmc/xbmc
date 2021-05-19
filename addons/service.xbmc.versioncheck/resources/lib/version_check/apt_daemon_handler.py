# -*- coding: utf-8 -*-

"""

    Copyright (C) 2013-2014 Team-XBMC
    Copyright (C) 2014-2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

from .common import log
from .handler import Handler

try:
    import apt
    from aptdaemon import client
    from aptdaemon import errors
except ImportError:
    apt = None
    client = None
    errors = None
    log('ImportError: apt, aptdaemon')


class AptDaemonHandler(Handler):
    """ Apt daemon handler
    """

    def __init__(self):
        Handler.__init__(self)
        self.apt_client = client.AptClient()

    def _check_versions(self, package):
        """ Check apt package versions

        :param package: package to check
        :type package: str
        :return: installed version, candidate version
        :rtype: str, str / False, False
        """
        if self.update and not self._update_cache():
            return False, False
        try:
            trans = self.apt_client.upgrade_packages([package])
            # trans = self.apt_client.upgrade_packages('bla')
            trans.simulate(reply_handler=self._apt_trans_started,
                           error_handler=self._apt_error_handler)
            pkg = trans.packages[4][0]
            if pkg == package:
                cache = apt.Cache()
                cache.open(None)
                cache.upgrade()
                if cache[pkg].installed:
                    return cache[pkg].installed.version, cache[pkg].candidate.version

            return False, False

        except Exception as error:  # pylint: disable=broad-except
            log('Exception while checking versions: %s' % error)
            return False, False

    def _update_cache(self):
        """ Update apt client cache

        :return: success of updating apt cache
        :rtype: bool
        """
        try:
            return self.apt_client.update_cache(wait=True) == 'exit-success'
        except errors.NotAuthorizedError:
            log('You are not allowed to update the cache')
            return False

    def upgrade_package(self, package):
        """ Upgrade apt package

        :param package: package to upgrade
        :type package: str
        :return: success of apt package upgrade
        :rtype: bool
        """
        try:
            log('Installing new version')
            if self.apt_client.upgrade_packages([package], wait=True) == 'exit-success':
                log('Upgrade successful')
                return True
        except Exception as error:  # pylint: disable=broad-except
            log('Exception during upgrade: %s' % error)
        return False

    def upgrade_system(self):
        """ Upgrade system

        :return: success of system upgrade
        :rtype: bool
        """
        try:
            log('Upgrading system')
            if self.apt_client.upgrade_system(wait=True) == 'exit-success':
                return True
        except Exception as error:  # pylint: disable=broad-except
            log('Exception during system upgrade: %s' % error)
        return False

    def _apt_trans_started(self):
        """ Apt transfer reply handler
        """

    @staticmethod
    def _apt_error_handler(error):
        """ Apt transfer error handler

        :param error: apt error message
        :type error: str
        """
        log('Apt Error %s' % error)
