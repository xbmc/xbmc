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
import xbmcgui
import lib.common
from lib.common import log, dialog_yesno, localise, wait_for_end_of_video

ADDON        = lib.common.ADDON
ADDONVERSION = lib.common.ADDONVERSION
ADDONNAME    = lib.common.ADDONNAME
ADDONPATH    = lib.common.ADDONPATH
ICON         = lib.common.ICON
oldversion = False


class Main:
    def __init__(self):
        linux = False
        packages = []
        global dialogbg
        dialogbg = xbmcgui.DialogProgressBG()
        dialogbg.create('Version Check')
        if xbmc.getCondVisibility('System.Platform.Linux') and ADDON.getSetting("upgrade_apt") == 'true':
            packages = ['kodi']
            self._versionchecklinux(packages)
        elif ADDON.getSetting("versioncheck_enable") == "true":
            self._versioncheck()
        dialogbg.close('Version Check')

    def _versioncheck(self):
        # initial vars
        from lib.jsoninterface import get_installedversion, get_versionfilelist
        from lib.versions import compare_version
        from lib.common import upgrade_message
        # retrieve versionlists from supplied version file
        versionlist = get_versionfilelist()
        # retrieve version installed
        version_installed = get_installedversion()
        # copmpare installed and available
        oldversion, version_installed, version_available, version_stable = compare_version(version_installed, versionlist)
        if oldversion:
          upgrade_message( version_installed, version_available, version_stable, oldversion, False)
        dialogbg.update(100,'Version Check',localise(32027))

    def _versionchecklinux(self,packages):
        if platform.dist()[0].lower() in ['ubuntu', 'debian', 'linuxmint']:
            handler = False
            result = False
            try:
                # try aptdaemon first
                from lib.aptdaemonhandler import AptdaemonHandler
                handler = AptdaemonHandler()
            except:
                # fallback to shell
                # since we need the user password, ask to check for new version first
                from lib.shellhandlerapt import ShellHandlerApt
                sudo = False
                if ADDON.getSetting("upgrade_sudo") == "true":
                    sudo = True
                handler = ShellHandlerApt(sudo)
            if handler:
                if ADDON.getSetting("versioncheck_enable") == "true":
                    dialogbg.update(10,'Version Check',localise(32007))
                    if handler.check_upgrade_available(packages[0]):
                        dialogbg.update(20,'Version Check',localise(32016))
                        result = handler.upgrade_package(packages[0])
                        if result:
                            from lib.common import message_upgrade_success, message_restart
                            dialogbg.update(20,'Version Check',localise(32013))
                            message_upgrade_success()
                            message_restart()
                        else:
                            log("Abort during upgrade %s" %packages[0],xbmc.LOGERROR)
                            dialogbg.update(20,'Version Check',localise(32029))
                    else:
                        dialogbg.update(50,'Version Check',localise(32027))
                if ADDON.getSetting("upgrade_system") == "true":
                    dialogbg.update(60,'Version Check', localise (32008))
                    if handler.check_upgrade_system_available():
                        dialogbg.update(80,'Version Check',localise(32019))
                        if dialog_yesno(32018):
                          if ADDON.getSetting("upgrade_autoremove"):
                            handler.autoremove_package()
                          result = handler.upgrade_system()
                          if result:
                              from lib.common import message_upgrade_success, message_restart_system
                              dialogbg.update(80,'Version Check',localise(32013))
                              message_upgrade_success()
                              message_restart_system()
                          else:
                              log("Abort during upgrade system",xbmc.LOGERROR)
                              dialogbg.update(80,'Version Check',localise(32029))
                    else:
                        dialogbg.update(100,'Version Check',localise(32028))
                dialogbg.update(100,'Version Check','')
            else:
                log("Error: no handler found",xbmc.LOGERROR)
                dialogbg.update(100,'Version Check',localise(32029))
        else:
            log("Unsupported platform %s" %platform.dist()[0],xbmc.LOGERROR)
            sys.exit(0)

if (__name__ == "__main__"):
    log('Service started',xbmc.LOGNOTICE)
    monitor = xbmc.Monitor()
    while not monitor.abortRequested():
        wait_for_end_of_video()
        Main()
        if monitor.waitForAbort(86400):
            log('Bye bye',xbmc.LOGNOTICE)
            break
