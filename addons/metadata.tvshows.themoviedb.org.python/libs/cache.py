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
#
# This is based on the metadata.tvmaze scrapper by Roman Miroshnychenko aka Roman V.M.

"""Cache-related functionality"""

from __future__ import absolute_import, unicode_literals

import os
import pickle
import xbmc
import xbmcvfs

from .utils import ADDON, logger

try:
    from typing import Optional, Text, Dict, Any  # pylint: disable=unused-import
except ImportError:
    pass


def _get_cache_directory():  # pylint: disable=missing-docstring
    # type: () -> Text
    temp_dir = xbmcvfs.translatePath('special://temp')
    cache_dir = os.path.join(temp_dir, 'scrapers', ADDON.getAddonInfo('id'))
    if not xbmcvfs.exists(cache_dir):
        xbmcvfs.mkdir(cache_dir)
    logger.debug('the cache dir is ' + cache_dir)
    return cache_dir


CACHE_DIR = _get_cache_directory()  # type: Text


def cache_show_info(show_info):
    # type: (Dict[Text, Any]) -> None
    """
    Save show_info dict to cache
    """
    file_name = str(show_info['id']) + '.pickle'
    cache = {
        'show_info': show_info
    }
    with open(os.path.join(CACHE_DIR, file_name), 'wb') as fo:
        pickle.dump(cache, fo, protocol=2)


def load_show_info_from_cache(show_id):
    # type: (Text) -> Optional[Dict[Text, Any]]
    """
    Load show info from a local cache

    :param show_id: show ID on TVmaze
    :return: show_info dict or None
    """
    file_name = str(show_id) + '.pickle'
    try:
        with open(os.path.join(CACHE_DIR, file_name), 'rb') as fo:
            load_kwargs = {}
            load_kwargs['encoding'] = 'bytes'
            cache = pickle.load(fo, **load_kwargs)
        return cache['show_info']
    except (IOError, pickle.PickleError) as exc:
        logger.debug('Cache message: {} {}'.format(type(exc), exc))
        return None
