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

    print 'Find TV show with title %s from year %i' %(title, int(year))
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
        liz.setProperty('video.original_title', 'Demo shåvv 1')
        liz.setProperty('video.sort_title', '2')
        liz.setProperty('video.ratings', '1')
        liz.setProperty('video.rating1.value', '5')
        liz.setProperty('video.rating1.votes', '100')
        liz.setProperty('video.user_rating', '5')
        liz.setProperty('video.unique_id', '123')
        liz.setProperty('video.plot_outline', 'Outline yo')
        liz.setProperty('video.plot', 'Plot yo')
        liz.setProperty('video.tag_line', 'Tag yo')
        liz.setProperty('video.duration_minutes', '110')
        liz.setProperty('video.mpaa', 'T')
        liz.setProperty('video.premiere_year', '2007')
        liz.setProperty('video.status', 'Cancelled')
        liz.setProperty('video.first_aired', '2007-01-01')
        liz.setProperty('video.trailer', '/home/akva/Videos/porn/bukkake.mkv')
        liz.setProperty('video.thumbs', '2')
        liz.setProperty('video.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.thumb1.aspect', '1.78')
        liz.setProperty('video.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.thumb2.aspect', '2.35')
        liz.setProperty('video.genre','Action / Comedy')
        liz.setProperty('video.country', 'Norway / Sweden / China')
        liz.setProperty('video.writing_credits', 'None / Want / To Admit It')
        liz.setProperty('video.director', 'spiff / spiff2')
        liz.setProperty('video.seasons', '2')
        liz.setProperty('video.season1.name', 'Horrible')
        liz.setProperty('video.season2.name', 'Crap')
        liz.setProperty('video.actors', '2')
        liz.setProperty('video.actor1.name', 'spiff')
        liz.setProperty('video.actor1.role', 'himself')
        liz.setProperty('video.actor1.sort_order', '2')
        liz.setProperty('video.actor1.thumb', '/home/akva/Pictures/fish.jpg')
        liz.setProperty('video.actor1.thumb_aspect', '1.33')
        liz.setProperty('video.actor2.name', 'monkey')
        liz.setProperty('video.actor2.role', 'orange')
        liz.setProperty('video.actor2.sort_order', '1')
        liz.setProperty('video.actor1.thumb_aspect', '1.78')
        liz.setProperty('video.actor2.thumb', '/home/akva/Pictures/coffee.jpg')
        liz.setProperty('video.tag', 'Porn / Umomz')
        liz.setProperty('video.studio', 'Studio1 / Studio2')
        liz.setProperty('video.episode_guide_url', '/path/to/show/guide')
        liz.setProperty('video.fanarts', '2')
        liz.setProperty('video.fanart1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.fanart1.preview', 'DefaultBackFanart.png')
        liz.setProperty('video.fanart1.dim', '720')
        liz.setProperty('video.fanart2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.fanart2.preview', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.fanart2.dim', '1080')
        liz.setProperty('video.date_added', '2016-01-01')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'getepisodelist':
    url=urllib.unquote_plus(params["url"])
    print 'in here yo ' + url
    if url == '/path/to/show/guide':
        liz=xbmcgui.ListItem('Demo Episode 1x1', offscreen=True)
        liz.setProperty('video.episode', '1')
        liz.setProperty('video.season', '1')
        liz.setProperty('video.aired', '2015-01-01')
        liz.setProperty('video.id', '1')
        liz.setProperty('video.url', '/path/to/episode1')
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/episode1", listitem=liz, isFolder=False)
        liz=xbmcgui.ListItem('Demo Episode 2x2', offscreen=True)
        liz.setProperty('video.episode', '2')
        #liz.setProperty('video.sub_episode', '1')
        liz.setProperty('video.season', '2')
        liz.setProperty('video.aired', '2014-01-01')
        liz.setProperty('video.id', '2')
        liz.setProperty('video.url', '/path/to/episode2')
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/episode2", listitem=liz, isFolder=False)
elif action == 'getepisodedetails':
    url=urllib.unquote_plus(params["url"])
    if url == '/path/to/episode1':
        liz=xbmcgui.ListItem('Demo Episode 1', offscreen=True)
        liz.setProperty('video.original_title', 'Demo æpisod 1x1')
        liz.setProperty('video.sort_title', '2')
        liz.setProperty('video.episode', '1')
        liz.setProperty('video.season', '1')
        liz.setProperty('video.ratings', '1')
        liz.setProperty('video.rating1.value', '5')
        liz.setProperty('video.rating1.votes', '100')
        liz.setProperty('video.user_rating', '5')
        liz.setProperty('video.unique_id', '123')
        liz.setProperty('video.plot_outline', 'Outline yo')
        liz.setProperty('video.plot', 'Plot yo')
        liz.setProperty('video.tag_line', 'Tag yo')
        liz.setProperty('video.duration_minutes', '110')
        liz.setProperty('video.mpaa', 'T')
        liz.setProperty('video.first_aired', '2007-01-01')
        liz.setProperty('video.thumbs', '2')
        liz.setProperty('video.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.thumb1.aspect', '1.78')
        liz.setProperty('video.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.thumb2.aspect', '2.35')
        liz.setProperty('video.genre','Action / Comedy')
        liz.setProperty('video.country', 'Norway / Sweden / China')
        liz.setProperty('video.writing_credits', 'None / Want / To Admit It')
        liz.setProperty('video.director', 'spiff / spiff2')
        liz.setProperty('video.actors', '2')
        liz.setProperty('video.actor1.name', 'spiff')
        liz.setProperty('video.actor1.role', 'himself')
        liz.setProperty('video.actor1.sort_order', '2')
        liz.setProperty('video.actor1.thumb', '/home/akva/Pictures/fish.jpg')
        liz.setProperty('video.actor1.thumb_aspect', 'poster')
        liz.setProperty('video.actor2.name', 'monkey')
        liz.setProperty('video.actor2.role', 'orange')
        liz.setProperty('video.actor2.sort_order', '1')
        liz.setProperty('video.actor1.thumb_aspect', '1.78')
        liz.setProperty('video.actor2.thumb', '/home/akva/Pictures/coffee.jpg')
        liz.setProperty('video.date_added', '2016-01-01')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
    elif url == '/path/to/episode2':
        liz=xbmcgui.ListItem('Demo Episode 2', offscreen=True)
        liz.setProperty('video.original_title', 'Demo æpisod 2x2')
        liz.setProperty('video.sort_title', '1')
        liz.setProperty('video.episode', '2')
        liz.setProperty('video.season', '2')
        liz.setProperty('video.ratings', '1')
        liz.setProperty('video.rating1.value', '5')
        liz.setProperty('video.rating1.votes', '100')
        liz.setProperty('video.user_rating', '5')
        liz.setProperty('video.unique_id', '123')
        liz.setProperty('video.plot_outline', 'Outline yo')
        liz.setProperty('video.plot', 'Plot yo')
        liz.setProperty('video.tag_line', 'Tag yo')
        liz.setProperty('video.duration_minutes', '110')
        liz.setProperty('video.mpaa', 'T')
        liz.setProperty('video.first_aired', '2007-01-01')
        liz.setProperty('video.thumbs', '2')
        liz.setProperty('video.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('video.thumb1.aspect', '1.78')
        liz.setProperty('video.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('video.thumb2.aspect', '2.35')
        liz.setProperty('video.genre','Action / Comedy')
        liz.setProperty('video.country', 'Norway / Sweden / China')
        liz.setProperty('video.writing_credits', 'None / Want / To Admit It')
        liz.setProperty('video.director', 'spiff / spiff2')
        liz.setProperty('video.actors', '2')
        liz.setProperty('video.actor1.name', 'spiff')
        liz.setProperty('video.actor1.role', 'himself')
        liz.setProperty('video.actor1.sort_order', '2')
        liz.setProperty('video.actor1.thumb', '/home/akva/Pictures/fish.jpg')
        liz.setProperty('video.actor1.thumb_aspect', 'poster')
        liz.setProperty('video.actor2.name', 'monkey')
        liz.setProperty('video.actor2.role', 'orange')
        liz.setProperty('video.actor2.sort_order', '1')
        liz.setProperty('video.actor1.thumb_aspect', '1.78')
        liz.setProperty('video.actor2.thumb', '/home/akva/Pictures/coffee.jpg')
        liz.setProperty('video.date_added', '2016-01-01')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)



xbmcplugin.endOfDirectory(int(sys.argv[1]))
