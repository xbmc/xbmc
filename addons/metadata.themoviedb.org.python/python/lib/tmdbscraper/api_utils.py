# coding: utf-8
#
# Copyright (C) 2020, Team Kodi
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""Functions to interact with various web site APIs."""

from __future__ import absolute_import, unicode_literals

import json, xbmc
# from pprint import pformat
try: #PY2 / PY3
    from urllib2 import Request, urlopen
    from urllib2 import URLError
    from urllib import urlencode
except ImportError:
    from urllib.request import Request, urlopen
    from urllib.error import URLError
    from urllib.parse import urlencode
try:
    from typing import Text, Optional, Union, List, Dict, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

HEADERS = {}


def set_headers(headers):
    HEADERS.update(headers)


def load_info(url, params=None, default=None, resp_type = 'json'):
    # type: (Text, Optional[Dict[Text, Union[Text, List[Text]]]]) -> Union[dict, list]
    """
    Load info from external api

    :param url: API endpoint URL
    :param params: URL query params
    :default: object to return if there is an error
    :resp_type: what to return to the calling function
    :return: API response or default on error
    """
    theerror = ''
    if params:
        url = url + '?' + urlencode(params)
    xbmc.log('Calling URL "{}"'.format(url), xbmc.LOGDEBUG)
    req = Request(url, headers=HEADERS)
    try:
        response = urlopen(req)
    except URLError as e:
        if hasattr(e, 'reason'):
            theerror = {'error': 'failed to reach the remote site\nReason: {}'.format(e.reason)}
        elif hasattr(e, 'code'):
            theerror = {'error': 'remote site unable to fulfill the request\nError code: {}'.format(e.code)}
        if default is not None:
            return default
        else:
            return theerror
    if resp_type.lower() == 'json':
        resp = json.loads(response.read().decode('utf-8'))
    else:
        resp = response.read().decode('utf-8')
    # xbmc.log('the api response:\n{}'.format(pformat(resp)), xbmc.LOGDEBUG)
    return resp
