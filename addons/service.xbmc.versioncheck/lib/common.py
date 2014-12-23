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
import xbmc
import xbmcaddon
import xbmcgui
import xbmcvfs

__addon__        = xbmcaddon.Addon()
__addonversion__ = __addon__.getAddonInfo('version')
__addonname__    = __addon__.getAddonInfo('name')
__addonpath__    = __addon__.getAddonInfo('path').decode('utf-8')
__addonprofile__ = xbmc.translatePath( __addon__.getAddonInfo('profile') ).decode('utf-8')
__icon__         = __addon__.getAddonInfo('icon')

# Fixes unicode problems
def string_unicode(text, encoding='utf-8'):
    try:
        text = unicode( text, encoding )
    except:
        pass
    return text

def normalize_string(text):
    try:
        text = unicodedata.normalize('NFKD', string_unicode(text)).encode('ascii', 'ignore')
    except:
        pass
    return text

def localise(id):
    string = normalize_string(__addon__.getLocalizedString(id))
    return string

def log(txt):
    if isinstance (txt,str):
        txt = txt.decode("utf-8")
    message = u'%s: %s' % ("Version Check", txt)
    xbmc.log(msg=message.encode("utf-8"), level=xbmc.LOGDEBUG)

def get_password_from_user():
    keyboard = xbmc.Keyboard("", __addonname__ + "," +localise(32022), True)
    keyboard.doModal()
    if (keyboard.isConfirmed()):
        pwd = keyboard.getText()
    return pwd

def message_upgrade_success():
    xbmc.executebuiltin("XBMC.Notification(%s, %s, %d, %s)" %(__addonname__,
                                                              localise(32013),
                                                              15000,
                                                              __icon__))

def message_restart():
    if dialog_yesno(32014):
        xbmc.executebuiltin("RestartApp")

def dialog_yesno(line1 = 0, line2 = 0):
    return xbmcgui.Dialog().yesno(__addonname__,
                                  localise(line1),
                                  localise(line2))

def upgrade_message(msg, oldversion, upgrade, msg_current, msg_available):
    # Don't show while watching a video
    while(xbmc.Player().isPlayingVideo() and not xbmc.abortRequested):
        xbmc.sleep(1000)
    i = 0
    while(i < 5 and not xbmc.abortRequested):
        xbmc.sleep(1000)
        i += 1
    if __addon__.getSetting("lastnotified_version") < __addonversion__:
        xbmcgui.Dialog().ok(__addonname__,
                    localise(msg),
                    localise(32001),
                    localise(32002))
        #__addon__.setSetting("lastnotified_version", __addonversion__)
    else:
        log("Already notified one time for upgrading.")
        
def upgrade_message2( version_installed, version_available, version_stable, oldversion, upgrade,):
    # shorten releasecandidate to rc
    if version_installed['tag'] == 'releasecandidate':
        version_installed['tag'] = 'rc'
    if version_available['tag'] == 'releasecandidate':
        version_available['tag'] = 'rc'
    # convert json-rpc result to strings for usage
    msg_current = '%i.%i %s%s' %(version_installed['major'],
                                   version_installed['minor'],
                                   version_installed['tag'],
                                   version_installed.get('tagversion',''))
    msg_available = version_available['major'] + '.' + version_available['minor'] + ' ' + version_available['tag'] + version_available.get('tagversion','')
    msg_stable = version_stable['major'] + '.' + version_stable['minor'] + ' ' + version_stable['tag'] + version_stable.get('tagversion','')
    msg = localise(32034) %(msg_current, msg_available)

    # Don't show notify while watching a video
    while(xbmc.Player().isPlayingVideo() and not xbmc.abortRequested):
        xbmc.sleep(1000)
    i = 0
    while(i < 10 and not xbmc.abortRequested):
        xbmc.sleep(1000)
        i += 1

    # hack: convert current version number to stable string
    # so users don't get notified again. remove in future
    if __addon__.getSetting("lastnotified_version") == '0.1.24':
        __addon__.setSetting("lastnotified_stable", msg_stable)

    # Show different dialogs depending if there's a newer stable available.
    # Also split them between xbmc and kodi notifications to reduce possible confusion.
    # People will find out once they visit the website.
    # For stable only notify once and when there's a newer stable available.
    # Ignore any add-on updates as those only count for != stable
    if oldversion == 'stable' and __addon__.getSetting("lastnotified_stable") != msg_stable: 
        if xbmcaddon.Addon('xbmc.addon').getAddonInfo('version') < "13.9.0":
            xbmcgui.Dialog().ok(__addonname__,
                                msg,
                                localise(32030),
                                localise(32031))
        else:
            xbmcgui.Dialog().ok(__addonname__,
                                msg,
                                localise(32032),
                                localise(32033))
        __addon__.setSetting("lastnotified_stable", msg_stable)
    
    elif oldversion != 'stable' and __addon__.getSetting("lastnotified_version") != msg_available:
        if xbmcaddon.Addon('xbmc.addon').getAddonInfo('version') < "13.9.0":
            # point them to xbmc.org
            xbmcgui.Dialog().ok(__addonname__,
                                msg,
                                localise(32035),
                                localise(32031))
        else:
            #use kodi.tv
            xbmcgui.Dialog().ok(__addonname__,
                                msg,
                                localise(32035),
                                localise(32033))

        # older skins don't support a text field in the OK dialog.
        # let's use split lines for now. see code above.
        '''
        msg = localise(32034) %(msg_current, msg_available)
        if oldversion == 'stable':
            msg = msg + ' ' + localise(32030)
        else:
            msg = msg + ' ' + localise(32035)
        msg = msg + ' ' + localise(32031)
        xbmcgui.Dialog().ok(__addonname__, msg)
        #__addon__.setSetting("lastnotified_version", __addonversion__)
        '''
        __addon__.setSetting("lastnotified_version", msg_available)
        
    else:
        log("Already notified one time for upgrading.")