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

"""Functions to process data"""

from __future__ import absolute_import, unicode_literals

import re
import json
from xbmc import Actor, VideoStreamDetail
from collections import namedtuple
from .utils import safe_get, logger
from . import settings, api_utils

try:
    from typing import Optional, Tuple, Text, Dict, List, Any  # pylint: disable=unused-import
    from xbmcgui import ListItem  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

TMDB_PARAMS = {'api_key': settings.TMDB_CLOWNCAR, 'language': settings.LANG}
BASE_URL = 'https://api.themoviedb.org/3/{}'
FIND_URL = BASE_URL.format('find/{}')
TAG_RE = re.compile(r'<[^>]+>')

# Regular expressions are listed in order of priority.
# "TMDB" provider is preferred than other providers (IMDB and TheTVDB),
# because external providers IDs need to be converted to TMDB_ID.
SHOW_ID_REGEXPS = (
    r'(themoviedb)\.org/tv/(\d+).*/episode_group/(.*)',   # TMDB_http_link
    r'(themoviedb)\.org/tv/(\d+)',                        # TMDB_http_link
    r'(themoviedb)\.org/./tv/(\d+)',                      # TMDB_http_link
    r'(tmdb)\.org/./tv/(\d+)',                            # TMDB_http_link
    r'(imdb)\.com/.+/(tt\d+)',                            # IMDB_http_link
    r'(thetvdb)\.com.+&id=(\d+)',                         # TheTVDB_http_link
    r'(thetvdb)\.com/series/(\d+)',                       # TheTVDB_http_link
    r'(thetvdb)\.com/api/.*series/(\d+)',                 # TheTVDB_http_link
    r'(thetvdb)\.com/.*?"id":(\d+)',                      # TheTVDB_http_link
    r'<uniqueid.+?type="(tvdb|imdb)".*?>([t\d]+?)</uniqueid>'
)


SUPPORTED_ARTWORK_TYPES = {'poster', 'banner'}
IMAGE_SIZES = ('large', 'original', 'medium')
CLEAN_PLOT_REPLACEMENTS = (
    ('<b>', '[B]'),
    ('</b>', '[/B]'),
    ('<i>', '[I]'),
    ('</i>', '[/I]'),
    ('</p><p>', '[CR]'),
)
VALIDEXTIDS = ['tmdb_id', 'imdb_id', 'tvdb_id']

UrlParseResult = namedtuple(
    'UrlParseResult', ['provider', 'show_id', 'ep_grouping'])


def _clean_plot(plot):
    # type: (Text) -> Text
    """Replace HTML tags with Kodi skin tags"""
    for repl in CLEAN_PLOT_REPLACEMENTS:
        plot = plot.replace(repl[0], repl[1])
    plot = TAG_RE.sub('', plot)
    return plot


def _set_cast(cast_info, vtag):
    # type: (InfoType, ListItem) -> ListItem
    """Save cast info to list item"""
    cast = []
    for item in cast_info:
        actor = {
            'name': item['name'],
            'role': item.get('character', item.get('character_name', '')),
            'order': item['order'],
        }
        thumb = None
        if safe_get(item, 'profile_path') is not None:
            thumb = settings.IMAGEROOTURL + item['profile_path']
        cast.append(Actor(actor['name'], actor['role'], actor['order'], thumb))
    vtag.setCast(cast)


def _get_credits(show_info):
    # type: (InfoType) -> List[Text]
    """Extract show creator(s) and writer(s) from show info"""
    credits = []
    for item in show_info.get('created_by', []):
        credits.append(item['name'])
    for item in show_info.get('credits', {}).get('crew', []):
        isWriter = item.get('job', '').lower() == 'writer' or item.get(
            'department', '').lower() == 'writing'
        if isWriter and item.get('name') not in credits:
            credits.append(item['name'])
    return credits


def _get_directors(episode_info):
    # type: (InfoType) -> List[Text]
    """Extract episode writer(s) from episode info"""
    directors_ = []
    for item in episode_info.get('credits', {}).get('crew', []):
        if item.get('job') == 'Director':
            directors_.append(item['name'])
    return directors_


