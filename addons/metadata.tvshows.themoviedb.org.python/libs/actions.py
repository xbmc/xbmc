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

"""Plugin route actions"""

from __future__ import absolute_import, unicode_literals

import sys
import json
import urllib.parse
import xbmcgui
import xbmcplugin
from . import tmdb, data_utils
from .utils import logger, safe_get
try:
    from typing import Optional, Text, Union, ByteString  # pylint: disable=unused-import
except ImportError:
    pass

HANDLE = int(sys.argv[1])  # type: int


def find_show(title, year=None):
    # type: (Union[Text, bytes], Optional[Text]) -> None
    """Find a show by title"""
    if not isinstance(title, str):
        title = title.decode('utf-8')
    logger.debug('Searching for TV show {} ({})'.format(title, year))
    search_results = tmdb.search_show(title, year)
    for search_result in search_results:
        show_name = search_result['name']
        if safe_get(search_result, 'first_air_date') is not None:
            show_name += ' ({})'.format(search_result['first_air_date'][:4])
        list_item = xbmcgui.ListItem(show_name, offscreen=True)
        show_info = search_result
        list_item = data_utils.add_main_show_info(
            list_item, show_info, full_info=False)
        # Below "url" is some unique ID string (may be an actual URL to a show page)
        # that is used to get information about a specific TV show.
        xbmcplugin.addDirectoryItem(
            HANDLE,
            url=str(search_result['id']),
            listitem=list_item,
            isFolder=True
        )


def get_show_id_from_nfo(nfo):
    # type: (Text) -> None
    """
    Get show ID by NFO file contents

    This function is called first instead of find_show
    if a NFO file is found in a TV show folder.

    :param nfo: the contents of a NFO file
    """
    if isinstance(nfo, bytes):
        nfo = nfo.decode('utf-8', 'replace')
    logger.debug('Parsing NFO file:\n{}'.format(nfo))
    parse_result, named_seasons = data_utils.parse_nfo_url(nfo)
    if parse_result:
        if parse_result.provider == 'themoviedb':
            show_info = tmdb.load_show_info(
                parse_result.show_id, ep_grouping=parse_result.ep_grouping, named_seasons=named_seasons)
        else:
            show_info = None
        if show_info is not None:
            list_item = xbmcgui.ListItem(show_info['name'], offscreen=True)
            # "url" is some string that unique identifies a show.
            # It may be an actual URL of a TV show page.
            xbmcplugin.addDirectoryItem(
                HANDLE,
                url=str(show_info['id']),
                listitem=list_item,
                isFolder=True
            )


def get_details(show_id):
    # type: (Text) -> None
    """Get details about a specific show"""
    logger.debug('Getting details for show id {}'.format(show_id))
    show_info = tmdb.load_show_info(show_id)
    if show_info is not None:
        list_item = xbmcgui.ListItem(show_info['name'], offscreen=True)
        list_item = data_utils.add_main_show_info(
            list_item, show_info, full_info=True)
        xbmcplugin.setResolvedUrl(HANDLE, True, list_item)
    else:
        xbmcplugin.setResolvedUrl(
            HANDLE, False, xbmcgui.ListItem(offscreen=True))


