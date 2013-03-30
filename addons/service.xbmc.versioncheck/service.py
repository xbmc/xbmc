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


import os
import platform
import xbmc
import xbmcaddon
import xbmcgui
import xbmcvfs

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = xbmcaddon.Addon()
__addonversion__ = __addon__.getAddonInfo('version')
__addonname__    = __addon__.getAddonInfo('name')
__addonpath__    = __addon__.getAddonInfo('path').decode('utf-8')
__icon__         = __addon__.getAddonInfo('icon')
__localize__    = __addon__.getLocalizedString

def log(txt):
    if isinstance (txt,str):
        txt = txt.decode("utf-8")
    message = u'%s: %s' % (__addonname__, txt)
    xbmc.log(msg=message.encode("utf-8"), level=xbmc.LOGDEBUG)

class Main:
    def __init__(self):
        if __addon__.getSetting("versioncheck_enable") == 'true' and not xbmc.getCondVisibility('System.HasAddon(os.openelec.tv)'):
            if not sys.argv[0]:
                xbmc.executebuiltin('XBMC.AlarmClock(CheckAtBoot,XBMC.RunScript(service.xbmc.versioncheck, started),00:00:30,silent)')
                xbmc.executebuiltin('XBMC.AlarmClock(CheckWhileRunning,XBMC.RunScript(service.xbmc.versioncheck, started),24:00:00,silent,loop)')
            elif sys.argv[0] and sys.argv[1] == 'started':
                if xbmc.getCondVisibility('System.Platform.Linux'):
                    oldversion = _versionchecklinux('xbmc')
                else:
                    oldversion = _versioncheck()
                if oldversion[0]:
                    _upgrademessage(oldversion[1])
            else:
                pass
                
def _versioncheck():
    # initial vars
    oldversion = False
    msg = ''
    # retrieve versionlists from supplied version file
    version_file = os.path.join(__addonpath__, 'resources/versions.txt')
    # Eden didn't have xbmcvfs.File()
    if xbmcaddon.Addon('xbmc.addon').getAddonInfo('version') < "11.9.3":
        file = open(version_file, 'r')
    else:
        file = xbmcvfs.File(version_file)
    data = file.read()
    file.close()
    version_query = unicode(data, 'utf-8', errors='ignore')
    version_query = simplejson.loads(version_query)
    
    # Create seperate version lists
    versionlist_stable = version_query['releases']['stable']
    versionlist_rc = version_query['releases']['releasecandidate']
    versionlist_beta = version_query['releases']['beta']
    versionlist_alpha = version_query['releases']['alpha']
    versionlist_prealpha = version_query['releases']['prealpha']        

    # retrieve current installed version
    json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "Application.GetProperties", "params": {"properties": ["version", "name"]}, "id": 1 }')
    json_query = unicode(json_query, 'utf-8', errors='ignore')
    json_query = simplejson.loads(json_query)
    version_installed = []
    if json_query.has_key('result') and json_query['result'].has_key('version'):
        version_installed  = json_query['result']['version']
        log("Version installed %s" %version_installed)
    # set oldversion flag to false
    oldversion = False

    ### Check to upgrade to newest available stable version
    # check on smaller major version. Smaller version than available always notify
    if version_installed['major'] < int(versionlist_stable[0]['major']):
        msg = __localize__(32003)
        oldversion = True
        log("Version available  %s" %versionlist_stable[0])

    # check on same major version installed and available
    elif version_installed['major'] == int(versionlist_stable[0]['major']):
        # check on smaller minor version
        if version_installed['minor'] < int(versionlist_stable[0]['minor']):
            msg = __localize__(32003)
            oldversion = True
            log("Version available  %s" %versionlist_stable[0])
        # check if not installed a stable so always notify
        elif version_installed['tag'] != "stable":
            msg = __localize__(32008)
            oldversion = True
            log("Version available  %s" %versionlist_stable[0])
        else:
            log("Last available stable installed")

    ### Check to upgrade to newest available RC version if not installed stable
    ## Check also oldversion hasn't been set true by previous check because if so this need to be skipped
    if not oldversion and version_installed['tag'] != "stable":
        # only check on equal or lower major because newer installed beta/alpha/prealpha version will be higher
        if versionlist_rc and version_installed['major'] <= int(versionlist_rc[0]['major']):
            if version_installed['revision'] <= versionlist_rc[0]['revision']:
                msg = __localize__(32004)
                oldversion = True
                log("Version available  %s" %versionlist_rc[0])

        # exclude if installed RC on checking for newer beta
        if not oldversion and versionlist_beta and version_installed['tag'] not in ["releasecandidate"]:
            if version_installed['major'] <= int(versionlist_beta[0]['major']):
                if version_installed['revision'] < versionlist_beta[0]['revision']:
                    msg = __localize__(32005)
                    oldversion = True
                    log("Version available  %s" %versionlist_beta[0])
    
        # exclude if installed RC or beta on checking for newer alpha
        if not oldversion and versionlist_alpha and version_installed['tag'] not in ["releasecandidate", "beta"]:
            if version_installed['major'] <= int(versionlist_alpha[0]['major']):
                if version_installed['revision'] < versionlist_alpha[0]['revision']:
                    msg = __localize__(32006)
                    oldversion = True
                    log("Version available  %s" %versionlist_alpha[0])

        # exclude if installed RC, beta or alpha on checking for newer prealpha
        if not oldversion and versionlist_prealpha and version_installed['tag'] not in ["releasecandidate", "beta", "alpha"]:
            if version_installed['major'] <= int(versionlist_prealpha[0]['major']):
                if version_installed['revision'] < versionlist_prealpha[0]['revision']:
                    msg = __localize__(32007)
                    oldversion = True
                    log("Version available  %s" %versionlist_prealpha[0])

        # Nothing to see here, move along
    else:
        # Nothing to see here, move along
        pass
    return oldversion, msg


