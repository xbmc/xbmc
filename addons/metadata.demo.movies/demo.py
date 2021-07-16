#!/usr/bin/python3
# -*- coding: UTF-8 -*-

from sys import argv
from urllib import unquote_plus
from xbmcplugin import addDirectoryItem, setResolvedUrl, endOfDirectory
from xbmcgui import ListItem

def get_params():
    param = []
    paramstring = argv[2]
    if len(paramstring) >=  2:
        params = argv[2]
        cleanedparams = params.replace('?', '')
        if (params[len(params)-1] == '/'):
            params = params[0:len(params)-2]
        pairsofparams = cleanedparams.split('&')
        param = {}
        for i in range(len(pairsofparams)):
            splitparams = {}
            splitparams = pairsofparams[i].split(' = ')
            if (len(splitparams)) == 2:
                param[splitparams[0]] = splitparams[1]
    return param


params = get_params()

action = unquote_plus(params["action"])

if action == 'find':
    year = 0
    title = unquote_plus(params["title"])
    try:
        year = int(unquote_plus(params["year"]))
    except Exception as error:
        print(f" The Error was {error}")

    print(f"Find movie with title {title} from year {year}")
    list_item = ListItem(
        'Demo movie 1', thumbnailImage = 'DefaultVideo.png', offscreen = True
    )
    list_item.setProperty('relevance', '0.5')
    addDirectoryItem(
        handle = int(argv[1]), url = "/path/to/movie",
        listitem = list_item, isFolder = True
    )
    list_item = ListItem(
        'Demo movie 2', thumbnailImage = 'DefaultVideo.png', offscreen = True
    )
    list_item.setProperty('relevance', '0.3')
    addDirectoryItem(
        handle = int(argv[1]), url = "/path/to/movie2",
        listitem = list_item, isFolder = True
    )
elif action == 'getdetails':
    url = unquote_plus(params["url"])
    if url == '/path/to/movie':
        list_item = ListItem('Demo movie 1', offscreen = True)
        list_item.setInfo('video', 
            {
                'title': 'Demo movie 1', 
                'originaltitle': 'Demo måvie 1', 
                'sorttitle': '2', 
                'userrating': 5, 
                'top250': 3, 
                'plotoutline': 'Outline yo', 
                'plot': 'Plot yo', 
                'tagline': 'Tag yo', 
                'duration': 110, 
                'mpaa': 'T', 
                'trailer': '/home/akva/bunnies/unicorns.mkv', 
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
            }
        )
                    #todo: missing actor thumb aspect
        list_item.setRating("imdb", 9, 100000, True )
        list_item.setRating("themoviedb", 8.9, 1000)
        list_item.setUniqueIDs({ 'imdb': 'tt8938399', 'tmdb' : '9837493'}, 'imdb')
        list_item.setCast(
            [
                {
                    'name': 'spiff', 'role': 'himself', 
                    'thumbnail': '/home/akva/Pictures/fish.jpg', 'order': 2
                }, {
                    'name': 'monkey', 'role': 'orange', 
                    'thumbnail': '/home/akva/Pictures/coffee.jpg', 'order': 1
                }
            ]
        )
        list_item.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        list_item.addAvailableArtwork(
            '/home/akva/Pictures/hawaii-shirt.png', 'poster'
        )
        list_item.setAvailableFanart(
            [
                {
                    'image': 'DefaultBackFanart.png', 
                    'preview': 'DefaultBackFanart.png'
                }, {
                    'image': '/home/akva/Pictures/hawaii-shirt.png', 
                    'preview': '/home/akva/Pictures/hawaii-shirt.png'
                }
            ]
        )
        setResolvedUrl(handle = int(argv[1]), succeeded = True, listitem = list_item)
elif action == 'getartwork':
    url = unquote_plus(params["id"])
    if url == '456':
        list_item = ListItem('Demo movie 1', offscreen = True)
        list_item.addAvailableArtwork('DefaultBackFanart.png', 'banner')
        list_item.addAvailableArtwork('/home/akva/Pictures/shirt.png', 'poster')
        list_item.setAvailableFanart(
            [
                {
                    'image': 'DefaultBackFanart.png', 
                    'preview': 'DefaultBackFanart.png'
                }, {
                    'image': '/home/akva/Pictures/hawaii-shirt.png', 
                    'preview': '/home/akva/Pictures/hawaii-shirt.png'
                }
            ]
        )
        setResolvedUrl(handle = int(argv[1]), succeeded = True, listitem = list_item)
elif action == 'nfourl':
    nfo = unquote_plus(params["nfo"])
    print('Find url from nfo file')
    list_item = ListItem('Demo movie 1', offscreen = True)
    addDirectoryItem(
        handle = int(argv[1]), url = "/path/to/movie1", 
        listitem = list_item, isFolder = True
    )

endOfDirectory(int(argv[1]))
