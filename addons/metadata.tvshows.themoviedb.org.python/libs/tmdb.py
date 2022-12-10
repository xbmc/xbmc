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

"""Functions to interact with TMDb API"""

from __future__ import absolute_import, unicode_literals

import unicodedata
from math import floor
from pprint import pformat
from . import cache, data_utils, api_utils, settings, imdbratings, traktratings
from .utils import logger
try:
    from typing import Text, Optional, Union, List, Dict, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

HEADERS = (
    ('User-Agent', 'Kodi TV Show scraper by Team Kodi; contact pkscout@kodi.tv'),
    ('Accept', 'application/json'),
)
api_utils.set_headers(dict(HEADERS))

TMDB_PARAMS = {'api_key': settings.TMDB_CLOWNCAR, 'language': settings.LANG}
BASE_URL = 'https://api.themoviedb.org/3/{}'
EPISODE_GROUP_URL = BASE_URL.format('tv/episode_group/{}')
SEARCH_URL = BASE_URL.format('search/tv')
FIND_URL = BASE_URL.format('find/{}')
SHOW_URL = BASE_URL.format('tv/{}')
SEASON_URL = BASE_URL.format('tv/{}/season/{}')
EPISODE_URL = BASE_URL.format('tv/{}/season/{}/episode/{}')
FANARTTV_URL = 'https://webservice.fanart.tv/v3/tv/{}'
FANARTTV_PARAMS = {'api_key': settings.FANARTTV_CLOWNCAR}
if settings.FANARTTV_CLIENTKEY:
    FANARTTV_PARAMS['client_key'] = settings.FANARTTV_CLIENTKEY


def search_show(title, year=None):
    # type: (Text, Text) -> List
    """
    Search for a single TV show

    :param title: TV show title to search
    : param year: the year to search (optional)
    :return: a list with found TV shows
    """
    params = TMDB_PARAMS.copy()
    results = []
    ext_media_id = data_utils.parse_media_id(title)
    if ext_media_id:
        logger.debug('using %s of %s to find show' %
                     (ext_media_id['type'], ext_media_id['title']))
        if ext_media_id['type'] == 'tmdb_id':
            search_url = SHOW_URL.format(ext_media_id['title'])
        else:
            search_url = FIND_URL.format(ext_media_id['title'])
            params['external_source'] = ext_media_id['type']
    else:
        logger.debug('using title of %s to find show' % title)
        search_url = SEARCH_URL
        params['query'] = unicodedata.normalize('NFKC', title)
        if year:
            params['first_air_date_year'] = str(year)
    resp = api_utils.load_info(
        search_url, params=params, verboselog=settings.VERBOSELOG)
    if resp is not None:
        if ext_media_id:
            if ext_media_id['type'] == 'tmdb_id':
                if resp.get('success') == 'false':
                    results = []
                else:
                    results = [resp]
            else:
                results = resp.get('tv_results', [])
        else:
            results = resp.get('results', [])
    return results


def load_episode_list(show_info, season_map, ep_grouping):
    # type: (InfoType, Dict, Text) -> Optional[InfoType]
    """get the IMDB ratings details"""
    """Load episode list from themoviedb.org API"""
    episode_list = []
    if ep_grouping is not None:
        logger.debug(
            'Getting episodes with episode grouping of ' + ep_grouping)
        episode_group_url = EPISODE_GROUP_URL.format(ep_grouping)
        custom_order = api_utils.load_info(
            episode_group_url, params=TMDB_PARAMS, verboselog=settings.VERBOSELOG)
        if custom_order is not None:
            show_info['seasons'] = []
            for custom_season in custom_order.get('groups', []):
                season_episodes = []
                try:
                    current_season = season_map.get(
                        str(custom_season['episodes'][0]['season_number']), {}).copy()
                except IndexError:
                    continue
                current_season['name'] = custom_season['name']
                current_season['season_number'] = custom_season['order']
                for episode in custom_season['episodes']:
                    episode['org_seasonnum'] = episode['season_number']
                    episode['org_epnum'] = episode['episode_number']
                    episode['season_number'] = custom_season['order']
                    episode['episode_number'] = episode['order'] + 1
                    season_episodes.append(episode)
                    episode_list.append(episode)
                current_season['episodes'] = season_episodes
                show_info['seasons'].append(current_season)
    else:
        logger.debug('Getting episodes from standard season list')
        show_info['seasons'] = []
        for key, value in season_map.items():
            show_info['seasons'].append(value)
        for season in show_info.get('seasons', []):
            for episode in season.get('episodes', []):
                episode['org_seasonnum'] = episode['season_number']
                episode['org_epnum'] = episode['episode_number']
                episode_list.append(episode)
    show_info['episodes'] = episode_list
    return show_info


