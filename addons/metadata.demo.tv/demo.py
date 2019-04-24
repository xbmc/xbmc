#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import xbmcplugin,xbmcgui,xbmc,xbmcaddon
import os,sys,urllib

def get_params():
        param=[]
        paramstring=sys.argv[2]
        if len(paramstring)>=2:
                params=sys.argv[2]
                cleanedparams=params.replace('?','')
                if (params[len(params)-1]=='/'):
                        params=params[0:len(params)-2]
                pairsofparams=cleanedparams.split('&')
                param={}
                for i in range(len(pairsofparams)):
                        splitparams={}
                        splitparams=pairsofparams[i].split('=')
                        if (len(splitparams))==2:
                                param[splitparams[0]]=splitparams[1]
                                
        return param


params=get_params()

action=urllib.unquote_plus(params["action"])

if action == 'find':
    year = 0
    title=urllib.unquote_plus(params["title"])
    try:
        year=int(urllib.unquote_plus(params["year"]))
    except:
        pass

    print('Find TV show with title %s from year %i' %(title, int(year)))
    liz=xbmcgui.ListItem('Demo show 1', thumbnailImage='DefaultVideo.png', offscreen=True)
    liz.setProperty('relevance', '0.5')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/show", listitem=liz, isFolder=True)
    liz=xbmcgui.ListItem('Demo show 2', thumbnailImage='DefaultVideo.png', offscreen=True)
    liz.setProperty('relevance', '0.3')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/show2", listitem=liz, isFolder=True)
