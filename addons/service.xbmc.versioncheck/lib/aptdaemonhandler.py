# -*- coding: utf-8 -*-
#
#     Copyright (C) 2013 Team-XBMC
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
#
import xbmc
from .common import *

try:
    #import apt
    import apt
    from aptdaemon import client
    from aptdaemon import errors
except:
    log('python apt import error')

class AptdaemonHandler:

    def __init__(self):
        self.aptclient = client.AptClient()

    def _check_versions(self, package):
        if not self._update_cache():
            return False, False
        try:
            trans = self.aptclient.upgrade_packages([package])
            #trans = self.aptclient.upgrade_packages("bla")
            trans.simulate(reply_handler=self._apttransstarted, error_handler=self._apterrorhandler)
            pkg = trans.packages[4][0]
            if pkg == package:
               cache=apt.Cache()
               cache.open(None)
               cache.upgrade()
               if cache[pkg].installed:
                   return cache[pkg].installed.version, cache[pkg].candidate.version

            return False, False

        except Exception as error:
            log("Exception while checking versions: %s" %error)
            return False, False

    def _update_cache(self):
        try:
            if self.aptclient.update_cache(wait=True) == "exit-success":
                return True
            else:
                return False
        except errors.NotAuthorizedError:
            log("You are not allowed to update the cache")
            return False

    def check_upgrade_available(self, package):
        '''returns True if newer package is available in the repositories'''
        installed, candidate = self._check_versions(package)
        if installed and candidate:
            if installed != candidate:
                log("Version installed  %s" %installed)
                log("Version available  %s" %candidate)
                return True
            else:
                log("Already on newest version")
        elif not installed:
                log("No installed package found")
                return False
        else:
            return False

    def upgrade_package(self, package):
        try:
            log("Installing new version")
            if self.aptclient.upgrade_packages([package], wait=True) == "exit-success":
                log("Upgrade successful")
                return True
        except Exception as error:
            log("Exception during upgrade: %s" %error)
        return False

    def upgrade_system(self):
        try:
            log("Upgrading system")
            if self.aptclient.upgrade_system(wait=True) == "exit-success":
                return True
        except Exception as error:
            log("Exception during system upgrade: %s" %error)
        return False

    def _getpassword(self):
        if len(self._pwd) == 0:
            self._pwd = get_password_from_user()
        return self._pwd

    def _apttransstarted(self):
        pass
    
    def _apterrorhandler(self, error):
        log("Apt Error %s" %error)