def load_show_info(show_id, ep_grouping=None, named_seasons=None):
    # type: (Text, Text, Dict) -> Optional[InfoType]
    """
    Get full info for a single show

    :param show_id: themoviedb.org show ID
    :param ep_grouping: the episode group from TMDb
    :param named_seasons: the named seasons from the NFO file
    :return: show info or None
    """
    if named_seasons == None:
        named_seasons = []
    show_info = cache.load_show_info_from_cache(show_id)
    if show_info is None:
        logger.debug('no cache file found, loading from scratch')
        show_url = SHOW_URL.format(show_id)
        params = TMDB_PARAMS.copy()
        params['append_to_response'] = 'credits,content_ratings,external_ids,images,videos'
        params['include_image_language'] = '%s,en,null' % settings.LANG[0:2]
        params['include_video_language'] = '%s,en,null' % settings.LANG[0:2]
        show_info = api_utils.load_info(
            show_url, params=params, verboselog=settings.VERBOSELOG)
        if show_info is None:
            return None
        if show_info['overview'] == '' and settings.LANG != 'en-US':
            params['language'] = 'en-US'
            del params['append_to_response']
            show_info_backup = api_utils.load_info(
                show_url, params=params, verboselog=settings.VERBOSELOG)
            if show_info_backup is not None:
                show_info['overview'] = show_info_backup.get('overview', '')
            params['language'] = settings.LANG
        season_map = {}
        params['append_to_response'] = 'credits,images'
        for season in show_info.get('seasons', []):
            season_url = SEASON_URL.format(
                show_id, season.get('season_number', 0))
            season_info = api_utils.load_info(
                season_url, params=params, default={}, verboselog=settings.VERBOSELOG)
            if (season_info.get('overview', '') == '' or season_info.get('name', '').lower().startswith('season')) and settings.LANG != 'en-US':
                params['language'] = 'en-US'
                season_info_backup = api_utils.load_info(
                    season_url, params=params, default={}, verboselog=settings.VERBOSELOG)
                params['language'] = settings.LANG
                if season_info.get('overview', '') == '':
                    season_info['overview'] = season_info_backup.get(
                        'overview', '')
                if season_info.get('name', '').lower().startswith('season'):
                    season_info['name'] = season_info_backup.get('name', '')
            # this is part of a work around for xbmcgui.ListItem.addSeasons() not respecting NFO file information
            for named_season in named_seasons:
                if str(named_season[0]) == str(season.get('season_number')):
                    logger.debug('adding season name of %s from named seasons in NFO for season %s' % (
                        named_season[1], season['season_number']))
                    season_info['name'] = named_season[1]
                    break
            # end work around
            season_info['images'] = _sort_image_types(
                season_info.get('images', {}))
            season_map[str(season.get('season_number', 0))] = season_info
        show_info = load_episode_list(show_info, season_map, ep_grouping)
        show_info['ratings'] = load_ratings(show_info)
        show_info = load_fanarttv_art(show_info)
        show_info['images'] = _sort_image_types(show_info.get('images', {}))
        show_info = trim_artwork(show_info)
        cast_check = []
        cast = []
        for season in reversed(show_info.get('seasons', [])):
            for cast_member in season.get('credits', {}).get('cast', []):
                if cast_member.get('name', '') not in cast_check:
                    cast.append(cast_member)
                    cast_check.append(cast_member.get('name', ''))
        show_info['credits']['cast'] = cast
        logger.debug('saving show info to the cache')
        if settings.VERBOSELOG:
            logger.debug(format(pformat(show_info)))
        cache.cache_show_info(show_info)
    else:
        logger.debug('using cached show info')
    return show_info


