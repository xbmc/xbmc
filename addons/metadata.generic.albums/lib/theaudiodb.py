# -*- coding: utf-8 -*-

def theaudiodb_albumdetails(data):
    if data.get('album'):
        item = data['album'][0]
        albumdata = {}
        albumdata['album'] = item['strAlbum']
        if item.get('intYearReleased',''):
            albumdata['year'] = item['intYearReleased']
        if item.get('strStyle',''):
            albumdata['styles'] = item['strStyle']
        if item.get('strGenre',''):
            albumdata['genre'] = item['strGenre']
        if item.get('strLabel',''):
            albumdata['label'] = item['strLabel']
        if item.get('strReleaseFormat',''):
            albumdata['type'] = item['strReleaseFormat']
        if item.get('intScore',''):
            albumdata['rating'] = str(int(float(item['intScore']) + 0.5))
        if item.get('intScoreVotes',''):
            albumdata['votes'] = item['intScoreVotes']
        if item.get('strMood',''):
            albumdata['moods'] = item['strMood']
        if item.get('strTheme',''):
            albumdata['themes'] = item['strTheme']
        if item.get('strMusicBrainzID',''):
            albumdata['mbreleasegroupid'] = item['strMusicBrainzID']
        # api inconsistent
        if item.get('strDescription',''):
            albumdata['descriptionEN'] = item['strDescription']
        elif item.get('strDescriptionEN',''):
            albumdata['descriptionEN'] = item['strDescriptionEN']
        if item.get('strDescriptionDE',''):
            albumdata['descriptionDE'] = item['strDescriptionDE']
        if item.get('strDescriptionFR',''):
            albumdata['descriptionFR'] = item['strDescriptionFR']
        if item.get('strDescriptionCN',''):
            albumdata['descriptionCN'] = item['strDescriptionCN']
        if item.get('strDescriptionIT',''):
            albumdata['descriptionIT'] = item['strDescriptionIT']
        if item.get('strDescriptionJP',''):
            albumdata['descriptionJP'] = item['strDescriptionJP']
        if item.get('strDescriptionRU',''):
            albumdata['descriptionRU'] = item['strDescriptionRU']
        if item.get('strDescriptionES',''):
            albumdata['descriptionES'] = item['strDescriptionES']
        if item.get('strDescriptionPT',''):
            albumdata['descriptionPT'] = item['strDescriptionPT']
        if item.get('strDescriptionSE',''):
            albumdata['descriptionSE'] = item['strDescriptionSE']
        if item.get('strDescriptionNL',''):
            albumdata['descriptionNL'] = item['strDescriptionNL']
        if item.get('strDescriptionHU',''):
            albumdata['descriptionHU'] = item['strDescriptionHU']
        if item.get('strDescriptionNO',''):
            albumdata['descriptionNO'] = item['strDescriptionNO']
        if item.get('strDescriptionIL',''):
            albumdata['descriptionIL'] = item['strDescriptionIL']
        if item.get('strDescriptionPL',''):
            albumdata['descriptionPL'] = item['strDescriptionPL']
        if item.get('strArtist',''):
            albumdata['artist_description'] = item['strArtist']
            artists = []
            artistdata = {}
            artistdata['artist'] = item['strArtist']
            if item.get('strMusicBrainzArtistID',''):
                artistdata['mbartistid'] = item['strMusicBrainzArtistID']
            artists.append(artistdata)
            albumdata['artist'] = artists
        thumbs = []
        extras = []
        if item.get('strAlbumThumb',''):
            thumbdata = {}
            thumbdata['image'] = item['strAlbumThumb']
            thumbdata['preview'] = item['strAlbumThumb'] + '/preview'
            thumbdata['aspect'] = 'thumb'
            thumbs.append(thumbdata)
        if item.get('strAlbumThumbBack',''):
            extradata = {}
            extradata['image'] = item['strAlbumThumbBack']
            extradata['preview'] = item['strAlbumThumbBack'] + '/preview'
            extradata['aspect'] = 'back'
            extras.append(extradata)
        if item.get('strAlbumSpine',''):
            extradata = {}
            extradata['image'] = item['strAlbumSpine']
            extradata['preview'] = item['strAlbumSpine'] + '/preview'
            extradata['aspect'] = 'spine'
            extras.append(extradata)
        if item.get('strAlbumCDart',''):
            extradata = {}
            extradata['image'] = item['strAlbumCDart']
            extradata['preview'] = item['strAlbumCDart'] + '/preview'
            extradata['aspect'] = 'discart'
            extras.append(extradata)
        if item.get('strAlbum3DCase',''):
            extradata = {}
            extradata['image'] = item['strAlbum3DCase']
            extradata['preview'] = item['strAlbum3DCase'] + '/preview'
            extradata['aspect'] = '3dcase'
            extras.append(extradata)
        if item.get('strAlbum3DFlat',''):
            extradata = {}
            extradata['image'] = item['strAlbum3DFlat']
            extradata['preview'] = item['strAlbum3DFlat'] + '/preview'
            extradata['aspect'] = '3dflat'
            extras.append(extradata)
        if item.get('strAlbum3DFace',''):
            extradata = {}
            extradata['image'] = item['strAlbum3DFace']
            extradata['preview'] = item['strAlbum3DFace'] + '/preview'
            extradata['aspect'] = '3dface'
            extras.append(extradata)
        if thumbs:
            albumdata['thumb'] = thumbs
        if extras:
            albumdata['extras'] = extras
        return albumdata
