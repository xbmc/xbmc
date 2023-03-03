# -*- coding: utf-8 -*-

"""

    Copyright (C) 2013-2014 Team-XBMC
    Copyright (C) 2014-2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

from contextlib import closing
import json
import os
import sys

import xbmc  # pylint: disable=import-error
import xbmcvfs  # pylint: disable=import-error

from .common import ADDON_PATH


def get_installed_version():
    """ Retrieve the currently installed version

    :return: currently installed version
    :rtype: dict
    """
    query = {
        "jsonrpc": "2.0",
        "method": "Application.GetProperties",
        "params": {
            "properties": ["version", "name"]
        },
        "id": 1
    }
    json_query = xbmc.executeJSONRPC(json.dumps(query))
    if sys.version_info[0] >= 3:
        json_query = str(json_query)
    else:
        json_query = unicode(json_query, 'utf-8', errors='ignore')  # pylint: disable=undefined-variable
    json_query = json.loads(json_query)
    version_installed = []
    if 'result' in json_query and 'version' in json_query['result']:
        version_installed = json_query['result']['version']
    return version_installed


def get_version_file_list():
    """ Retrieve version lists from supplied version file (resources/versions.txt)

    :return: all provided versions
    :rtype: dict
    """
    version_file = os.path.join(ADDON_PATH, 'resources/versions.txt')
    with closing(xbmcvfs.File(version_file)) as open_file:
        data = open_file.read()

    if sys.version_info[0] >= 3:
        version_query = str(data)
    else:
        version_query = unicode(data, 'utf-8', errors='ignore')  # pylint: disable=undefined-variable
    version_query = json.loads(version_query)
    return version_query