def _set_unique_ids(ext_ids, vtag):
    # type: (Dict, ListItem) -> ListItem
    """Extract unique ID in various online databases"""
    return_ids = {}
    for key, value in ext_ids.items():
        if key in VALIDEXTIDS and value:
            if key == 'tmdb_id':
                isTMDB = True
            else:
                isTMDB = False
            shortkey = key[:-3]
            str_value = str(value)
            vtag.setUniqueID(str_value, type=shortkey, isdefault=isTMDB)
            return_ids[shortkey] = str_value
    return return_ids


def _set_rating(the_info, vtag):
    # type: (InfoType, ListItem) -> None
    """Set show/episode rating"""
    first = True
    for rating_type in settings.RATING_TYPES:
        logger.debug('adding rating type of %s' % rating_type)
        rating = float(the_info.get('ratings', {}).get(
            rating_type, {}).get('rating', '0'))
        votes = int(the_info.get('ratings', {}).get(
            rating_type, {}).get('votes', '0'))
        logger.debug("adding rating of %s and votes of %s" %
                     (str(rating), str(votes)))
        if rating > 0:
            vtag.setRating(rating, votes=votes,
                           type=rating_type, isdefault=first)
            first = False


def _add_season_info(show_info, vtag):
    # type: (InfoType, ListItem) -> None
    """Add info for show seasons"""
    for season in show_info['seasons']:
        logger.debug('adding information for season %s to list item' %
                     season['season_number'])
        vtag.addSeason(season['season_number'],
                       safe_get(season, 'name', ''))
        for image_type, image_list in season.get('images', {}).items():
            if image_type == 'posters':
                destination = 'poster'
            else:
                destination = image_type
            for image in image_list:
                theurl, previewurl = get_image_urls(image)
                if theurl:
                    vtag.addAvailableArtwork(
                        theurl, arttype=destination, preview=previewurl, season=season['season_number'])


def get_image_urls(image):
    # type: (Dict) -> Tuple[Text, Text]
    """Get image URLs from image information"""
    if image.get('file_path', '').endswith('.svg'):
        return None, None
    if image.get('type') == 'fanarttv':
        theurl = image['file_path']
        previewurl = theurl.replace(
            '.fanart.tv/fanart/', '.fanart.tv/preview/')
    else:
        theurl = settings.IMAGEROOTURL + image['file_path']
        previewurl = settings.PREVIEWROOTURL + image['file_path']
    return theurl, previewurl


def set_show_artwork(show_info, list_item):
    # type: (InfoType, ListItem) -> ListItem
    """Set available images for a show"""
    vtag = list_item.getVideoInfoTag()
    for image_type, image_list in show_info.get('images', {}).items():
        if image_type == 'backdrops':
            fanart_list = []
            for image in image_list:
                theurl, previewurl = get_image_urls(image)
                if image.get('iso_639_1') != None and settings.CATLANDSCAPE and theurl:
                    vtag.addAvailableArtwork(
                        theurl, arttype="landscape", preview=previewurl)
                elif theurl:
                    fanart_list.append({'image': theurl})
            if fanart_list:
                list_item.setAvailableFanart(fanart_list)
        else:
            if image_type == 'posters':
                destination = 'poster'
            elif image_type == 'logos':
                destination = 'clearlogo'
            else:
                destination = image_type
            for image in image_list:
                theurl, previewurl = get_image_urls(image)
                if theurl:
                    vtag.addAvailableArtwork(
                        theurl, arttype=destination, preview=previewurl)
    return list_item


