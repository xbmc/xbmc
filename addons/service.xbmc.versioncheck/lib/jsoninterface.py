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
import sys
import xbmc
import xbmcaddon
import xbmcvfs
import lib.common
from lib.common import log

ADDONPATH    = lib.common.ADDONPATH

import json as jsoninterface

def get_installedversion():
    # retrieve current installed version
    json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "Application.GetProperties", "params": {"properties": ["version", "name"]}, "id": 1 }')
    if sys.version_info[0] >= 3:
        json_query = str(json_query)
    else:
        json_query = unicode(json_query, 'utf-8', errors='ignore')
    json_query = jsoninterface.loads(json_query)
    version_installed = []
    if 'result' in json_query and 'version' in json_query['result']:
        version_installed  = json_query['result']['version']
    return version_installed

def get_versionfilelist():
    # retrieve versionlists from supplied version file
    version_file = os.path.join(ADDONPATH, 'resources/versions.txt')
    # Eden didn't have xbmcvfs.File()
    if xbmcaddon.Addon('xbmc.addon').getAddonInfo('version') < "11.9.3":
        file = open(version_file, 'r')
    else:
        file = xbmcvfs.File(version_file)
    data = file.read()
    file.close()
    if sys.version_info[0] >= 3:
        version_query = str(data)
    else:
        version_query = unicode(data, 'utf-8', errors='ignore')
    version_query = jsoninterface.loads(version_query)
    return version_query
