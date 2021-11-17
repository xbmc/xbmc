# -*- coding: utf-8 -*-

def musicbrainz_albumfind(data, artist, album):
    albums = []
    # count how often each releasegroup occurs in the release results
    # keep track of the release with the highest score and earliest releasedate in each releasegroup
    releasegroups = {}
    for item in data.get('releases'):
        mbid = item['id']
        score = item.get('score', 0)
        releasegroup = item['release-group']['id']
        if 'date' in item and item['date']:
            date = item['date'].replace('-','')
            if len(date) == 4:
                date = date + '9999'
        else:
            date = '99999999'
        if releasegroup in releasegroups:
            count = releasegroups[releasegroup][0] + 1
            topmbid = releasegroups[releasegroup][1]
            topdate = releasegroups[releasegroup][2]
            topscore = releasegroups[releasegroup][3]
            if date < topdate and score >= topscore:
                topdate = date
                topmbid = mbid
            releasegroups[releasegroup] = [count, topmbid, topdate, topscore]
        else:
            releasegroups[releasegroup] = [1, mbid, date, score]
    if releasegroups:
        # get the highest releasegroup count
        maxcount = max(releasegroups.values())[0]
        # get the releasegroup(s) that match this highest value
        topgroups = [k for k, v in releasegroups.items() if v[0] == maxcount]
    for item in data.get('releases'):
        # only use the 'top' release from each releasegroup
        if item['id'] != releasegroups[item['release-group']['id']][1]:
            continue
        albumdata = {}
        if item.get('artist-credit'):
            artists = []
            artistdisp = ""
            for artist in item['artist-credit']:
                artistdata = {}
                artistdata['artist'] = artist['artist']['name']
                artistdata['mbartistid'] = artist['artist']['id']
                artistdata['artistsort'] = artist['artist']['sort-name']
                artistdisp = artistdisp + artist['artist']['name']
                artistdisp = artistdisp + artist.get('joinphrase', '')
                artists.append(artistdata)
            albumdata['artist'] = artists
            albumdata['artist_description'] = artistdisp
        if item.get('label-info','') and item['label-info'][0].get('label','') and item['label-info'][0]['label'].get('name',''):
            albumdata['label'] = item['label-info'][0]['label']['name']
        albumdata['album'] = item['title']
        if item.get('date',''):
            albumdata['year'] = item['date'][:4]
        albumdata['thumb'] = 'https://coverartarchive.org/release-group/%s/front-250' % item['release-group']['id']
        if item.get('label-info','') and item['label-info'][0].get('label','') and item['label-info'][0]['label'].get('name',''):
            albumdata['label'] = item['label-info'][0]['label']['name']
        if item.get('status',''):
            albumdata['releasestatus'] = item['status']
        albumdata['type'] = item['release-group'].get('primary-type')
        albumdata['mbalbumid'] = item['id']
        albumdata['mbreleasegroupid'] = item['release-group']['id']
        if item.get('score'):
            releasescore = item['score'] / 100.0
            # if the release is in the releasegroup with most releases, it is considered the most accurate one
            # (this also helps with preferring official releases over bootlegs, assuming there are more variations of an official release than of a bootleg)
            if item['release-group']['id'] not in topgroups:
                releasescore -= 0.001
            # if the release is an album, prefer it over singles/ep's
            # (this needs to be the double of the above, as me might have just given the album a lesser score if the single happened to be in the topgroup)
            if item['release-group'].get('primary-type') != 'Album':
                releasescore -= 0.002
            albumdata['relevance'] = str(releasescore)
        albums.append(albumdata)
    return albums

def musicbrainz_albumlinks(data):
    albumlinks = {}
    if 'relations' in data and data['relations']:
        for item in data['relations']:
            if item['type'] == 'allmusic':
                albumlinks['allmusic'] = item['url']['resource']
            elif item['type'] == 'discogs':
                albumlinks['discogs'] = item['url']['resource'].rsplit('/', 1)[1]
            elif item['type'] == 'wikipedia':
                albumlinks['wikipedia'] = item['url']['resource'].rsplit('/', 1)[1]
            elif item['type'] == 'wikidata':
                albumlinks['wikidata'] = item['url']['resource'].rsplit('/', 1)[1]
    return albumlinks

def musicbrainz_albumdetails(data):
    albumdata = {}
    albumdata['album'] = data['title']
    albumdata['mbalbumid'] = data['id']
    if data.get('release-group',''):
        albumdata['mbreleasegroupid'] = data['release-group']['id']
        if data['release-group']['rating'] and data['release-group']['rating']['value']:
            albumdata['rating'] = str(int((float(data['release-group']['rating']['value']) * 2) + 0.5))
            albumdata['votes'] = str(data['release-group']['rating']['votes-count'])
        if data['release-group'].get('primary-type'):
            albumtypes = [data['release-group']['primary-type']] + data['release-group']['secondary-types']
            albumdata['type'] = ' / '.join(albumtypes)
            if 'Compilation' in albumtypes:
                albumdata['compilation'] = 'true'
        if data['release-group'].get('first-release-date',''):
            albumdata['originaldate'] = data['release-group']['first-release-date']
    if data.get('release-events',''):
        albumdata['year'] = data['release-events'][0]['date'][:4]
        albumdata['releasedate'] = data['release-events'][0]['date']
    if data.get('label-info','') and data['label-info'][0].get('label','') and data['label-info'][0]['label'].get('name',''):
        albumdata['label'] = data['label-info'][0]['label']['name']
    if data.get('status',''):
        albumdata['releasestatus'] = data['status']
    if data.get('artist-credit'):
        artists = []
        artistdisp = ''
        for artist in data['artist-credit']:
            artistdata = {}
            artistdata['artist'] = artist['name']
            artistdata['mbartistid'] = artist['artist']['id']
            artistdata['artistsort'] = artist['artist']['sort-name']
            artistdisp = artistdisp + artist['name']
            artistdisp = artistdisp + artist.get('joinphrase', '')
            artists.append(artistdata)
        albumdata['artist'] = artists
        albumdata['artist_description'] = artistdisp
    return albumdata

def musicbrainz_albumart(data):
    albumdata = {}
    thumbs = []
    extras = []
    for item in data['images']:
        if 'Front' in item['types']:
            thumbdata = {}
            thumbdata['image'] = item['image']
            thumbdata['preview'] = item['thumbnails']['small']
            thumbdata['aspect'] = 'thumb'
            thumbs.append(thumbdata)
        if 'Back' in item['types']:
            backdata = {}
            backdata['image'] = item['image']
            backdata['preview'] = item['thumbnails']['small']
            backdata['aspect'] = 'back'
            extras.append(backdata)
        if 'Medium' in item['types']:
            discartdata = {}
            discartdata['image'] = item['image']
            discartdata['preview'] = item['thumbnails']['small']
            discartdata['aspect'] = 'discart'
            extras.append(discartdata)
        # exclude spine+back images
        if 'Spine' in item['types'] and len(item['types']) == 1:
            spinedata = {}
            spinedata['image'] = item['image']
            spinedata['preview'] = item['thumbnails']['small']
            spinedata['aspect'] = 'spine'
            extras.append(spinedata)
    if thumbs:
        albumdata['thumb'] = thumbs
    if extras:
        albumdata['extras'] = extras
    return albumdata
