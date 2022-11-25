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

    liz = xbmcgui.ListItem('Demo show 1', offscreen=True)
    liz.setArt({'thumb': 'DefaultVideo.png'})
    liz.setProperty('relevance', '0.5')
    xbmcplugin.addDirectoryItem(handle=plugin_handle, url='/path/to/show', listitem=liz, isFolder=True)
    liz = xbmcgui.ListItem('Demo show 2', offscreen=True)
    liz.setArt({'thumb': 'DefaultVideo.png'})
    liz.setProperty('relevance', '0.3')
    xbmcplugin.addDirectoryItem(handle=plugin_handle, url='/path/to/show2', listitem=liz, isFolder=True)

elif action == 'getdetails':
    url = params['url']
    if url == '/path/to/show':
        xbmc.log('Get tv show details callback', xbmc.LOGDEBUG)
        liz = xbmcgui.ListItem('Demo show 1', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.setTitle('Demo show 1')
        tags.setOriginalTitle('Demo shåvv 1')
        tags.setSortTitle('2')
        tags.setUserRating(5)
        tags.setPlotOutline('Outline yo')
        tags.setPlot('Plot yo')
        tags.setTagLine('Tag yo')
        tags.setDuration(110)
        tags.setMpaa('T')
        tags.setTrailer('/home/akva/fluffy/bunnies.mkv')
        tags.setGenres(['Action', 'Comedy'])
        tags.setWriters(['None', 'Want', 'To Admit It'])
        tags.setDirectors(['Director 1', 'Director 2'])
        tags.setStudios(['Studio1', 'Studio2'])
        tags.setDateAdded('2016-01-01')
        tags.setPremiered('2015-01-01')
        tags.setFirstAired('2007-01-01')
        tags.setTvShowStatus('Cancelled')
        tags.setEpisodeGuide('/path/to/show/guide')
        tags.setTagLine('Family / Mom <3')
        tags.setRatings({'imdb': (9, 100000), 'tvdb': (8.9, 1000)}, defaultrating='imdb')
        tags.setUniqueIDs({'imdb': 'tt8938399', 'tmdb': '9837493'}, defaultuniqueid='tvdb')
        tags.addSeason(1, 'Beautiful')
        tags.addSeason(2, 'Sun')
        tags.setCast([xbmc.Actor('spiff', 'himself', order=2, thumbnail='/home/akva/Pictures/fish.jpg'),
                      xbmc.Actor('monkey', 'orange', order=1, thumbnail='/home/akva/Pictures/coffee.jpg')])
        tags.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        tags.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'},
                                {'image': '/home/akva/Pictures/hawaii-shirt.png',
                                 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=plugin_handle, succeeded=True, listitem=liz)

elif action == 'getepisodelist':
    url = params['url']
    xbmc.log(f'Get episode list callback "{url}"', xbmc.LOGDEBUG)
    if url == '/path/to/show/guide':
        liz = xbmcgui.ListItem('Demo Episode 1x1', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.setTitle('Demo Episode 1')
        tags.setSeason(1)
        tags.setEpisode(1)
        tags.setFirstAired('2015-01-01')
        tags.addAvailableArtwork('/path/to/episode1', 'banner')
        xbmcplugin.addDirectoryItem(handle=plugin_handle, url="/path/to/episode1", listitem=liz, isFolder=False)

        liz = xbmcgui.ListItem('Demo Episode 2x2', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.setTitle('Demo Episode 2')
        tags.setSeason(2)
        tags.setEpisode(2)
        tags.setFirstAired('2014-01-01')
        tags.addAvailableArtwork('/path/to/episode2', 'banner')
        xbmcplugin.addDirectoryItem(handle=plugin_handle, url="/path/to/episode1", listitem=liz, isFolder=False)

elif action == 'getepisodedetails':
    url = params['url']
    if url == '/path/to/episode1':
        xbmc.log('Get episode 1 details callback', xbmc.LOGDEBUG)
        liz = xbmcgui.ListItem('Demo Episode 1', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.setTitle('Demo Episode 1')
        tags.setOriginalTitle('Demo æpisod 1x1')
        tags.setSeason(1)
        tags.setEpisode(1)
        tags.setUserRating(5)
        tags.setPlotOutline('Outline yo')
        tags.setPlot('Plot yo')
        tags.setTagLine('Tag yo')
        tags.setDuration(110)
        tags.setMpaa('T')
        tags.setTrailer('/home/akva/fluffy/unicorns.mkv')
        tags.setGenres(['Action', 'Comedy'])
        tags.setCountries(['Norway', 'Sweden', 'China'])
        tags.setWriters(['None', 'Want', 'To Admit It'])
        tags.setDirectors(['Director 1', 'Director 2'])
        tags.setStudios(['Studio1', 'Studio2'])
        tags.setDateAdded('2016-01-01')
        tags.setPremiered('2015-01-01')
        tags.setFirstAired('2007-01-01')
        tags.setTagLine('Family / Dad <3')
        tags.setRatings({'imdb': (9, 100000), 'tvdb': (8.9, 1000)}, defaultrating='imdb')
        tags.setUniqueIDs({'tvdb': '3894', 'imdb': 'tt384940'}, defaultuniqueid='tvdb')
        tags.addSeason(1, 'Beautiful')
        tags.addSeason(2, 'Sun')
        tags.setCast([xbmc.Actor('spiff', 'himself', order=2, thumbnail='/home/akva/Pictures/fish.jpg'),
                      xbmc.Actor('monkey', 'orange', order=1, thumbnail='/home/akva/Pictures/coffee.jpg')])
        tags.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        tags.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'},
                                {'image': '/home/akva/Pictures/hawaii-shirt.png',
                                 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=plugin_handle, succeeded=True, listitem=liz)

    elif url == '/path/to/episode2':
        xbmc.log('Get episode 2 details callback', xbmc.LOGDEBUG)
        liz = xbmcgui.ListItem('Demo Episode 2', offscreen=True)
        tags = liz.getVideoInfoTag()
        tags.setTitle('Demo Episode 2')
        tags.setOriginalTitle('Demo æpisod 2x2')
        tags.setSortTitle('1')
        tags.setSeason(2)
        tags.setEpisode(2)
        tags.setUserRating(8)
        tags.setPlotOutline('Outline yo')
        tags.setPlot('Plot yo')
        tags.setTagLine('Tag yo')
        tags.setDuration(110)
        tags.setMpaa('T')
        tags.setTrailer('/home/akva/fluffy/puppies.mkv')
        tags.setGenres(['Action', 'Comedy'])
        tags.setCountries(['Norway', 'Sweden', 'China'])
        tags.setWriters(['None', 'Want', 'To Admit It'])
        tags.setDirectors(['Director 1', 'Director 2'])
        tags.setStudios(['Studio1', 'Studio2'])
        tags.setDateAdded('2016-01-01')
        tags.setPremiered('2015-01-01')
        tags.setFirstAired('2007-01-01')
        tags.setTagLine('Something / Else')
        tags.setRatings({'imdb': (7, 25457), 'tvdb': (8.1, 5478)}, defaultrating='imdb')
        tags.setUniqueIDs({'tvdb': '3894', 'imdb': 'tt384940'}, defaultuniqueid='tvdb')
        tags.addSeason(1, 'Beautiful')
        tags.addSeason(2, 'Sun')
        tags.setCast([xbmc.Actor('spiff', 'himself', order=2, thumbnail='/home/akva/Pictures/fish.jpg'),
                      xbmc.Actor('monkey', 'orange', order=1, thumbnail='/home/akva/Pictures/coffee.jpg')])
        tags.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        tags.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'},
                                {'image': '/home/akva/Pictures/hawaii-shirt.png',
                                 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=plugin_handle, succeeded=True, listitem=liz)

elif action == 'nfourl':
    nfo = params['nfo']
    xbmc.log('Find url from nfo file', xbmc.LOGDEBUG)
    liz = xbmcgui.ListItem('Demo show 1', offscreen=True)
    xbmcplugin.addDirectoryItem(handle=plugin_handle, url="/path/to/show", listitem=liz, isFolder=True)

elif action is not None:
    xbmc.log(f'Action "{action}" not implemented', xbmc.LOGDEBUG)

xbmcplugin.endOfDirectory(plugin_handle)