def add_main_show_info(list_item, show_info, full_info=True):
    # type: (ListItem, InfoType, bool) -> ListItem
    """Add main show info to a list item"""
    vtag = list_item.getVideoInfoTag()
    original_name = show_info.get('original_name')
    if settings.KEEPTITLE and original_name:
        showname = original_name
    else:
        showname = show_info['name']
    plot = _clean_plot(safe_get(show_info, 'overview', ''))
    vtag.setTitle(showname)
    vtag.setOriginalTitle(original_name)
    vtag.setTvShowTitle(showname)
    vtag.setPlot(plot)
    vtag.setPlotOutline(plot)
    vtag.setMediaType('tvshow')
    ext_ids = {'tmdb_id': show_info['id']}
    ext_ids.update(show_info.get('external_ids', {}))
    epguide_ids = _set_unique_ids(ext_ids, vtag)
    vtag.setEpisodeGuide(json.dumps(epguide_ids))
    if show_info.get('first_air_date'):
        vtag.setYear(int(show_info['first_air_date'][:4]))
        vtag.setPremiered(show_info['first_air_date'])
    if full_info:
        vtag.setTvShowStatus(safe_get(show_info, 'status', ''))
        genre_list = safe_get(show_info, 'genres', {})
        genres = []
        for genre in genre_list:
            genres.append(genre['name'])
        vtag.setGenres(genres)
        networks = show_info.get('networks', [])
        if networks:
            network = networks[0]
            country = network.get('origin_country', '')
        else:
            network = None
            country = None
        if network and country and settings.STUDIOCOUNTRY:
            vtag.setStudios(['{0} ({1})'.format(network['name'], country)])
        elif network:
            vtag.setStudios([network['name']])
        if country:
            vtag.setCountries([country])
        content_ratings = show_info.get(
            'content_ratings', {}).get('results', {})
        if content_ratings:
            mpaa = ''
            mpaa_backup = ''
            for content_rating in content_ratings:
                iso = content_rating.get('iso_3166_1', '').lower()
                if iso == 'us':
                    mpaa_backup = content_rating.get('rating')
                if iso == settings.CERT_COUNTRY.lower():
                    mpaa = content_rating.get('rating', '')
            if not mpaa:
                mpaa = mpaa_backup
            if mpaa:
                vtag.setMpaa(settings.CERT_PREFIX + mpaa)
        vtag.setWriters(_get_credits(show_info))
        if settings.ENABTRAILER:
            trailer = _parse_trailer(show_info.get(
                'videos', {}).get('results', {}))
            if trailer:
                vtag.setTrailer(trailer)
        list_item = set_show_artwork(show_info, list_item)
        _add_season_info(show_info, vtag)
        _set_cast(show_info['credits']['cast'], vtag)
        _set_rating(show_info, vtag)
    else:
        image = show_info.get('poster_path', '')
        if image and not image.endswith('.svg'):
            theurl = settings.IMAGEROOTURL + image
            previewurl = settings.PREVIEWROOTURL + image
            vtag.addAvailableArtwork(
                theurl, arttype='poster', preview=previewurl)
    logger.debug('adding tv show information for %s to list item' % showname)
    return list_item


def add_episode_info(list_item, episode_info, full_info=True):
    # type: (ListItem, InfoType, bool) -> ListItem
    """Add episode info to a list item"""
    title = episode_info.get('name', 'Episode ' +
                             str(episode_info['episode_number']))
    vtag = list_item.getVideoInfoTag()
    vtag.setTitle(title)
    vtag.setSeason(episode_info['season_number'])
    vtag.setEpisode(episode_info['episode_number'])
    vtag.setMediaType('episode')
    if safe_get(episode_info, 'air_date') is not None:
        vtag.setFirstAired(episode_info['air_date'])
    if full_info:
        summary = safe_get(episode_info, 'overview')
        if summary is not None:
            plot = _clean_plot(summary)
            vtag.setPlot(plot)
            vtag.setPlotOutline(plot)
        if safe_get(episode_info, 'air_date') is not None:
            vtag.setPremiered(episode_info['air_date'])
        duration = episode_info.get('runtime')
        if duration:
            videostream = VideoStreamDetail(duration=int(duration)*60)
            vtag.addVideoStream(videostream)
        _set_cast(
            episode_info['season_cast'] + episode_info['credits']['guest_stars'], vtag)
        ext_ids = {'tmdb_id': episode_info['id']}
        ext_ids.update(episode_info.get('external_ids', {}))
        _set_unique_ids(ext_ids, vtag)
        _set_rating(episode_info, vtag)
        for image in episode_info.get('images', {}).get('stills', []):
            theurl, previewurl = get_image_urls(image)
            if theurl:
                vtag.addAvailableArtwork(
                    theurl, arttype='thumb', preview=previewurl)
        vtag.setWriters(_get_credits(episode_info))
        vtag.setDirectors(_get_directors(episode_info))
    logger.debug('adding episode information for S%sE%s - %s to list item' %
                 (episode_info['season_number'], episode_info['episode_number'], title))
    return list_item


