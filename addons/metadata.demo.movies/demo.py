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

    print 'Find movie with title %s from year %i' %(title, int(year))
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
        liz.setProperty('video.original_title', 'Demo måvie 1')
        liz.setProperty('video.sort_title', '2')
        liz.setProperty('video.ratings', '1')
        liz.setProperty('video.rating1.value', '5')
        liz.setProperty('video.rating1.votes', '100')
        liz.setProperty('video.user_rating', '5')
        liz.setProperty('video.top250', '3')
        liz.setProperty('video.unique_id', '123')
        liz.setProperty('video.imdb_id', '456')
        liz.setProperty('video.plot_outline', 'Outline yo')
        liz.setProperty('video.plot', 'Plot yo')
        liz.setProperty('video.tag_line', 'Tag yo')
        liz.setProperty('video.duration_minutes', '110')
        liz.setProperty('video.mpaa', 'T')
        liz.setProperty('video.trailer', '/home/akva/Videos/porn/bukkake.mkv')
        liz.setProperty('video.thumbs', '2')
        liz.setProperty('video.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.thumb1.aspect', 'poster')
        liz.setProperty('video.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.thumb2.aspect', 'banner')
        liz.setProperty('video.genre','Action / Comedy')
        liz.setProperty('video.country', 'Norway / Sweden / China')
        liz.setProperty('video.writing_credits', 'None / Want / To Admit It')
        liz.setProperty('video.director', 'spiff / spiff2')
        liz.setProperty('video.tvshow_links' ,'Demo show 1')
        liz.setProperty('video.actors', '2')
        liz.setProperty('video.actor1.name', 'spiff')
        liz.setProperty('video.actor1.role', 'himself')
        liz.setProperty('video.actor1.sort_order', '2')
        liz.setProperty('video.actor1.thumb', '/home/akva/Pictures/fish.jpg')
        liz.setProperty('video.actor1.thumb_aspect', 'banner')
        liz.setProperty('video.actor2.name', 'monkey')
        liz.setProperty('video.actor2.role', 'orange')
        liz.setProperty('video.actor2.sort_order', '1')
        liz.setProperty('video.actor1.thumb_aspect', 'poster')
        liz.setProperty('video.actor2.thumb', '/home/akva/Pictures/coffee.jpg')
        liz.setProperty('video.set_name', 'Spiffy creations')
        liz.setProperty('video.set_overview', 'Horrors created by spiff')
        liz.setProperty('video.tags', 'Very / Bad')
        liz.setProperty('video.studio', 'Studio1 / Studio2')
        liz.setProperty('video.fanarts', '2')
        liz.setProperty('video.fanart1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.fanart1.preview', 'DefaultBackFanart.png')
        liz.setProperty('video.fanart1.dim', '720')
        liz.setProperty('video.fanart2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.fanart2.preview', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.fanart2.dim', '1080')
        liz.setProperty('video.date_added', '2016-01-01')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'getartwork':
    url=urllib.unquote_plus(params["id"])
    if url == '456':
        liz=xbmcgui.ListItem('Demo movie 1', offscreen=True)
        liz.setProperty('video.thumbs', '2')
        liz.setProperty('video.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.thumb1.aspect', 'poster')
        liz.setProperty('video.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.thumb2.aspect', 'banner')
        liz.setProperty('video.fanarts', '2')
        liz.setProperty('video.fanart1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.fanart1.preview', 'DefaultBackFanart.png')
        liz.setProperty('video.fanart1.dim', '720')
        liz.setProperty('video.fanart2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.fanart2.preview', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.fanart2.dim', '1080')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)

xbmcplugin.endOfDirectory(int(sys.argv[1]))