def _versionchecklinux(package):
    if (platform.dist()[0] == "Ubuntu" or platform.dist()[0] == "Debian"):
        oldversion, msg = _versioncheckapt(package)
    else:
        log("Unsupported platform %s" %platform.dist()[0])
        sys.exit(0)
    return oldversion, msg
        
def _versioncheckapt(package):
    #check for linux using Apt
    # initial vars
    oldversion = False
    msg = ''
    result = ''
    
    # try to import apt
    try:
        import apt
        from aptdaemon import client
        from aptdaemon import errors
    except:
        log('python apt import error')
        sys.exit(0)
    apt_client = client.AptClient()
    try:
        result = apt_client.update_cache(wait=True)
        if (result == "exit-success"):
            log("Finished updating the cache")
        else:
            log("Error updating the cache %s" %result) 
    except errors.NotAuthorizedError:
        log("You are not allowed to update the cache")
        sys.exit(0)
    
    trans = apt_client.upgrade_packages([package])
    trans.simulate(reply_handler=_apttransstarted, error_handler=_apterrorhandler)
    pkg = trans.packages[4][0]
    if (pkg == package):
       cache=apt.Cache()
       cache.open(None)
       cache.upgrade()
       if (cache[package].installed and cache[package].installed.version != cache[package].candidate.version):
           log("Version installed  %s" %cache[package].installed.version)
           log("Version available  %s" %cache[package].candidate.version)
           oldversion = True
           msg = __localize__(32011)
       elif (cache[package].installed):
           log("Already on newest version  %s" %cache[package].installed.version)
       else:
           log("No installed package found, probably manual install")
           sys.exit(0)

    return oldversion, msg

def _apttransstarted():
    pass

def _apterrorhandler(error):
    raise error

def _upgrademessage(msg):
    # Don't show while watching a video
    while(xbmc.Player().isPlayingVideo() and not xbmc.abortRequested):
        xbmc.sleep(1000)
    i = 0
    while(i < 5 and not xbmc.abortRequested):
        xbmc.sleep(1000)
        i += 1
    # Detect if it's first run and only show OK dialog + ask to disable on that
    firstrun = __addon__.getSetting("versioncheck_firstrun") != 'false'
    if firstrun and not xbmc.abortRequested:
        xbmcgui.Dialog().ok(__addonname__,
                            msg,
                            __localize__(32001),
                            __localize__(32002))
        # sets check to false which is checked on startup
        if xbmcgui.Dialog().yesno(__addonname__,
                                  __localize__(32009),
                                  __localize__(32010)):
            __addon__.setSetting("versioncheck_enable", 'false')
        # set first run to false to only show a popup next startup / every two days
        __addon__.setSetting("versioncheck_firstrun", 'false')
    # Show notification after firstrun
    elif not xbmc.abortRequested:
        log(__localize__(32001) + '' + __localize__(32002))
        xbmc.executebuiltin("XBMC.Notification(%s, %s, %d, %s)" %(__addonname__,
                                                                  __localize__(32001) + '' + __localize__(32002),
                                                                  15000,
                                                                  __icon__))
    else:
        pass

if (__name__ == "__main__"):
    log('Version %s started' % __addonversion__)
    Main()