def parse_nfo_url(nfo):
    # type: (Text) -> Optional[UrlParseResult]
    """Extract show ID and named seasons from NFO file contents"""
    # work around for xbmcgui.ListItem.addSeason overwriting named seasons from NFO files
    ns_regex = r'<namedseason number="(.*)">(.*)</namedseason>'
    ns_match = re.findall(ns_regex, nfo, re.I)
    sid_match = None
    ep_grouping = None
    for regexp in SHOW_ID_REGEXPS:
        logger.debug('trying regex to match service from parsing nfo:')
        logger.debug(regexp)
        show_id_match = re.search(regexp, nfo, re.I)
        if show_id_match:
            logger.debug('match group 1: ' + show_id_match.group(1))
            logger.debug('match group 2: ' + show_id_match.group(2))
            if show_id_match.group(1) == "themoviedb" or show_id_match.group(1) == "tmdb":
                try:
                    ep_grouping = show_id_match.group(3)
                except IndexError:
                    pass
                tmdb_id = show_id_match.group(2)
            else:
                tmdb_id = _convert_ext_id(
                    show_id_match.group(1), show_id_match.group(2))
            if tmdb_id:
                logger.debug('match group 3: ' + str(ep_grouping))
                sid_match = UrlParseResult('tmdb', tmdb_id, ep_grouping)
                break
    return sid_match, ns_match


def _convert_ext_id(ext_provider, ext_id):
    # type: (Text, Text) -> Text
    """get a TMDb ID from an external ID"""
    providers_dict = {'imdb': 'imdb_id',
                      'thetvdb': 'tvdb_id',
                      'tvdb': 'tvdb_id'}
    show_url = FIND_URL.format(ext_id)
    params = TMDB_PARAMS.copy()
    provider = providers_dict.get(ext_provider)
    if provider:
        params['external_source'] = provider
        show_info = api_utils.load_info(show_url, params=params)
    else:
        show_info = None
    if show_info:
        tv_results = show_info.get('tv_results')
        if tv_results:
            return tv_results[0].get('id')
    return None


def parse_media_id(title):
    # type: (Text) -> Dict
    """get the ID from a title and return with the type"""
    title = title.lower()
    if title.startswith('tt') and title[2:].isdigit():
        # IMDB ID works alone because it is clear
        return {'type': 'imdb_id', 'title': title}
    # IMDB ID with prefix to match
    elif title.startswith('imdb/tt') and title[7:].isdigit():
        # IMDB ID works alone because it is clear
        return {'type': 'imdb_id', 'title': title[5:]}
    elif title.startswith('tmdb/') and title[5:].isdigit():  # TMDB ID
        return {'type': 'tmdb_id', 'title': title[5:]}
    elif title.startswith('tvdb/') and title[5:].isdigit():  # TVDB ID
        return {'type': 'tvdb_id', 'title': title[5:]}
    return None


def _parse_trailer(results):
    # type: (Text) -> Text
    """create a valid Tubed or YouTube plugin trailer URL"""
    if results:
        if settings.PLAYERSOPT == 'tubed':
            addon_player = 'plugin://plugin.video.tubed/?mode=play&video_id='
        elif settings.PLAYERSOPT == 'youtube':
            addon_player = 'plugin://plugin.video.youtube/?action=play_video&videoid='
        backup_keys = []
        for video_lang in [settings.LANG[0:2], 'en']:
            for result in results:
                if result.get('site') == 'YouTube' and result.get('iso_639_1') == video_lang:
                    key = result.get('key')
                    if result.get('type') == 'Trailer':
                        if _check_youtube(key):
                            # video is available and is defined as "Trailer" by TMDB. Perfect link!
                            return addon_player+key
                    else:
                        # video is available, but NOT defined as "Trailer" by TMDB. Saving it as backup in case it doesn't find any perfect link.
                        backup_keys.append(key)
            for keybackup in backup_keys:
                if _check_youtube(keybackup):
                    return addon_player+keybackup
    return None


def _check_youtube(key):
    # type: (Text) -> bool
    """check to see if the YouTube key returns a valid link"""
    chk_link = "https://www.youtube.com/watch?v="+key
    check = api_utils.load_info(chk_link, resp_type='not_json')
    if not check or "Video unavailable" in check:       # video not available
        return False
    return True