def load_episode_info(show_id, episode_id):
    # type: (Text, Text) -> Optional[InfoType]
    """
    Load episode info

    :param show_id:
    :param episode_id:
    :return: episode info or None
    """
    show_info = load_show_info(show_id)
    if show_info is not None:
        try:
            episode_info = show_info['episodes'][int(episode_id)]
        except KeyError:
            return None
        # this ensures we are using the season/ep from the episode grouping if provided
        ep_url = EPISODE_URL.format(
            show_info['id'], episode_info['org_seasonnum'], episode_info['org_epnum'])
        params = TMDB_PARAMS.copy()
        params['append_to_response'] = 'credits,external_ids,images'
        params['include_image_language'] = '%s,en,null' % settings.LANG[0:2]
        ep_return = api_utils.load_info(
            ep_url, params=params, verboselog=settings.VERBOSELOG)
        if ep_return is None:
            return None
        bad_return_name = False
        bad_return_overview = False
        check_name = ep_return.get('name')
        if check_name == None:
            bad_return_name = True
            ep_return['name'] = 'Episode ' + \
                str(episode_info['episode_number'])
        elif check_name.lower().startswith('episode') or check_name == '':
            bad_return_name = True
        if ep_return.get('overview', '') == '':
            bad_return_overview = True
        if (bad_return_overview or bad_return_name) and settings.LANG != 'en-US':
            params['language'] = 'en-US'
            del params['append_to_response']
            ep_return_backup = api_utils.load_info(
                ep_url, params=params, verboselog=settings.VERBOSELOG)
            if ep_return_backup is not None:
                if bad_return_overview:
                    ep_return['overview'] = ep_return_backup.get(
                        'overview', '')
                if bad_return_name:
                    ep_return['name'] = ep_return_backup.get(
                        'name', 'Episode ' + str(episode_info['episode_number']))
        ep_return['images'] = _sort_image_types(ep_return.get('images', {}))
        ep_return['season_number'] = episode_info['season_number']
        ep_return['episode_number'] = episode_info['episode_number']
        ep_return['org_seasonnum'] = episode_info['org_seasonnum']
        ep_return['org_epnum'] = episode_info['org_epnum']
        ep_return['ratings'] = load_ratings(
            ep_return, show_imdb_id=show_info.get('external_ids', {}).get('imdb_id'))
        for season in show_info.get('seasons', []):
            if season.get('season_number') == episode_info['season_number']:
                ep_return['season_cast'] = season.get(
                    'credits', {}).get('cast', [])
                break
        show_info['episodes'][int(episode_id)] = ep_return
        cache.cache_show_info(show_info)
        return ep_return
    return None


def load_ratings(the_info, show_imdb_id=''):
    # type: (InfoType, Text) -> Dict
    """
    Load the ratings for the show/episode

    :param the_info: show or episode info
    :param show_imdb_id: show IMDB
    :return: ratings or empty dict
    """
    ratings = {}
    imdb_id = the_info.get('external_ids', {}).get('imdb_id')
    for rating_type in settings.RATING_TYPES:
        logger.debug('setting rating using %s' % rating_type)
        if rating_type == 'tmdb':
            ratings['tmdb'] = {'votes': the_info['vote_count'],
                               'rating': the_info['vote_average']}
        elif rating_type == 'imdb' and imdb_id:
            imdb_rating = imdbratings.get_details(imdb_id).get('ratings')
            if imdb_rating:
                ratings.update(imdb_rating)
        elif rating_type == 'trakt':
            if show_imdb_id:
                season = the_info['org_seasonnum']
                episode = the_info['org_epnum']
                resp = traktratings.get_details(
                    show_imdb_id, season=season, episode=episode)
            else:
                resp = traktratings.get_details(imdb_id)
            trakt_rating = resp.get('ratings')
            if trakt_rating:
                ratings.update(trakt_rating)
    logger.debug('returning ratings of\n{}'.format(pformat(ratings)))
    return ratings