elif action == 'getdetails':
    url=urllib.unquote_plus(params["url"])
    if url == '/path/to/show':
        liz=xbmcgui.ListItem('Demo show 1', offscreen=True)
        liz.setInfo('video',
                    {'title': 'Demo show 1',
                     'originaltitle': 'Demo shåvv 1',
                     'sorttitle': '2',
                     'userrating': 5,
                     'plotoutline': 'Outline yo',
                     'plot': 'Plot yo',
                     'tagline': 'Tag yo',
                     'duration': 110,
                     'mpaa': 'T',
                     'trailer': '/home/akva/fluffy/bunnies.mkv',
                     'genre': ['Action', 'Comedy'],
                     'country': ['Norway', 'Sweden', 'China'],
                     'credits': ['None', 'Want', 'To Admit It'],
                     'director': ['spiff', 'spiff2'],
                     'studio': ['Studio1', 'Studio2'],
                     'dateadded': '2016-01-01',
                     'premiered': '2015-01-01',
                     'aired': '2007-01-01',
                     'status': 'Cancelled',
                     'episodeguide': '/path/to/show/guide',
                     'tag': ['Family', 'Mom <3']
                    })
        liz.setRating("imdb", 9, 100000, True )
        liz.setRating("tvdb", 8.9, 1000)
        liz.setUniqueIDs({ 'imdb': 'tt8938399', 'tvdb' : '9837493' }, 'tvdb')
        liz.addSeason(1, 'Horrible')
        liz.addSeason(2, 'Crap')
        liz.setCast([{'name': 'spiff', 'role': 'himself', 'thumbnail': '/home/akva/Pictures/fish.jpg', 'order': 2},
                    {'name': 'monkey', 'role': 'orange', 'thumbnail': '/home/akva/Pictures/coffee.jpg', 'order': 1}])
        liz.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        liz.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'}, 
                                {'image': '/home/akva/Pictures/hawaii-shirt.png', 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'getepisodelist':
    url=urllib.unquote_plus(params["url"])
    print('in here yo ' + url)
    if url == '/path/to/show/guide':
        liz=xbmcgui.ListItem('Demo Episode 1x1', offscreen=True)
        liz.setInfo('video',
                    {'title': 'Demo Episode 1',
                     'season': 1,
                     'episode': 1,
                     'aired': '2015-01-01'
                     })
        liz.addAvailableArtwork('/path/to/episode1','banner')
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/episode1", listitem=liz, isFolder=False)
        liz=xbmcgui.ListItem('Demo Episode 2x2', offscreen=True)
        liz.setInfo('video',
                    {'title': 'Demo Episode 2',
                     'season': 2,
                     'episode': 2,
                     'aired': '2014-01-01'
                     })
        liz.addAvailableArtwork('/path/to/episode2','banner')
        #liz.setProperty('video.sub_episode', '1')
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/episode2", listitem=liz, isFolder=False)
elif action == 'getepisodedetails':
    url=urllib.unquote_plus(params["url"])
    if url == '/path/to/episode1':
        liz=xbmcgui.ListItem('Demo Episode 1', offscreen=True)
        liz.setInfo('video',
                    {'title': 'Demo Episode 1',
                     'originaltitle': 'Demo æpisod 1x1',
                     'sorttitle': '2',
                     'episode': 1,
                     'season': 1,
                     'userrating': 5,
                     'plotoutline': 'Outline yo',
                     'plot': 'Plot yo',
                     'tagline': 'Tag yo',
                     'duration': 110,
                     'mpaa': 'T',
                     'trailer': '/home/akva/fluffy/unicorns.mkv',
                     'genre': ['Action', 'Comedy'],
                     'country': ['Norway', 'Sweden', 'China'],
                     'credits': ['None', 'Want', 'To Admit It'],
                     'director': ['spiff', 'spiff2'],
                     'studio': ['Studio1', 'Studio2'],
                     'dateadded': '2016-01-01',
                     'premiered': '2015-01-01',
                     'aired': '2007-01-01',
                     'tag': ['Family', 'Dad <3']
                    })
        liz.setRating("imdb", 9, 100000, True )
        liz.setRating("tvdb", 8.9, 1000)
        liz.setUniqueIDs({ 'tvdb': '3894', 'imdb' : 'tt384940' }, 'tvdb')
        liz.addSeason(1, 'Horrible')
        liz.addSeason(2, 'Crap')
        liz.setCast([{'name': 'spiff', 'role': 'himself', 'thumbnail': '/home/akva/Pictures/fish.jpg', 'order': 2},
                    {'name': 'monkey', 'role': 'orange', 'thumbnail': '/home/akva/Pictures/coffee.jpg', 'order': 1}])
        liz.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        liz.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'}, 
                                {'image': '/home/akva/Pictures/hawaii-shirt.png', 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
    elif url == '/path/to/episode2':
        liz=xbmcgui.ListItem('Demo Episode 2', offscreen=True)
        liz.setInfo('video',
                    {'title': 'Demo Episode 2',
                     'originaltitle': 'Demo æpisod 2x2',
                     'sorttitle': '1',
                     'episode': 2,
                     'season': 2,
                     'userrating': 8,
                     'plotoutline': 'Outline yo',
                     'plot': 'Plot yo',
                     'tagline': 'Tag yo',
                     'duration': 110,
                     'mpaa': 'T',
                     'trailer': '/home/akva/fluffy/puppies.mkv',
                     'genre': ['Action', 'Comedy'],
                     'country': ['Norway', 'Sweden', 'China'],
                     'credits': ['None', 'Want', 'To Admit It'],
                     'director': ['spiff', 'spiff2'],
                     'studio': ['Studio1', 'Studio2'],
                     'dateadded': '2016-01-01',
                     'premiered': '2015-01-01',
                     'aired': '2007-01-01',
                     'tag': ['Something', 'Else']
                    })
        liz.setRating("imdb", 7, 25457, True )
        liz.setRating("tvdb", 8.1, 5478)
        liz.setUniqueIDs({ 'tvdb': '3894', 'imdb' : 'tt384940' })
        liz.addSeason(1, 'Horrible')
        liz.addSeason(2, 'Crap')
        liz.setCast([{'name': 'spiff', 'role': 'himself', 'thumbnail': '/home/akva/Pictures/fish.jpg', 'order': 2},
                    {'name': 'monkey', 'role': 'orange', 'thumbnail': '/home/akva/Pictures/coffee.jpg', 'order': 1}])
        liz.addAvailableArtwork('DefaultBackFanart.png','banner')
        liz.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png','poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'}, 
                                {'image': '/home/akva/Pictures/hawaii-shirt.png', 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'nfourl': 
    nfo=urllib.unquote_plus(params["nfo"]) 
    print 'Find url from nfo file' 
    liz=xbmcgui.ListItem('Demo show 1', offscreen=True) 
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/show", listitem=liz, isFolder=True) 
 



xbmcplugin.endOfDirectory(int(sys.argv[1]))