def get_episode_list(show_ids):  # pylint: disable=missing-docstring
    # type: (Text) -> None
    # Kodi has a bug: when a show directory contains an XML NFO file with
    # episodeguide URL, that URL is always passed here regardless of
    # the actual parsing result in get_show_from_nfo()
    # so much of this weird logic is to deal with that
    try:
        all_ids = json.loads(show_ids)
        show_id = all_ids.get('tmdb')
        if not show_id:
            for key, value in all_ids.items():
                show_id = str(data_utils._convert_ext_id(key, value))
                if show_id:
                    break
            if not show_id:
                show_id = str(show_ids)
    except (ValueError, AttributeError):
        show_id = str(show_ids)
        if show_id.isdigit():
            logger.error(
                'using deprecated episodeguide format, this show should be refreshed or rescraped')
    if not show_id:
        raise RuntimeError(
            'No TMDb TV show id found in episode guide, this show should be refreshed or rescraped')
    elif not show_id.isdigit():
        parse_result, named_seasons = data_utils.parse_nfo_url(show_id)
        if parse_result:
            show_id = parse_result.show_id
        else:
            raise RuntimeError(
                'No TMDb TV show id found in episode guide, this show should be refreshed or rescraped')
    logger.debug('Getting episode list for show id {}'.format(show_id))
    show_info = tmdb.load_show_info(show_id)
    if show_info is not None:
        theindex = 0
        for episode in show_info['episodes']:
            epname = episode.get('name', 'Episode ' +
                                 str(episode['episode_number']))
            list_item = xbmcgui.ListItem(epname, offscreen=True)
            list_item = data_utils.add_episode_info(
                list_item, episode, full_info=False)
            encoded_ids = urllib.parse.urlencode(
                {'show_id': str(show_info['id']), 'episode_id': str(theindex)}
            )
            theindex = theindex + 1
            # Below "url" is some unique ID string (may be an actual URL to an episode page)
            # that allows to retrieve information about a specific episode.
            url = urllib.parse.quote(encoded_ids)
            xbmcplugin.addDirectoryItem(
                HANDLE,
                url=url,
                listitem=list_item,
                isFolder=True
            )
    else:
        logger.error(
            'unable to get show information using show id {}'.format(show_id))
        logger.error('you may need to refresh the show to get a valid show id')


def get_episode_details(encoded_ids):  # pylint: disable=missing-docstring
    # type: (Text) -> None
    encoded_ids = urllib.parse.unquote(encoded_ids)
    decoded_ids = dict(urllib.parse.parse_qsl(encoded_ids))
    logger.debug('Getting episode details for {}'.format(decoded_ids))
    episode_info = tmdb.load_episode_info(
        decoded_ids['show_id'], decoded_ids['episode_id']
    )
    if episode_info:
        list_item = xbmcgui.ListItem(episode_info['name'], offscreen=True)
        list_item = data_utils.add_episode_info(
            list_item, episode_info, full_info=True)
        xbmcplugin.setResolvedUrl(HANDLE, True, list_item)
    else:
        xbmcplugin.setResolvedUrl(
            HANDLE, False, xbmcgui.ListItem(offscreen=True))


def get_artwork(show_id):
    # type: (Text) -> None
    """
    Get available artwork for a show

    :param show_id: default unique ID set by setUniqueIDs() method
    """
    if not show_id:
        return
    logger.debug('Getting artwork for show ID {}'.format(show_id))
    show_info = tmdb.load_show_info(show_id)
    if show_info is not None:
        list_item = xbmcgui.ListItem(show_info['name'], offscreen=True)
        list_item = data_utils.set_show_artwork(show_info, list_item)
        xbmcplugin.setResolvedUrl(HANDLE, True, list_item)
    else:
        xbmcplugin.setResolvedUrl(
            HANDLE, False, xbmcgui.ListItem(offscreen=True))


def router(paramstring):
    # type: (Text) -> None
    """
    Route addon calls

    :param paramstring: url-encoded query string
    :raises RuntimeError: on unknown call action
    """
    params = dict(urllib.parse.parse_qsl(paramstring))
    logger.debug('Called addon with params: {}'.format(sys.argv))
    if params['action'] == 'find':
        logger.debug('performing find action')
        find_show(params['title'], params.get('year'))
    elif params['action'].lower() == 'nfourl':
        logger.debug('performing nfourl action')
        get_show_id_from_nfo(params['nfo'])
    elif params['action'] == 'getdetails':
        logger.debug('performing getdetails action')
        get_details(params['url'])
    elif params['action'] == 'getepisodelist':
        logger.debug('performing getepisodelist action')
        get_episode_list(params['url'])
    elif params['action'] == 'getepisodedetails':
        logger.debug('performing getepisodedetails action')
        get_episode_details(params['url'])
    elif params['action'] == 'getartwork':
        logger.debug('performing getartwork action')
        get_artwork(params.get('id'))
    else:
        raise RuntimeError('Invalid addon call: {}'.format(sys.argv))
    xbmcplugin.endOfDirectory(HANDLE)
