# -*- coding: utf-8 -*-
import difflib

def discogs_artistfind(data, artist):
    artists = []
    for item in data.get('results',[]):
        artistdata = {}
        artistdata['artist'] = item['title']
        # filter inaccurate results
        match = difflib.SequenceMatcher(None, artist.lower(), item['title'].lower()).ratio()
        score = round(match, 2)
        if score > 0.90:
            artistdata['thumb'] = item['thumb']
            artistdata['genre'] = ''
            artistdata['born'] = ''
            artistdata['dcid'] = item['id']
            # discogs does not provide relevance, use our own
            artistdata['relevance'] = str(score)
            artists.append(artistdata)
    return artists

def discogs_artistdetails(data):
    artistdata = {}
    artistdata['artist'] = data['name']
    artistdata['biography'] = data['profile']
    if 'images' in data:
        thumbs = []
        for item in data['images']:
            thumbdata = {}
            thumbdata['image'] = item['uri']
            thumbdata['preview'] = item['uri150']
            thumbdata['aspect'] = 'thumb'
            thumbs.append(thumbdata)
        artistdata['thumb'] = thumbs
    return artistdata

def discogs_artistalbums(data):
    albums = []
    for item in data['releases']:
        if item['role'] == 'Main':
            albumdata = {}
            albumdata['title'] = item['title']
            albumdata['year'] = str(item.get('year', ''))
            albums.append(albumdata)
    return albums
