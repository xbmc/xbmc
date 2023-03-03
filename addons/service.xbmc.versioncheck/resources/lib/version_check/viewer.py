#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""

    Copyright (C) 2011-2013 Martijn Kaijser
    Copyright (C) 2013-2014 Team-XBMC
    Copyright (C) 2014-2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

from contextlib import closing
import os
import sys

import xbmc  # pylint: disable=import-error
import xbmcaddon  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error
import xbmcvfs  # pylint: disable=import-error

_ADDON = xbmcaddon.Addon('service.xbmc.versioncheck')
_ADDON_NAME = _ADDON.getAddonInfo('name')
if sys.version_info[0] >= 3:
    _ADDON_PATH = _ADDON.getAddonInfo('path')
else:
    _ADDON_PATH = _ADDON.getAddonInfo('path').decode('utf-8')
_ICON = _ADDON.getAddonInfo('icon')


class Viewer:
    """ Show user a text viewer (WINDOW_DIALOG_TEXT_VIEWER)
    Include the text file for the viewers body in the resources/ directory

    usage:
        script_path = os.path.join(_ADDON_PATH, 'resources', 'lib', 'version_check', 'viewer.py')
        xbmc.executebuiltin('RunScript(%s,%s,%s)' % (script_path, 'Heading', 'notice.txt'))

    :param heading: text viewer heading
    :type heading: str
    :param filename: filename to use for text viewers body
    :type filename: str
    """
    WINDOW = 10147
    CONTROL_LABEL = 1
    CONTROL_TEXTBOX = 5

    def __init__(self, heading, filename):
        self.heading = heading
        self.filename = filename
        # activate the text viewer window
        xbmc.executebuiltin('ActivateWindow(%d)' % (self.WINDOW,))
        # get window
        self.window = xbmcgui.Window(self.WINDOW)
        # give window time to initialize
        xbmc.sleep(100)
        # set controls
        self.set_controls()

    def set_controls(self):
        """ Set the window controls
        """
        # get text viewer body text
        text = self.get_text()
        # set heading
        self.window.getControl(self.CONTROL_LABEL).setLabel('%s : %s' % (_ADDON_NAME,
                                                                         self.heading,))
        # set text
        self.window.getControl(self.CONTROL_TEXTBOX).setText(text)
        xbmc.sleep(2000)

    def get_text(self):
        """ Get the text viewers body text from self.filename

        :return: contents of self.filename
        :rtype: str
        """
        try:
            return self.read_file(self.filename)
        except Exception as error:  # pylint: disable=broad-except
            xbmc.log(_ADDON_NAME + ': ' + str(error), xbmc.LOGERROR)
        return ''

    @staticmethod
    def read_file(filename):
        """ Read the contents of the provided file, from
        os.path.join(_ADDON_PATH, 'resources', filename)

        :param filename: name of file to read
        :type filename: str
        :return: contents of the provided file
        :rtype: str
        """
        filename = os.path.join(_ADDON_PATH, 'resources', filename)
        with closing(xbmcvfs.File(filename)) as open_file:
            contents = open_file.read()
        return contents


class WebBrowser:
    """ Display url using the default browser

    usage:
        script_path = os.path.join(_ADDON_PATH, 'resources', 'lib', 'version_check', 'viewer.py')
        xbmc.executebuiltin('RunScript(%s,%s,%s)' % (script_path, 'webbrowser', 'https://kodi.tv/'))

    :param url: url to open
    :type url: str
    """

    def __init__(self, url):
        self.url = url
        try:
            # notify user
            self.notification(_ADDON_NAME, self.url)
            xbmc.sleep(100)
            # launch url
            self.launch_url()
        except Exception as error:  # pylint: disable=broad-except
            xbmc.log(_ADDON_NAME + ': ' + str(error), xbmc.LOGERROR)

    @staticmethod
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
            icon = _ICON
        xbmcgui.Dialog().notification(heading, message, icon, time, sound)

    def launch_url(self):
        """ Open self.url in the default web browser
        """
        import webbrowser  # pylint: disable=import-outside-toplevel
        webbrowser.open(self.url)


if __name__ == '__main__':
    try:
        if sys.argv[1] == 'webbrowser':
            WebBrowser(sys.argv[2])
        else:
            Viewer(sys.argv[1], sys.argv[2])
    except Exception as err:  # pylint: disable=broad-except
        xbmc.log(_ADDON_NAME + ': ' + str(err), xbmc.LOGERROR)
