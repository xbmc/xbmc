# -*- coding: utf-8 -*-

import datetime
import difflib
import time
import re

def allmusic_albumfind(data, artist, album):
    data = data.decode('utf-8')
    albums = []
    albumlist = re.findall('class="album">\s*(.*?)\s*</li', data, re.S)
    for item in albumlist:
        albumdata = {}
        albumartist = re.search('class="artist">.*?>(.*?)</a', item, re.S)
        if albumartist:
            albumdata['artist'] = albumartist.group(1)
        else: # classical album
            continue
        albumname = re.search('class="title">.*?>(.*?)</a', item, re.S)
        if albumname:
            albumdata['album'] = albumname.group(1)
        else: # not likely to happen, but just in case
            continue
        # filter inaccurate results
        artistmatch = difflib.SequenceMatcher(None, artist.lower(), albumdata['artist'].lower()).ratio()
        albummatch = difflib.SequenceMatcher(None, album.lower(), albumdata['album'].lower()).ratio()
        if artistmatch > 0.90 and albummatch > 0.90:
            albumurl = re.search('class="title">\s*<a href="(.*?)"', item)
            if albumurl:
                albumdata['url'] = albumurl.group(1)
            else: # not likely to happen, but just in case
                continue
            albums.append(albumdata)
            # we are only interested in the top result
            break
    return albums

def allmusic_albumdetails(data):
    data = data.decode('utf-8')
    albumdata = {}
    releasedata = re.search('class="release-date">.*?<span>(.*?)<', data, re.S)
    if releasedata:
        dateformat = releasedata.group(1)
        if len(dateformat) > 4:
            try:
                # month day, year
                albumdata['releasedate'] = datetime.datetime(*(time.strptime(dateformat, '%B %d, %Y')[0:3])).strftime('%Y-%m-%d')
            except:
                # month, year
                albumdata['releasedate'] = datetime.datetime(*(time.strptime(dateformat, '%B, %Y')[0:3])).strftime('%Y-%m')
        else:
            # year
            albumdata['releasedate'] = dateformat
    yeardata = re.search('class="year".*?>\s*(.*?)\s*<', data)
    if yeardata:
        albumdata['year'] = yeardata.group(1)
    genredata = re.search('class="genre">.*?">(.*?)<', data, re.S)
    if genredata:
        albumdata['genre'] = genredata.group(1)
    styledata = re.search('class="styles">.*?div>\s*(.*?)\s*</div', data, re.S)
    if styledata:
        stylelist = re.findall('">(.*?)<', styledata.group(1))
        if stylelist:
            albumdata['styles'] =  ' / '.join(stylelist)
    mooddata = re.search('class="moods">.*?div>\s*(.*?)\s*</div', data, re.S)
    if mooddata:
        moodlist = re.findall('">(.*?)<', mooddata.group(1))
        if moodlist:
            albumdata['moods'] =  ' / '.join(moodlist)
    themedata = re.search('class="themes">.*?div>\s*(.*?)\s*</div', data, re.S)
    if themedata:
        themelist = re.findall('">(.*?)<', themedata.group(1))
        if themelist:
            albumdata['themes'] =  ' / '.join(themelist)
    ratingdata = re.search('itemprop="ratingValue">\s*(.*?)\s*</div', data)
    if ratingdata:
        albumdata['rating'] = ratingdata.group(1)
    albumdata['votes'] = ''
    titledata = re.search('class="album-title".*?>\s*(.*?)\s*<', data, re.S)
    if titledata:
        albumdata['album'] = titledata.group(1)
    labeldata = re.search('class="label-catalog".*?<.*?>(.*?)<', data, re.S)
    if labeldata:
        albumdata['label'] = labeldata.group(1)
    artistdata = re.search('class="album-artist".*?<span.*?>\s*(.*?)\s*</span', data, re.S)
    if artistdata:
        artistlist = re.findall('">(.*?)<', artistdata.group(1))
        artists = []
        for item in artistlist:
            artistinfo = {}
            artistinfo['artist'] = item
            artists.append(artistinfo)
        if artists:
            albumdata['artist'] = artists
            albumdata['artist_description'] = ' / '.join(artistlist)
    thumbsdata = re.search('class="album-contain".*?src="(.*?)"', data, re.S)
    if thumbsdata:
        thumbs = []
        thumbdata = {}
        thumb = thumbsdata.group(1).rstrip('?partner=allrovi.com')
        # ignore internal blank thumb
        if thumb.startswith('http'):
            # 0=largest / 1=75 / 2=150 / 3=250 / 4=400 / 5=500 / 6=1080
            if thumb.endswith('f=5'):
                thumbdata['image'] = thumb.replace('f=5', 'f=0')
                thumbdata['preview'] = thumb.replace('f=5', 'f=2')
            else:
                thumbdata['image'] = thumb
                thumbdata['preview'] = thumb
            thumbdata['aspect'] = 'thumb'
            thumbs.append(thumbdata)
            albumdata['thumb'] = thumbs
    return albumdata
