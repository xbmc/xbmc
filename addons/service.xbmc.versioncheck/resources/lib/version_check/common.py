# -*- coding: utf-8 -*-

"""

    Copyright (C) 2013-2014 Team-XBMC
    Copyright (C) 2014-2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

import sys

import xbmc  # pylint: disable=import-error
import xbmcaddon  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error
import xbmcvfs  # pylint: disable=import-error

try:
    xbmc.translatePath = xbmcvfs.translatePath
except AttributeError:
    pass

ADDON = xbmcaddon.Addon('service.xbmc.versioncheck')
ADDON_VERSION = ADDON.getAddonInfo('version')
ADDON_NAME = ADDON.getAddonInfo('name')
if sys.version_info[0] >= 3:
    ADDON_PATH = ADDON.getAddonInfo('path')
    ADDON_PROFILE = xbmc.translatePath(ADDON.getAddonInfo('profile'))
else:
    ADDON_PATH = ADDON.getAddonInfo('path').decode('utf-8')
    ADDON_PROFILE = xbmc.translatePath(ADDON.getAddonInfo('profile')).decode('utf-8')
ICON = ADDON.getAddonInfo('icon')
KODI_VERSION_MAJOR = int(xbmc.getInfoLabel('System.BuildVersion')[0:2])

MONITOR = xbmc.Monitor()


# Fixes unicode problems
def string_unicode(text, encoding='utf-8'):
    """ Python 2/3 -> unicode/str

    :param text: text to convert
    :type text: unicode (py2) / str (py3) / bytes (py3)
    :param encoding: text encoding
    :type encoding: str
    :return: converted text
    :rtype: unicode (py2) / str (py3)
    """
    try:
        if sys.version_info[0] >= 3:
            text = str(text)
        else:
            text = unicode(text, encoding)  # pylint: disable=undefined-variable
    except:  # pylint: disable=bare-except
        pass
    return text


def normalize_string(text):
    """ Normalize string

    :param text: text to normalize
    :type text: unicode (py2) / str (py3) / bytes (py3)
    :return: normalized text
    :rtype: unicode (py2) / str (py3)
    """
    try:
        text = unicodedata.normalize('NFKD', string_unicode(text)).encode('ascii', 'ignore')  # pylint: disable=undefined-variable
    except:  # pylint: disable=bare-except
        pass
    return text


def localise(string_id):
    """ Localise string id

    :param string_id: id of the string to localise
    :type string_id: int
    :return: localised string
    :rtype: unicode (py2) / str (py3)
    """
    string = normalize_string(ADDON.getLocalizedString(string_id))
    return string


def log(txt):
    """ Log text at xbmc.LOGDEBUG level

    :param txt: text to log
    :type txt: str / unicode / bytes (py3)
    """
    if sys.version_info[0] >= 3:
        if isinstance(txt, bytes):
            txt = txt.decode('utf-8')
        message = '%s: %s' % (ADDON_NAME, txt)
    else:
        if isinstance(txt, str):
            txt = txt.decode('utf-8')
        message = (u'%s: %s' % (ADDON_NAME, txt)).encode('utf-8')  # pylint: disable=redundant-u-string-prefix
    xbmc.log(msg=message, level=xbmc.LOGDEBUG)


def notification(heading, message, icon=None, time=15000, sound=True):
    """ Create a notification

    :param heading: notification heading
    :type heading: str
    :param message: notification message
    :type message: str
    :param icon: path and filename for the notification icon
    :type icon: str
    :param time: time to display notification
    :type time: int
    :param sound: is notification audible
    :type sound: bool
    """
    if not icon:
        icon = ICON
    xbmcgui.Dialog().notification(heading, message, icon, time, sound)


def get_password_from_user():
    """ Prompt user to input password

    :return: password
    :rtype: str
    """
    pwd = ''
    keyboard = xbmc.Keyboard('', ADDON_NAME + ': ' + localise(32022), True)
    keyboard.doModal()
    if keyboard.isConfirmed():
        pwd = keyboard.getText()
    return pwd


def message_upgrade_success():
    """ Upgrade success notification
    """
    notification(ADDON_NAME, localise(32013))


def message_restart():
    """ Prompt user to restart Kodi
    """
    if dialog_yes_no(32014):
        xbmc.executebuiltin('RestartApp')


def dialog_yes_no(line1=0, line2=0):
    """ Prompt user with yes/no dialog

    :param line1: string id for the first line of the dialog
    :type line1: int
    :param line2: string id for the second line of the dialog
    :type line2: int
    :return: users selection (yes / no)
    :rtype: bool
    """
    return xbmcgui.Dialog().yesno(ADDON_NAME, '[CR]'.join([localise(line1), localise(line2)]))


def upgrade_message(msg):
    """ Prompt user with upgrade suggestion message

    :param msg: string id for prompt message
    :type msg: int
    """
    wait_for_end_of_video()

    if ADDON.getSetting('lastnotified_version') < ADDON_VERSION:
        xbmcgui.Dialog().ok(
            ADDON_NAME,
            '[CR]'.join([localise(msg), localise(32001), localise(32002)])
        )
    else:
        log('Already notified one time for upgrading.')


def upgrade_message2(version_installed, version_available, version_stable, old_version):
    """ Prompt user with upgrade suggestion message

    :param version_installed: currently installed version
    :type version_installed: dict
    :param version_available: available version
    :type version_available: dict
    :param version_stable: latest stable version
    :type version_stable: dict
    :param old_version: whether using an old version
    :type old_version: bool / 'stable'
    """
    # shorten releasecandidate to rc
    if version_installed['tag'] == 'releasecandidate':
        version_installed['tag'] = 'rc'
    if version_available['tag'] == 'releasecandidate':
        version_available['tag'] = 'rc'
    # convert json-rpc result to strings for usage
    msg_current = '%i.%i %s%s' % (version_installed['major'],
                                  version_installed['minor'],
                                  version_installed['tag'],
                                  version_installed.get('tagversion', ''))
    msg_available = version_available['major'] + '.' + version_available['minor'] + ' ' + \
                    version_available['tag'] + version_available.get('tagversion', '')
    msg_stable = version_stable['major'] + '.' + version_stable['minor'] + ' ' + \
                 version_stable['tag'] + version_stable.get('tagversion', '')
    msg = localise(32034) % (msg_current, msg_available)

    wait_for_end_of_video()

    # hack: convert current version number to stable string
    # so users don't get notified again. remove in future
    if ADDON.getSetting('lastnotified_version') == '0.1.24':
        ADDON.setSetting('lastnotified_stable', msg_stable)

    # Show different dialogs depending if there's a newer stable available.
    # Also split them between xbmc and kodi notifications to reduce possible confusion.
    # People will find out once they visit the website.
    # For stable only notify once and when there's a newer stable available.
    # Ignore any add-on updates as those only count for != stable
    if old_version == 'stable' and ADDON.getSetting('lastnotified_stable') != msg_stable:
        if xbmcaddon.Addon('xbmc.addon').getAddonInfo('version') < '13.9.0':
            xbmcgui.Dialog().ok(ADDON_NAME, '[CR]'.join([msg, localise(32030), localise(32031)]))
        else:
            xbmcgui.Dialog().ok(ADDON_NAME, '[CR]'.join([msg, localise(32032), localise(32033)]))
        ADDON.setSetting('lastnotified_stable', msg_stable)

    elif old_version != 'stable' and ADDON.getSetting('lastnotified_version') != msg_available:
        if xbmcaddon.Addon('xbmc.addon').getAddonInfo('version') < '13.9.0':
            # point them to xbmc.org
            xbmcgui.Dialog().ok(ADDON_NAME, '[CR]'.join([msg, localise(32035), localise(32031)]))
        else:
            # use kodi.tv
            xbmcgui.Dialog().ok(ADDON_NAME, '[CR]'.join([msg, localise(32035), localise(32033)]))

        ADDON.setSetting('lastnotified_version', msg_available)

    else:
        log('Already notified one time for upgrading.')


def abort_requested():
    """ Kodi 13+ compatible xbmc.Monitor().abortRequested()

    :return: whether abort requested
    :rtype: bool
    """
    if KODI_VERSION_MAJOR > 13:
        return MONITOR.abortRequested()

    return xbmc.abortRequested


def wait_for_abort(seconds):
    """ Kodi 13+ compatible xbmc.Monitor().waitForAbort()

    :param seconds: seconds to wait for abort
    :type seconds: int / float
    :return: whether abort was requested
    :rtype: bool
    """
    if KODI_VERSION_MAJOR > 13:
        return MONITOR.waitForAbort(seconds)

    for _ in range(0, seconds * 1000 / 200):
        if xbmc.abortRequested:
            return True
        xbmc.sleep(200)

    return False


def wait_for_end_of_video():
    """ Wait for video playback to end
    """
    # Don't show notify while watching a video
    while xbmc.Player().isPlayingVideo() and not abort_requested():
        if wait_for_abort(1):
            # Abort was requested while waiting. We should exit
            break
    i = 0
    while i < 10 and not abort_requested():
        if wait_for_abort(1):
            # Abort was requested while waiting. We should exit
            break
        i += 1