def load_fanarttv_art(show_info):
    # type: (InfoType) -> Optional[InfoType]
    """
    Add fanart.tv images for a show

    :param show_info: the current show info
    :return: show info
    """
    tvdb_id = show_info.get('external_ids', {}).get('tvdb_id')
    if tvdb_id and settings.FANARTTV_ENABLE:
        fanarttv_url = FANARTTV_URL.format(tvdb_id)
        artwork = api_utils.load_info(
            fanarttv_url, params=FANARTTV_PARAMS, verboselog=settings.VERBOSELOG)
        if artwork is None:
            return show_info
        for fanarttv_type, tmdb_type in settings.FANARTTV_MAPPING.items():
            if not show_info['images'].get(tmdb_type) and not tmdb_type.startswith('season'):
                show_info['images'][tmdb_type] = []
            for item in artwork.get(fanarttv_type, []):
                lang = item.get('lang')
                if lang == '' or lang == '00':
                    lang = None
                filepath = ''
                if lang is None or lang == settings.LANG[0:2] or lang == 'en':
                    filepath = item.get('url')
                if filepath:
                    if tmdb_type.startswith('season'):
                        image_type = tmdb_type[6:]
                        for s in range(len(show_info.get('seasons', []))):
                            season_num = show_info['seasons'][s]['season_number']
                            artseason = item.get('season', '')
                            if not show_info['seasons'][s].get('images'):
                                show_info['seasons'][s]['images'] = {}
                            if not show_info['seasons'][s]['images'].get(image_type):
                                show_info['seasons'][s]['images'][image_type] = []
                            if artseason == '' or artseason == str(season_num):
                                show_info['seasons'][s]['images'][image_type].append(
                                    {'file_path': filepath, 'type': 'fanarttv', 'iso_639_1': lang})
                    else:
                        show_info['images'][tmdb_type].append(
                            {'file_path': filepath, 'type': 'fanarttv', 'iso_639_1': lang})
    return show_info


def trim_artwork(show_info):
    # type: (InfoType) -> Optional[InfoType]
    """
    Trim artwork to keep the text blob below 65K characters

    :param show_info: the current show info
    :return: show info
    """
    image_counts = {}
    image_total = 0
    backdrops_total = 0
    for image_type, image_list in show_info.get('images', {}).items():
        total = len(image_list)
        if image_type == 'backdrops':
            backdrops_total = backdrops_total + total
        else:
            image_counts[image_type] = {'total': total}
            image_total = image_total + total
    for season in show_info.get('seasons', []):
        for image_type, image_list in season.get('images', {}).items():
            total = len(image_list)
            thetype = '%s_%s' % (str(season['season_number']), image_type)
            image_counts[thetype] = {'total': total}
            image_total = image_total + total
    if image_total <= settings.MAXIMAGES and backdrops_total <= settings.MAXIMAGES:
        return show_info
    if backdrops_total > settings.MAXIMAGES:
        logger.error('there are %s fanart images' % str(backdrops_total))
        logger.error('that is more than the max of %s, image results will be trimmed to the max' % str(
            settings.MAXIMAGES))
        reduce = -1 * (backdrops_total - settings.MAXIMAGES)
        del show_info['images']['backdrops'][reduce:]
    if image_total > settings.MAXIMAGES:
        reduction = (image_total - settings.MAXIMAGES)/image_total
        logger.error('there are %s non-fanart images' % str(image_total))
        logger.error('that is more than the max of %s, image results will be trimmed by %s' % (
            str(settings.MAXIMAGES), str(reduction)))
        for key, value in image_counts.items():
            image_counts[key]['reduce'] = -1 * \
                int(floor(value['total'] * reduction))
            logger.debug('%s: %s' % (key, pformat(image_counts[key])))
        for image_type, image_list in show_info.get('images', {}).items():
            if image_type == 'backdrops':
                continue  # already handled backdrops above
            reduce = image_counts[image_type]['reduce']
            if reduce != 0:
                del show_info['images'][image_type][reduce:]
        for s in range(len(show_info.get('seasons', []))):
            for image_type, image_list in show_info['seasons'][s].get('images', {}).items():
                thetype = '%s_%s' % (
                    str(show_info['seasons'][s]['season_number']), image_type)
                reduce = image_counts[thetype]['reduce']
                if reduce != 0:
                    del show_info['seasons'][s]['images'][image_type][reduce:]
    return show_info


def _sort_image_types(imagelist):
    # type: (Dict) -> Dict
    """
    sort the images by language

    :param imagelist:
    :return: imagelist
    """
    for image_type, images in imagelist.items():
        imagelist[image_type] = _image_sort(images, image_type)
    return imagelist


def _image_sort(images, image_type):
    # type: (List, Text) -> List
    """
    sort the images by language

    :param images:
    :param image_type:
    :return: list of images
    """
    lang_pref = []
    lang_null = []
    lang_en = []
    firstimage = True
    for image in images:
        image_lang = image.get('iso_639_1')
        if image_lang == settings.LANG[0:2]:
            lang_pref.append(image)
        elif image_lang == 'en':
            lang_en.append(image)
        else:
            if firstimage:
                lang_pref.append(image)
            else:
                lang_null.append(image)
        firstimage = False
    if image_type == 'posters':
        return lang_pref + lang_en + lang_null
    else:
        return lang_pref + lang_null + lang_en
