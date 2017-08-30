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
    from subprocess import check_output
    from subprocess import call
except:
    log('subprocess import error')


class ShellHandlerApt:

    _pwd = ""

    def __init__(self, usesudo=False):
        self.sudo = usesudo
        installed, candidate = self._check_versions("xbmc", False)
        if not installed:
            # there is no package installed via repo, so we exit here
            log("No installed package found, exiting")
            import sys
            sys.exit(0)

    def _check_versions(self, package, update=True):
        _cmd = "apt-cache policy " + package

        if update and not self._update_cache():
            return False, False

        try:
            result = check_output([_cmd], shell=True).split("\n")
        except Exception as error:
            log("ShellHandlerApt: exception while executing shell command %s: %s" %(_cmd, error))
            return False, False

        if result[0].replace(":", "") == package:
            installed = result[1].split()[1]
            candidate = result[2].split()[1]
            if installed == "(none)":
                installed = False
            if candidate == "(none)":
                candidate = False
            return installed, candidate
        else:
            log("ShellHandlerApt: error during version check")
            return False, False

    def _update_cache(self):
        _cmd = 'apt-get update'
        try:
            if self.sudo:
                x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
            else:
                x = check_output(_cmd.split())
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error))
            return False

        return True

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
        _cmd = "apt-get install -y " + package
        try:
            if self.sudo:
                x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
            else:
                x = check_output(_cmd.split())
            log("Upgrade successful")
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error))
            return False

        return True

    def upgrade_system(self):
        _cmd = "apt-get upgrade -y"
        try:
            log("Upgrading system")
            if self.sudo:
                x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
            else:
                x = check_output(_cmd.split())
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error))
            return False

        return True

    def _getpassword(self):
        if len(self._pwd) == 0:
            self._pwd = get_password_from_user()
        return self._pwd
