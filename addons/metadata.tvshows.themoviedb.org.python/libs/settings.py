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

import json
import sys
import urllib.parse
from .utils import logger
from . import api_utils
from xbmcaddon import Addon
from datetime import datetime, timedelta


TMDB_CLOWNCAR = 'af3a53eb387d57fc935e9128468b1899'
FANARTTV_CLOWNCAR = 'b018086af0e1478479adfc55634db97d'
TRAKT_CLOWNCAR = '90901c6be3b2de5a4fa0edf9ab5c75e9a5a0fef2b4ee7373d8b63dcf61f95697'
MAXIMAGES = 200
FANARTTV_MAPPING = {'showbackground': 'backdrops',
                    'tvposter': 'posters',
                    'tvbanner': 'banner',
                    'hdtvlogo': 'clearlogo',
                    'clearlogo': 'clearlogo',
                    'hdclearart': 'clearart',
                    'clearart': 'clearart',
                    'tvthumb': 'landscape',
                    'characterart': 'characterart',
                    'seasonposter': 'seasonposters',
                    'seasonbanner': 'seasonbanner',
                    'seasonthumb': 'seasonlandscape'
                    }


def _get_date_numeric(datetime_):
    return (datetime_ - datetime(1970, 1, 1)).total_seconds()


def _get_configuration():
    addon = Addon()
    logger.debug('getting configuration details')
    return api_utils.load_info('https://api.themoviedb.org/3/configuration', params={'api_key': TMDB_CLOWNCAR}, verboselog=addon.getSettingBool('verboselog'))


def loadBaseUrls():
    addon = Addon()
    image_root_url = addon.getSettingString('originalUrl')
    preview_root_url = addon.getSettingString('previewUrl')
    last_updated = addon.getSettingString('lastUpdated')
    if not image_root_url or not preview_root_url or not last_updated or \
            float(last_updated) < _get_date_numeric(datetime.now() - timedelta(days=30)):
        conf = _get_configuration()
        if conf:
            image_root_url = conf['images']['secure_base_url'] + 'original'
            preview_root_url = conf['images']['secure_base_url'] + 'w780'
            addon.setSetting('originalUrl', image_root_url)
            addon.setSetting('previewUrl', preview_root_url)
            addon.setSetting('lastUpdated', str(
                _get_date_numeric(datetime.now())))
    return image_root_url, preview_root_url


def getSourceSettings():
    addon = Addon()
    settings = {}
    logger.debug('Got settings of: {}'.format(sys.argv[2]))
    try:
        source_params = dict(urllib.parse.parse_qsl(sys.argv[2]))
    except IndexError:
        source_params = {}
    source_settings = json.loads(source_params.get('pathSettings', '{}'))
    settings["KEEPTITLE"] = source_settings.get(
        'keeporiginaltitle', addon.getSettingBool('keeporiginaltitle'))
    settings["CATLANDSCAPE"] = source_settings.get('cat_landscape', True)
    settings["STUDIOCOUNTRY"] = source_settings.get('studio_country', False)
    settings["ENABTRAILER"] = source_settings.get(
        'enab_trailer', addon.getSettingBool('enab_trailer'))
    settings["PLAYERSOPT"] = source_settings.get(
        'players_opt', addon.getSettingString('players_opt')).lower()
    settings["VERBOSELOG"] = source_settings.get(
        'verboselog', addon.getSettingBool('verboselog'))
    settings["CERT_COUNTRY"] = source_settings.get(
        'tmdbcertcountry', addon.getSettingString('tmdbcertcountry')).lower()
    settings["SAVETAGS"] = source_settings.get(
        'keywordsastags', addon.getSettingBool('keywordsastags'))
    settings["LANG_DETAILS"] = source_settings.get(
        'languageDetails', addon.getSettingString('languageDetails'))
    if source_settings.get('usedifferentlangforimages', addon.getSettingBool('usedifferentlangforimages')):
        settings["LANG_IMAGES"] = source_settings.get(
            'languageImages', addon.getSettingString('languageImages'))
    else:
        settings["LANG_IMAGES"] = settings["LANG_DETAILS"]
    if source_settings.get('usecertprefix', addon.getSettingBool('usecertprefix')):
        settings["CERT_PREFIX"] = source_settings.get(
            'certprefix', addon.getSettingString('certprefix'))
    else:
        settings["CERT_PREFIX"] = ''
    primary_rating = source_settings.get(
        'ratings', addon.getSettingString('ratings')).lower()
    RATING_TYPES = [primary_rating]
    if source_settings.get('imdbanyway', addon.getSettingBool('imdbanyway')) and primary_rating != 'imdb':
        RATING_TYPES.append('imdb')
    if source_settings.get('traktanyway', addon.getSettingBool('traktanyway')) and primary_rating != 'trakt':
        RATING_TYPES.append('trakt')
    if source_settings.get('tmdbanyway', addon.getSettingBool('tmdbanyway')) and primary_rating != 'tmdb':
        RATING_TYPES.append('tmdb')
    settings["RATING_TYPES"] = RATING_TYPES
    settings["FANARTTV_ENABLE"] = source_settings.get(
        'enable_fanarttv', addon.getSettingBool('enable_fanarttv'))
    settings["FANARTTV_CLIENTKEY"] = source_settings.get(
        'fanarttv_clientkey', addon.getSettingString('fanarttv_clientkey'))
    logger.debug('Sending back settings of: {}'.format(settings))
    return settings
