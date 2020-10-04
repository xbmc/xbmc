# -*- coding: utf-8 -*-

def theaudiodb_artistdetails(data):
    if data.get('artists',[]):
        item = data['artists'][0]
        artistdata = {}
        extras = []
        artistdata['artist'] = item['strArtist']
        # api inconsistent
        if item.get('intFormedYear','') and item['intFormedYear'] != '0':
            artistdata['formed'] = item['intFormedYear']
        if item.get('intBornYear','') and item['intBornYear'] != '0':
            artistdata['born'] = item['intBornYear']
        if item.get('intDiedYear','') and item['intDiedYear'] != '0':
            artistdata['died'] = item['intDiedYear']
        if item.get('strDisbanded','') and item['strDisbanded'] != '0':
            artistdata['disbanded'] = item['strDisbanded']
        if item.get('strStyle',''):
            artistdata['styles'] = item['strStyle']
        if item.get('strGenre',''):
            artistdata['genre'] = item['strGenre']
        if item.get('strMood',''):
            artistdata['moods'] = item['strMood']
        if item.get('strGender',''):
            artistdata['gender'] = item['strGender']
        if item.get('strBiographyEN',''):
            artistdata['biographyEN'] = item['strBiographyEN']
        if item.get('strBiographyDE',''):
            artistdata['biographyDE'] = item['strBiographyDE']
        if item.get('strBiographyFR',''):
            artistdata['biographyFR'] = item['strBiographyFR']
        if item.get('strBiographyCN',''):
            artistdata['biographyCN'] = item['strBiographyCN']
        if item.get('strBiographyIT',''):
            artistdata['biographyIT'] = item['strBiographyIT']
        if item.get('strBiographyJP',''):
            artistdata['biographyJP'] = item['strBiographyJP']
        if item.get('strBiographyRU',''):
            artistdata['biographyRU'] = item['strBiographyRU']
        if item.get('strBiographyES',''):
            artistdata['biographyES'] = item['strBiographyES']
        if item.get('strBiographyPT',''):
            artistdata['biographyPT'] = item['strBiographyPT']
        if item.get('strBiographySE',''):
            artistdata['biographySE'] = item['strBiographySE']
        if item.get('strBiographyNL',''):
            artistdata['biographyNL'] = item['strBiographyNL']
        if item.get('strBiographyHU',''):
            artistdata['biographyHU'] = item['strBiographyHU']
        if item.get('strBiographyNO',''):
            artistdata['biographyNO'] = item['strBiographyNO']
        if item.get('strBiographyIL',''):
            artistdata['biographyIL'] = item['strBiographyIL']
        if item.get('strBiographyPL',''):
            artistdata['biographyPL'] = item['strBiographyPL']
        if item.get('strMusicBrainzID',''):
            artistdata['mbartistid'] = item['strMusicBrainzID']
        if item.get('strArtistFanart',''):
            fanart = []
            fanartdata = {}
            fanartdata['image'] = item['strArtistFanart']
            fanartdata['preview'] = item['strArtistFanart'] + '/preview'
            fanartdata['aspect'] = 'fanart'
            fanart.append(fanartdata)
            if item['strArtistFanart2']:
                fanartdata = {}
                fanartdata['image'] = item['strArtistFanart2']
                fanartdata['preview'] = item['strArtistFanart2'] + '/preview'
                fanartdata['aspect'] = 'fanart'
                fanart.append(fanartdata)
                if item['strArtistFanart3']:
                    fanartdata = {}
                    fanartdata['image'] = item['strArtistFanart3']
                    fanartdata['preview'] = item['strArtistFanart3'] + '/preview'
                    fanartdata['aspect'] = 'fanart'
                    fanart.append(fanartdata)
            artistdata['fanart'] = fanart
        if item.get('strArtistThumb',''):
            thumbs = []
            thumbdata = {}
            thumbdata['image'] = item['strArtistThumb']
            thumbdata['preview'] = item['strArtistThumb'] + '/preview'
            thumbdata['aspect'] = 'thumb'
            thumbs.append(thumbdata)
            artistdata['thumb'] = thumbs
        if item.get('strArtistLogo',''):
            extradata = {}
            extradata['image'] = item['strArtistLogo']
            extradata['preview'] = item['strArtistLogo'] + '/preview'
            extradata['aspect'] = 'clearlogo'
            extras.append(extradata)
        if item.get('strArtistClearart',''):
            extradata = {}
            extradata['image'] = item['strArtistClearart']
            extradata['preview'] = item['strArtistClearart'] + '/preview'
            extradata['aspect'] = 'clearart'
            extras.append(extradata)
        if item.get('strArtistWideThumb',''):
            extradata = {}
            extradata['image'] = item['strArtistWideThumb']
            extradata['preview'] = item['strArtistWideThumb'] + '/preview'
            extradata['aspect'] = 'landscape'
            extras.append(extradata)
        if item.get('strArtistBanner',''):
            extradata = {}
            extradata['image'] = item['strArtistBanner']
            extradata['preview'] = item['strArtistBanner'] + '/preview'
            extradata['aspect'] = 'banner'
            extras.append(extradata)
        if extras:
            artistdata['extras'] = extras
        return artistdata

def theaudiodb_artistalbums(data):
    albums = []
    albumlist = data.get('album',[])
    if albumlist:
        for item in data.get('album',[]):
            albumdata = {}
            albumdata['title'] = item['strAlbum']
            albumdata['year'] = item.get('intYearReleased', '')
            albums.append(albumdata)
    return albums
