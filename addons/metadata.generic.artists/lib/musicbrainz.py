# -*- coding: utf-8 -*-

def musicbrainz_artistfind(data, artist):
    artists = []
    for item in data.get('artists',[]):
        artistdata = {}
        artistdata['artist'] = item['name']
        artistdata['thumb'] = ''
        artistdata['genre'] = ''
        artistdata['born'] = item['life-span'].get('begin', '')
        if 'type' in item:
            artistdata['type'] = item['type']
        if 'gender' in item:
            artistdata['gender'] = item['gender']
        if 'disambiguation' in item:
            artistdata['disambiguation'] = item['disambiguation']
        artistdata['mbartistid'] = item['id']
        if item.get('score',1):
            artistdata['relevance'] = str(item['score'] / 100.00)
        artists.append(artistdata)
    return artists

def musicbrainz_artistdetails(data):
    artistdata = {}
    artistdata['artist'] = data['name']
    artistdata['mbartistid'] = data['id']
    artistdata['type'] = data['type']
    artistdata['gender'] = data['gender']
    artistdata['disambiguation'] = data['disambiguation']
    if data.get('life-span','') and data.get('type',''):
        begin = data['life-span'].get('begin', '')
        end = data['life-span'].get('end', '')
        if data['type'] in ['Group', 'Orchestra', 'Choir']:
            artistdata['formed'] = begin
            artistdata['disbanded'] = end
        elif data['type'] in ['Person', 'Character']:
            artistdata['born'] = begin
            artistdata['died'] = end
    albums = []
    for item in data.get('release-groups',[]):
        albumdata = {}
        albumdata['title'] = item.get('title','')
        albumdata['year'] = item.get('first-release-date','')
        albumdata['musicbrainzreleasegroupid'] = item.get('id','')
        albums.append(albumdata)
    if albums:
        artistdata['albums'] = albums
    for item in data['relations']:
        if item['type'] == 'allmusic':
            artistdata['allmusic'] = item['url']['resource']
        elif item['type'] == 'discogs':
            dataid = item['url']['resource'].rsplit('/', 1)[1]
            artistdata['discogs'] = dataid
        elif item['type'] == 'wikidata':
            dataid = item['url']['resource'].rsplit('/', 1)[1]
            artistdata['wikidata'] = dataid
        elif item['type'] == 'wikipedia':
            dataid = item['url']['resource'].rsplit('/', 1)[1]
            artistdata['wikipedia'] = dataid
    return artistdata
