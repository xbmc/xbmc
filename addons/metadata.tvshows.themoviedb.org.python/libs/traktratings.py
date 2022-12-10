# -*- coding: UTF-8 -*-
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
# pylint: disable=missing-docstring

"""Functions to interact with Trakt API"""

from __future__ import absolute_import, unicode_literals

from . import api_utils, settings
try:
    from typing import Text, Optional, Union, List, Dict, Any  # pylint: disable=unused-import
except ImportError:
    pass


HEADERS = (
    ('User-Agent', 'Kodi TV Show scraper by Team Kodi; contact pkscout@kodi.tv'),
    ('Accept', 'application/json'),
    ('trakt-api-key', settings.TRAKT_CLOWNCAR),
    ('trakt-api-version', '2'),
    ('Content-Type', 'application/json'),
)
api_utils.set_headers(dict(HEADERS))

SHOW_URL = 'https://api.trakt.tv/shows/{}'
EP_URL = SHOW_URL + '/seasons/{}/episodes/{}/ratings'


def get_details(imdb_id, season=None, episode=None):
    # type: (Text, Text, Text) -> Dict
    """
    get the Trakt ratings

    :param imdb_id:
    :param season:
    :param episode:
    :return: trackt ratings
    """
    result = {}
    if season and episode:
        url = EP_URL.format(imdb_id, season, episode)
        params = None
    else:
        url = SHOW_URL.format(imdb_id)
        params = {'extended': 'full'}
    resp = api_utils.load_info(
        url, params=params, default={}, verboselog=settings.VERBOSELOG)
    rating = resp.get('rating')
    votes = resp.get('votes')
    if votes and rating:
        result['ratings'] = {'trakt': {'votes': votes, 'rating': rating}}
    return result
