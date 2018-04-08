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
    from subprocess import CalledProcessError
except Exception as error:
    log('subprocess import error :%s' %error,xbmc.LOGWARNING)

class ShellHandlerApt:

    _pwd = ""

    def __init__(self, usesudo=False):
        self.sudo = usesudo
        installed, candidate = self._check_versions("kodi", False)
        if not installed:
            # there is no package installed via repo, so we exit here
            log("No installed package found, exiting",xbmc.LOGNOTICE)
            import sys
            sys.exit(0)

    def _check_versions(self, package, update=True):
        _cmd = "apt-cache policy " + package

        if update and not self._update_cache():
            return False, False

        try:
            result = check_output([_cmd], shell=True).split("\n")
        except Exception as error:
            log("ShellHandlerApt: exception while executing shell command %s: %s" %(_cmd, error),xbmc.LOGERROR)
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
            log("ShellHandlerApt: error during version check",xbmc.LOGERROR)
            return False, False

    def _update_cache(self):
        _cmd = 'apt-get update'
        try:
            if self.sudo:
                x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
            else:
                x = check_output(_cmd.split())
            log("Update cache successful",xbmc.LOGNOTICE)
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error),xbmc.LOGERROR)
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
                log("Already on newest version for %s" %package,xbmc.LOGNOTICE)
        elif not installed:
                log("No installed package found for %s" %package,xbmc.LOGNOTICE)
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
            log("Install package %s successful" %package,xbmc.LOGNOTICE)
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error),xbmc.LOGERROR)
            return False

        return True

    def upgrade_system(self):
        _cmd = "apt-get dist-upgrade -y"
        try:
            log("Upgrading system")
            if self.sudo:
                x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
            else:
                x = check_output(_cmd.split())
            log("Upgrade System successful",xbmc.LOGNOTICE)
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error),xbmc.LOGERROR)
            return False

        return True

    def _getpassword(self):
        try:
            check_output('sudo -n true',shell=True)
            log("No mandatory password")
            return self._pwd
        except CalledProcessError as sudotest:
            log("Mandatory password for [SUDO]")
            if len(self._pwd) == 0:
                self._pwd = get_password_from_user()
            return self._pwd

    def check_upgrade_system_available(self):
        _cmd = "apt list --upgradable"
        try:
            if self._update_cache():
                if self.sudo:
                    x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
                else:
                    x = check_output(_cmd.split())
                n = len(x.splitlines())
                if (n > 1):
                    log("Upgrade system available",xbmc.LOGNOTICE)
                    return True
            log("No system update available",xbmc.LOGNOTICE)
            return False
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error),xbmc.LOGERROR)
            return False

        return True

    def  autoremove_package(self):
        _cmd = "apt-get autoremove -y "
        try:
            if self.sudo:
                x = check_output('echo \'%s\' | sudo -S %s' %(self._getpassword(), _cmd), shell=True)
            else:
                x = check_output(_cmd.split())
            log("Automatically remove old package successful",xbmc.LOGNOTICE)
        except Exception as error:
            log("Exception while executing shell command %s: %s" %(_cmd, error),xbmc.LOGERROR)
            return False

        return True
