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

    print('Find movie with title %s from year %i' %(title, int(year)))
    liz=xbmcgui.ListItem('Demo movie 1', thumbnailImage='DefaultVideo.png', offscreen=True)
    liz.setProperty('relevance', '0.5')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/movie", listitem=liz, isFolder=True)
    liz=xbmcgui.ListItem('Demo movie 2', thumbnailImage='DefaultVideo.png', offscreen=True)
    liz.setProperty('relevance', '0.3')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/movie2", listitem=liz, isFolder=True)
elif action == 'getdetails':
    url=urllib.unquote_plus(params["url"])
    if url == '/path/to/movie':
        liz=xbmcgui.ListItem('Demo movie 1', offscreen=True)
        liz.setInfo('video',
                    {'title': 'Demo movie 1',
                     'originaltitle': 'Demo måvie 1',
                     'sorttitle': '2',
                     'userrating': 5,
                     'top250': 3,
                     'plotoutline': 'Outline yo',
                     'plot': 'Plot yo',
                     'tagline': 'Tag yo',
                     'duration': 110,
                     'mpaa': 'T',
                     'trailer': '/home/akva//porn/bukkake.mkv',
                     'genre': ['Action', 'Comedy'],
                     'country': ['Norway', 'Sweden', 'China'],
                     'credits': ['None', 'Want', 'To Admit It'],
                     'director': ['spiff', 'spiff2'],
                     'set': 'Spiffy creations',
                     'setoverview': 'Horrors created by spiff',
                     'studio': ['Studio1', 'Studio2'],
                     'dateadded': '2016-01-01',
                     'premiered': '2015-01-01',
                     'showlink': ['Demo show 1']
                    })
                    #todo: missing actor thumb aspect
        liz.setRating("imdb", 9, 100000, True )
        liz.setRating("themoviedb", 8.9, 1000)
        liz.setUniqueIDs({ 'imdb': 'tt8938399', 'tmdb' : '9837493' }, 'imdb')
        liz.setCast([{'name': 'spiff', 'role': 'himself', 'thumbnail': '/home/akva/Pictures/fish.jpg', 'order': 2},
                    {'name': 'monkey', 'role': 'orange', 'thumbnail': '/home/akva/Pictures/coffee.jpg', 'order': 1}])
        liz.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        liz.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'}, 
                                {'image': '/home/akva/Pictures/hawaii-shirt.png', 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'getartwork':
    url=urllib.unquote_plus(params["id"])
    if url == '456':
        liz=xbmcgui.ListItem('Demo movie 1', offscreen=True)
        liz.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        liz.addAvailableArtwork('/home/akva/Pictures/hawaii-shirt.png', 'poster')
        liz.setAvailableFanart([{'image': 'DefaultBackFanart.png', 'preview': 'DefaultBackFanart.png'}, 
                                {'image': '/home/akva/Pictures/hawaii-shirt.png', 'preview': '/home/akva/Pictures/hawaii-shirt.png'}])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'nfourl':
    nfo=urllib.unquote_plus(params["nfo"])
    print 'Find url from nfo file'
    liz=xbmcgui.ListItem('Demo movie 1', offscreen=True)
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/movie1", listitem=liz, isFolder=True)

xbmcplugin.endOfDirectory(int(sys.argv[1]))
