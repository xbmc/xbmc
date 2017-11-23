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
print(params)

try:
    action=urllib.unquote_plus(params["action"])
except:
    pass

print ("Action: "+action)

if action == 'find':
    try:
        artist=urllib.unquote_plus(params["artist"])
        album=urllib.unquote_plus(params["title"])
    except:
        pass

    print('Find album with title %s from artist %s' %(album, artist))
    liz=xbmcgui.ListItem('Demo album 1', thumbnailImage='DefaultAlbum.png', offscreen=True)
    liz.setProperty('relevance', '0.5')
    liz.setProperty('album.artist', artist)
    liz.setProperty('album.year', '2005')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/album", listitem=liz, isFolder=True)

    liz=xbmcgui.ListItem('Demo album 2', thumbnailImage='DefaultVideo.png', offscreen=True)
    liz.setProperty('relevance', '0.3')
    liz.setProperty('album.artist', 'spiff')
    liz.setProperty('album.year', '2016')
    xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="/path/to/album2", listitem=liz, isFolder=True)
elif action == 'getdetails':
    try:
        url=urllib.unquote_plus(params["url"])
    except:
        pass

    if url == '/path/to/album':
        liz=xbmcgui.ListItem('Demo album 1', offscreen=True)
        liz.setProperty('album.musicbrainzid', '123')
        liz.setProperty('album.artists', '2')
        liz.setProperty('album.artist1.name', 'Jan')
        liz.setProperty('album.artist1.musicbrainzid', '456')
        liz.setProperty('album.artist2.name', 'Banan')
        liz.setProperty('album.artist2.musicbrainzid', '789')
        liz.setProperty('album.artist_description', 'I hate this album.')
        liz.setProperty('album.genre', 'rock / pop')
        liz.setProperty('album.styles', 'light / heavy')
        liz.setProperty('album.moods', 'angry / happy')
        liz.setProperty('album.themes', 'Morbid sexual things.. And urmumz.')
        liz.setProperty('album.compiliation', 'true')
        liz.setProperty('album.review', 'Somebody should die for making this')
        liz.setProperty('album.release_date', '2005-01-02')
        liz.setProperty('album.label', 'ArtistExploitation inc')
        liz.setProperty('album.type', 'what is this?')
        liz.setProperty('album.release_type', 'single')
        liz.setProperty('album.year', '2005')
        liz.setProperty('album.rating', '2.5')
        liz.setProperty('album.userrating', '4.5')
        liz.setProperty('album.votes', '100')
        liz.setProperty('album.thumbs', '2')
        liz.setProperty('album.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('album.thumb1.aspect', '1.78')
        liz.setProperty('album.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('album.thumb2.aspect', '2.35')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)
    elif url == '/path/to/album2':
        liz=xbmcgui.ListItem('Demo album 2', offscreen=True)
        liz.setProperty('album.musicbrainzid', '123')
        liz.setProperty('album.artists', '2')
        liz.setProperty('album.artist1.name', 'Heise')
        liz.setProperty('album.artist1.musicbrainzid', '456')
        liz.setProperty('album.artist2.name', 'Kran')
        liz.setProperty('album.artist2.musicbrainzid', '789')
        liz.setProperty('album.artist_description', 'I love this album.')
        liz.setProperty('album.genre', 'classical / jazz')
        liz.setProperty('album.styles', 'yay / hurrah')
        liz.setProperty('album.moods', 'sad / excited')
        liz.setProperty('album.themes', 'Nice things.. And unicorns.')
        liz.setProperty('album.compiliation', 'false')
        liz.setProperty('album.review', 'Somebody should be rewarded for making this')
        liz.setProperty('album.release_date', '2015-01-02')
        liz.setProperty('album.label', 'Artists inc')
        liz.setProperty('album.type', 'what is that?')
        liz.setProperty('album.release_type', 'album')
        liz.setProperty('album.year', '2015')
        liz.setProperty('album.rating', '4.5')
        liz.setProperty('album.userrating', '3.5')
        liz.setProperty('album.votes', '200')
        liz.setProperty('album.thumbs', '2')
        liz.setProperty('album.thumb1.url', 'DefaultBackFanart.png')
        liz.setProperty('album.thumb1.aspect', '1.78')
        liz.setProperty('album.thumb2.url', '/home/akva/Pictures/hawaii-shirt.png')
        liz.setProperty('album.thumb2.aspect', '2.35')
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=liz)

xbmcplugin.endOfDirectory(int(sys.argv[1]))
