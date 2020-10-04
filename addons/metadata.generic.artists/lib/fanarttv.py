# -*- coding: utf-8 -*-

def fanarttv_artistart(data):
    artistdata = {}
    extras = []
    if 'artistbackground' in data:
        fanart = []
        for item in data['artistbackground']:
            fanartdata = {}
            fanartdata['image'] = item['url']
            fanartdata['preview'] = item['url'].replace('/fanart/', '/preview/')
            fanartdata['aspect'] = 'fanart'
            fanart.append(fanartdata)
        artistdata['fanart'] = fanart
    if 'artistthumb' in data:
        thumbs = []
        for item in data['artistthumb']:
            thumbdata = {}
            thumbdata['image'] = item['url']
            thumbdata['preview'] = item['url'].replace('/fanart/', '/preview/')
            thumbdata['aspect'] = 'thumb'
            thumbs.append(thumbdata)
        if thumbs:
            artistdata['thumb'] = thumbs
    if 'musicbanner' in data:
        for item in data['musicbanner']:
            extradata = {}
            extradata['image'] = item['url']
            extradata['preview'] = item['url'].replace('/fanart/', '/preview/')
            extradata['aspect'] = 'banner'
            extras.append(extradata)
    if 'hdmusiclogo' in data:
        for item in data['hdmusiclogo']:
            extradata = {}
            extradata['image'] = item['url']
            extradata['preview'] = item['url'].replace('/fanart/', '/preview/')
            extradata['aspect'] = 'clearlogo'
            extras.append(extradata)
    elif 'musiclogo' in data:
        for item in data['musiclogo']:
            extradata = {}
            extradata['image'] = item['url']
            extradata['preview'] = item['url'].replace('/fanart/', '/preview/')
            extradata['aspect'] = 'clearlogo'
            extras.append(extradata)
    if extras:
        artistdata['extras'] = extras
    return artistdata
