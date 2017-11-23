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

try:
    action=urllib.unquote_plus(params["action"])
except:
    pass

if action == 'find':
    try:
        artist=urllib.unquote_plus(params["artist"])
    except:
        pass

    print('Find artist with name %s' %(artist))
    liz=xbmcgui.ListItem('Demo artist 1', thumbnailImage='DefaultAlbum.png', offscreen=True)
    liz.setProperty('artist.genre', 'rock / pop')
    liz.setProperty('artist.born', '2002')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/artist", listitem=liz, isFolder=True)

    liz=xbmcgui.ListItem('Demo artist 2', thumbnailImage='DefaultAlbum.png', offscreen=True)
    liz.setProperty('artist.genre', 'classical / jazz')
    liz.setProperty('artist.born', '2012')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/artist2", listitem=liz, isFolder=True)
elif action == 'resolveid':
    liz=xbmcgui.ListItem(path='/path/to/artist2', offscreen=True)
    xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
elif action == 'getdetails':
    url=urllib.unquote_plus(params["url"])
    print('Artist with url %s' %(url))
    if url == '/path/to/artist':
        liz=xbmcgui.ListItem('Demo artist 1', offscreen=True)
        liz.setProperty('artist.musicbrainzid', '123')
        liz.setProperty('artist.genre', 'rock / pop')
        liz.setProperty('artist.styles', 'heavy / light')
        liz.setProperty('artist.moods', 'angry / happy')
        liz.setProperty('artist.years_active', '1980 / 2012')
        liz.setProperty('artist.instruments', 'guitar / drums')
        liz.setProperty('artist.born', '1/1/2001')
        liz.setProperty('artist.formed', '1980')
        liz.setProperty('artist.biography', 'Wrote lots of crap. Likes to torture cats.')
        liz.setProperty('artist.died', 'Tomorrow.')
        liz.setProperty('artist.disbanded', 'Dec 21 2012')
        liz.setProperty('artist.fanarts', '2')
        liz.setProperty('artist.fanart1.url', 'DefaultBackFanart.png')
        liz.setProperty('artist.fanart1.preview', 'DefaultBackFanart.png')
        liz.setProperty('artist.fanart1.dim', '720')
        liz.setProperty('artist.fanart2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('artist.fanart2.preview', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('artist.fanart2.dim', '1080')
        liz.setProperty('artist.albums', '2')
        liz.setProperty('artist.album1.title', 'Demo album 1')
        liz.setProperty('artist.album1.year', '2002')
        liz.setProperty('artist.album2.title', 'Demo album 2')
        liz.setProperty('artist.album2.year', '2007')
        liz.setProperty('artist.thumbs', '2')
        liz.setProperty('artist.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('artist.thumb1.aspect', '1.78')
        liz.setProperty('artist.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('artist.thumb2.aspect', '2.35')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
    if url == '/path/to/artist2':
        liz=xbmcgui.ListItem('Demo artist 2', thumbnailImage='DefaultAlbum.png', offscreen=True)
        liz.setProperty('artist.musicbrainzid', '456')
        liz.setProperty('artist.genre', 'classical / jazz')
        liz.setProperty('artist.styles', 'morbid / funny')
        liz.setProperty('artist.moods', 'fast / dance')
        liz.setProperty('artist.years_active', '1990 / 2016')
        liz.setProperty('artist.instruments', 'bass / flute')
        liz.setProperty('artist.born', '2/2/1971')
        liz.setProperty('artist.formed', '1990')
        liz.setProperty('artist.biography', 'Tortured lots of cats. Likes crap.')
        liz.setProperty('artist.died', 'Yesterday.')
        liz.setProperty('artist.disbanded', 'Nov 20 1980')
        liz.setProperty('artist.fanarts', '2')
        liz.setProperty('artist.fanart1.thumb', 'DefaultBackFanart.png')
        liz.setProperty('artist.fanart1.dim', '720')
        liz.setProperty('artist.fanart2.thumb', '/home/akva/Pictures/gnome-tshirt.png')
        liz.setProperty('artist.fanart2.dim', '1080')
        liz.setProperty('artist.albums', '2')
        liz.setProperty('artist.album1.title', 'Demo album 1')
        liz.setProperty('artist.album1.year', '2002')
        liz.setProperty('artist.album2.title', 'Demo album 2')
        liz.setProperty('artist.album2.year', '2005')
        liz.setProperty('artist.thumbs', '2')
        liz.setProperty('artist.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('artist.thumb1.aspect', '1.78')
        liz.setProperty('artist.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('artist.thumb2.aspect', '2.35')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)

xbmcplugin.endOfDirectory(int(sys.argv[1]))
