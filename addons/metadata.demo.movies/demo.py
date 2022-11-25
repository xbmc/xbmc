# -*- coding: utf-8 -*-
"""
    Copyright (C) 2005-2021 Team Kodi
    This file is part of Kodi - kodi.tv
    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/README.md for more information.
"""
import sys
import urllib.parse

import xbmcgui
import xbmcplugin
import xbmc


def get_params():
    param_string = sys.argv[2][1:]
    if param_string:
        return dict(urllib.parse.parse_qsl(param_string))
    return {}


params = get_params()
plugin_handle = int(sys.argv[1])
action = params.get('action')

if action == 'find':
    title = params['title']
    year = params.get('year', 'not specified')
    xbmc.log(f'Find movie with title "{title}" from year {year}', xbmc.LOGDEBUG)

    liz = xbmcgui.ListItem('Demo movie 1', offscreen=True)
    liz.setArt({'thumb': 'DefaultVideo.png'})
    liz.setProperty('relevance', '0.5')
    xbmcplugin.addDirectoryItem(handle=plugin_handle, url='/path/to/movie', listitem=liz, isFolder=True)
    liz = xbmcgui.ListItem('Demo movie 2', offscreen=True)
    liz.setArt({'thumb': 'DefaultVideo.png'})
    liz.setProperty('relevance', '0.3')
    xbmcplugin.addDirectoryItem(handle=plugin_handle, url='/path/to/movie2', listitem=liz, isFolder=True)

elif action == 'getdetails':
    url = params['url']
    if url == '/path/to/movie':
        xbmc.log('Get movie details callback', xbmc.LOGDEBUG)
        liz = xbmcgui.ListItem('Demo movie 1', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.setTitle('Demo movie 1')
        tags.setOriginalTitle('Demo måvie 1')
        tags.setSortTitle('2')
        tags.setUserRating(5)
        tags.setTop250(3)
        tags.setPlotOutline('Outline yo')
        tags.setPlot('Plot yo')
        tags.setTagLine('Tag yo')
        tags.setDuration(110)
        tags.setMpaa('T')
        tags.setTrailer('/home/akva/bunnies/unicorns.mkv')
        tags.setGenres(['Action', 'Comedy'])
        tags.setWriters(['None', 'Want', 'To Admit It'])
        tags.setDirectors(['Director 1', 'Director 2'])
        tags.setSet('Spiffy creations')
        tags.setSetOverview('Horrors created by spiff')
        tags.setStudios(['Studio1', 'Studio2'])
        tags.setDateAdded('2016-01-01')
        tags.setPremiered('2015-01-01')
        tags.setShowLinks(['Demo show 1'])
        tags.setRatings({'imdb': (9, 100000), 'themoviedb': (8.9, 1000)}, defaultrating='imdb')
        tags.setUniqueIDs({'imdb': 'tt8938399', 'tmdb': '9837493'}, defaultuniqueid='imdb')
        tags.setCast([xbmc.Actor('spiff', 'himself', order=2, thumbnail='/home/akva/Pictures/fish.jpg'),
                      xbmc.Actor('monkey', 'orange', order=1, thumbnail='/home/akva/Pictures/coffee.jpg')])
        tags.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        tags.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'},
                                {'image': '/home/akva/Pictures/hawaii-shirt.png',
                                 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=plugin_handle, succeeded=True, listitem=liz)

elif action == 'getartwork':
    url = params['id']
    if url == '456':
        xbmc.log('Get movie artworks callback', xbmc.LOGDEBUG)
        liz = xbmcgui.ListItem('Demo movie 1', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        tags.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'},
                                {'image': '/home/akva/Pictures/hawaii-shirt.png',
                                 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=plugin_handle, succeeded=True, listitem=liz)

elif action == 'nfourl':
    nfo = params["nfo"]
    xbmc.log('Find url from nfo file', xbmc.LOGDEBUG)
    liz = xbmcgui.ListItem('Demo movie 1', offscreen=True)
    xbmcplugin.addDirectoryItem(handle=plugin_handle, url='/path/to/movie1', listitem=liz, isFolder=True)

elif action is not None:
    xbmc.log(f'Action "{action}" not implemented', xbmc.LOGDEBUG)

xbmcplugin.endOfDirectory(plugin_handle)
