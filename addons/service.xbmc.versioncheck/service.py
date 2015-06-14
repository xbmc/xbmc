#!/usr/bin/python
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

import platform
import xbmc
import lib.common
from lib.common import log, dialog_yesno
from lib.common import upgrade_message as _upgrademessage
from lib.common import upgrade_message2 as _upgrademessage2

__addon__        = lib.common.__addon__
__addonversion__ = lib.common.__addonversion__
__addonname__    = lib.common.__addonname__
__addonpath__    = lib.common.__addonpath__
__icon__         = lib.common.__icon__
oldversion = False

class Main:
    def __init__(self):
        linux = False
        packages = []
        xbmc.sleep(5000)
        if xbmc.getCondVisibility('System.Platform.Linux') and __addon__.getSetting("upgrade_apt") == 'true':
            packages = ['kodi']
            _versionchecklinux(packages)
        else:
            oldversion, version_installed, version_available, version_stable = _versioncheck()
            if oldversion:
                _upgrademessage2( version_installed, version_available, version_stable, oldversion, False)
                
def _versioncheck():
    # initial vars
    from lib.jsoninterface import get_installedversion, get_versionfilelist
    from lib.versions import compare_version
    # retrieve versionlists from supplied version file
    versionlist = get_versionfilelist()
    # retrieve version installed
    version_installed = get_installedversion()
    # copmpare installed and available
    oldversion, version_installed, version_available, version_stable = compare_version(version_installed, versionlist)
    return oldversion, version_installed, version_available, version_stable


def _versionchecklinux(packages):
    if platform.dist()[0].lower() in ['ubuntu', 'debian', 'linuxmint']:
        handler = False
        result = False
        try:
            # try aptdeamon first
            from lib.aptdeamonhandler import AptdeamonHandler
            handler = AptdeamonHandler()
        except:
            # fallback to shell
            # since we need the user password, ask to check for new version first
            from lib.shellhandlerapt import ShellHandlerApt
            sudo = True
            handler = ShellHandlerApt(sudo)
            if dialog_yesno(32015):
                pass
            elif dialog_yesno(32009, 32010):
                log("disabling addon by user request")
                __addon__.setSetting("versioncheck_enable", 'false')
                return

        if handler:
            if handler.check_upgrade_available(packages[0]):
                if _upgrademessage(32012, oldversion, True):
                    if __addon__.getSetting("upgrade_system") == "false":
                        result = handler.upgrade_package(packages[0])
                    else:
                        result = handler.upgrade_system()
                    if result:
                        from lib.common import message_upgrade_success, message_restart
                        message_upgrade_success()
                        message_restart()
                    else:
                        log("Error during upgrade")
        else:
            log("Error: no handler found")
    else:
        log("Unsupported platform %s" %platform.dist()[0])
        sys.exit(0)



if (__name__ == "__main__"):
    log('Version %s started' % __addonversion__)
    Main()
