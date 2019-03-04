# -*- coding: utf-8 -*-

import json
import socket
import sys
import time
import urllib.parse
import urllib.request
import _strptime # https://bugs.python.org/issue7980
from threading import Thread
from urllib.error import HTTPError, URLError
from socket import timeout
import xbmc
import xbmcgui
import xbmcplugin
import xbmcaddon
from .theaudiodb import theaudiodb_albumdetails
from .musicbrainz import musicbrainz_albumfind
from .musicbrainz import musicbrainz_albumdetails
from .musicbrainz import musicbrainz_albumart
from .discogs import discogs_albumfind
from .discogs import discogs_albumdetails
from .allmusic import allmusic_albumfind
from .allmusic import allmusic_albumdetails
from .nfo import nfo_geturl
from .fanarttv import fanarttv_albumart
from .utils import *

ADDONID = xbmcaddon.Addon().getAddonInfo('id')
ADDONNAME = xbmcaddon.Addon().getAddonInfo('name')
ADDONVERSION = xbmcaddon.Addon().getAddonInfo('version')


def log(txt):
    message = '%s: %s' % (ADDONID, txt)
    xbmc.log(msg=message, level=xbmc.LOGDEBUG)

def get_data(url, jsonformat):
    try:
        headers = {}
        headers['User-Agent'] = '%s/%s ( http://kodi.tv )' % (ADDONNAME, ADDONVERSION)
        req = urllib.request.Request(url, headers=headers)
        resp = urllib.request.urlopen(req, timeout=5)
        respdata = resp.read()
    except URLError as e:
        log('URLError: %s - %s' % (e.reason, url))
        return
    except HTTPError as e:
        log('HTTPError: %s - %s' % (e.reason, url))
        return
    except socket.timeout as e:
        log('socket: %s - %s' % (e, url))
        return
    if resp.getcode() == 503:
        log('exceeding musicbrainz api limit')
        return
    elif resp.getcode() == 429:
        log('exceeding discogs api limit')
        return
    if jsonformat:
        respdata = json.loads(respdata)
    return respdata


class Scraper():
    def __init__(self, action, key, artist, album, url, nfo, settings):
        # get start time in milliseconds
        self.start = int(round(time.time() * 1000))
        # parse path settings
        self.parse_settings(settings)
        # return a dummy result, this is just for backward compitability with xml based scrapers https://github.com/xbmc/xbmc/pull/11632
        if action == 'resolveid':
            result = self.resolve_mbid(key)
            if result:
                self.return_resolved(result)
        # search for artist name / album title matches
        elif action == 'find':
            # both musicbrainz and discogs allow 1 api per second. this query requires 1 musicbrainz api call and optionally 1 discogs api call
            RATELIMIT = 1000
            # try musicbrainz first
            result = self.find_album(artist, album, 'musicbrainz')
            if result:
                self.return_search(result)
            # fallback to discogs
            else:
                result = self.find_album(artist, album, 'discogs')
                if result:
                    self.return_search(result)
        # return info using artistname / albumtitle / id's
        elif action == 'getdetails':
            details = {}
            url = json.loads(url)
            artist = url['artist'].encode('utf-8')
            album = url['album'].encode('utf-8')
            mbid = url.get('mbalbumid', '')
            dcid = url.get('dcalbumid', '')
            mbreleasegroupid = url.get('mbreleasegroupid', '')
            threads = []
            # we have a musicbrainz album id, but no musicbrainz releasegroupid
            if mbid and not mbreleasegroupid:
                # musicbrainz allows 1 api per second.
                RATELIMIT = 1000
                for item in [[mbid, 'musicbrainz']]:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
                # wait for musicbrainz to finish
                threads[0].join()
                # check if we have a result:
                if 'musicbrainz' in details:
                    artist = details['musicbrainz']['artist_description'].encode('utf-8')
                    album = details['musicbrainz']['album'].encode('utf-8')
                    mbreleasegroupid = details['musicbrainz']['mbreleasegroupid']
                    scrapers = [[mbreleasegroupid, 'theaudiodb'], [mbreleasegroupid, 'fanarttv'], [mbreleasegroupid, 'coverarchive'], [[artist, album], 'allmusic']]
                    if self.usediscogs == 1:
                        scrapers.append([[artist, album, dcid], 'discogs'])
                        # discogs allows 1 api per second. this query requires 2 discogs api calls
                        RATELIMIT = 2000
                    for item in scrapers:
                        thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                        threads.append(thread)
                        thread.start()
            # we have a discogs id and artistname and albumtitle
            elif dcid:
                # discogs allows 1 api per second. this query requires 1 discogs api call
                RATELIMIT = 1000
                for item in [[[artist, album, dcid], 'discogs'], [[artist, album], 'allmusic']]:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
            # we have musicbrainz album id, musicbrainz releasegroupid, artistname and albumtitle
            else:
                # musicbrainz allows 1 api per second.
                RATELIMIT = 1000
                scrapers = [[mbid, 'musicbrainz'], [mbreleasegroupid, 'theaudiodb'], [mbreleasegroupid, 'fanarttv'], [mbreleasegroupid, 'coverarchive'], [[artist, album], 'allmusic']]
                if self.usediscogs == 1:
                    scrapers.append([[artist, album, dcid], 'discogs'])
                    # discogs allows 1 api per second. this query requires 2 discogs api calls
                    RATELIMIT = 2000
                for item in scrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
            for thread in threads:
                thread.join()
            result = self.compile_results(details)
            if result:
                self.return_details(result)
        # extract the mbid from the provided musicbrainz url
        elif action == 'NfoUrl':
            mbid = nfo_geturl(nfo)
            if mbid:
                # create a dummy item
                result = self.resolve_mbid(mbid)
                if result:
                    self.return_nfourl(result)
        # get end time in milliseconds
        self.end = int(round(time.time() * 1000))
        # handle musicbrainz and discogs ratelimit
        if action == 'find' or action == 'getdetails':
            if self.end - self.start < RATELIMIT:
                # wait max 2 seconds
                diff = RATELIMIT - (self.end - self.start)
                xbmc.sleep(diff)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))

    def parse_settings(self, data):
        settings = json.loads(data)
        # note: path settings are taken from the db, they may not reflect the current settings.xml file
        self.genre = settings['genre']
        self.lang = settings['lang']
        self.mood = settings['mood']
        self.rating = settings['rating']
        self.style = settings['style']
        self.theme = settings['theme']
        self.usediscogs = settings['usediscogs']

    def resolve_mbid(self, mbid):
        # create dummy result
        item = {}
        item['artist_description'] = ''
        item['album'] = ''
        item['mbalbumid'] = mbid
        item['mbreleasegroupid'] = ''
        return item

    def find_album(self, artist, album, site):
        json = True
        # musicbrainz
        if site == 'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZSEARCH % (urllib.parse.quote_plus(album), urllib.parse.quote_plus(artist), urllib.parse.quote_plus(artist)))
            scraper = musicbrainz_albumfind
        # discogs
        elif site == 'discogs':
            url = DISCOGSURL % (DISCOGSSEARCH % (urllib.parse.quote_plus(album), urllib.parse.quote_plus(artist), DISCOGSKEY , DISCOGSSECRET))
            scraper = discogs_albumfind
        # allmusic
        elif site == 'allmusic':
            url = ALLMUSICURL % (ALLMUSICSEARCH % (urllib.parse.quote_plus(artist), urllib.parse.quote_plus(album)))
            scraper = allmusic_albumfind
            json = False
        result = get_data(url, json)
        if not result:
            return
        albumresults = scraper(result, artist, album)
        return albumresults

    def get_details(self, param, site, details):
        json = True
        # theaudiodb
        if site == 'theaudiodb':
            url = AUDIODBURL % (AUDIODBKEY, AUDIODBDETAILS % param)
            albumscraper = theaudiodb_albumdetails
        # musicbrainz
        elif site == 'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZDETAILS % param)
            albumscraper = musicbrainz_albumdetails
        # fanarttv
        elif site == 'fanarttv':
            url = FANARTVURL % (param, FANARTVKEY)
            albumscraper = fanarttv_albumart
       # coverarchive
        elif site == 'coverarchive':
            url = MUSICBRAINZART % (param)
            albumscraper = musicbrainz_albumart
        # discogs
        elif site == 'discogs':
            dcalbumid = param[2]
            if not dcalbumid:
                # search
                found = self.find_album(param[0], param[1], 'discogs')
                if found:
                    # get details
                    dcalbumid = found[0]['dcalbumid']
                else:
                    return
            url = DISCOGSURL % (DISCOGSDETAILS % (dcalbumid, DISCOGSKEY, DISCOGSSECRET))
            albumscraper = discogs_albumdetails
        # allmusic
        elif site == 'allmusic':
            # search
            found = self.find_album(param[0], param[1], 'allmusic')
            if found:
                # get details
                url = ALLMUSICDETAILS % found[0]['url']
                albumscraper = allmusic_albumdetails
                json = False
            else:
                return
        result = get_data(url, json)
        if not result:
            return
        albumresults = albumscraper(result)
        if not albumresults:
            return
        details[site] = albumresults
        return details

    def compile_results(self, details):
        result = {}
        thumbs = []
        extras = []
        # merge metadata results, start with the least accurate sources
        if 'discogs' in details:
            for k, v in details['discogs'].items():
                result[k] = v
                if k == 'thumb':
                    thumbs.append(v)
        if 'allmusic' in details:
            for k, v in details['allmusic'].items():
                result[k] = v
                if k == 'thumb':
                    thumbs.append(v)
        if 'theaudiodb' in details:
            for k, v in details['theaudiodb'].items():
                result[k] = v
                if k == 'thumb':
                    thumbs.append(v)
                if k == 'extras':
                    extras.append(v)
        if 'musicbrainz' in details:
            for k, v in details['musicbrainz'].items():
                result[k] = v
        if 'coverarchive' in details:
            for k, v in details['coverarchive'].items():
                result[k] = v
                if k == 'thumb':
                    thumbs.append(v)
                if k == 'extras':
                    extras.append(v)
        # prefer artwork from fanarttv
        if 'fanarttv' in details:
            for k, v in details['fanarttv'].items():
                result[k] = v
                if k == 'thumb':
                    thumbs.append(v)
                if k == 'extras':
                    extras.append(v)
        # use musicbrainz artist list as they provide mbid's, these can be passed to the artist scraper
        if 'musicbrainz' in details:
            result['artist'] = details['musicbrainz']['artist']
        # provide artwork from all scrapers for getthumb option
        if result:
            # thumb list from most accurate sources first
            thumbs.reverse()
            thumbnails = []
            for thumblist in thumbs:
                for item in thumblist:
                    thumbnails.append(item)
            # the order for extra art does not matter
            extraart = []
            for extralist in extras:
                for item in extralist:
                    extraart.append(item)
            # add the extra art to the end of the thumb list
            thumbnails.extend(extraart)
            result['thumb'] = thumbnails
        data = self.user_prefs(details, result)
        return data

    def user_prefs(self, details, result):
        # user preferences
        lang = 'description' + self.lang
        if 'theaudiodb' in details:
            if lang in details['theaudiodb']:
                result['description'] = details['theaudiodb'][lang]
            elif 'descriptionEN' in details['theaudiodb']:
                result['description'] = details['theaudiodb']['descriptionEN']
        if (self.genre in details) and ('genre' in details[self.genre]):
            result['genre'] = details[self.genre]['genre']
        if (self.style in details) and ('styles' in details[self.style]):
            result['styles'] = details[self.style]['styles']
        if (self.mood in details) and ('moods' in details[self.mood]):
            result['moods'] = details[self.mood]['moods']
        if (self.theme in details) and ('themes' in details[self.theme]):
            result['themes'] = details[self.theme]['themes']
        if (self.rating in details) and ('rating' in details[self.rating]):
            result['rating'] = details[self.rating]['rating']
            result['votes'] = details[self.rating]['votes']
        return result

    def return_search(self, data):
        for count, item in enumerate(data):
            listitem = xbmcgui.ListItem(item['album'], offscreen=True)
            listitem.setArt({'thumb': item['thumb']})
            listitem.setProperty('album.artist', item['artist_description'])
            listitem.setProperty('album.year', item.get('year',''))
            listitem.setProperty('album.type', item.get('type',''))
            listitem.setProperty('album.releasestatus', item.get('releasestatus',''))
            listitem.setProperty('album.label', item.get('label',''))
            listitem.setProperty('relevance', item['relevance'])
            url = {'artist':item['artist_description'], 'album':item['album']}
            if 'mbalbumid' in item:
                url['mbalbumid'] = item['mbalbumid']
            if 'mbreleasegroupid' in item:
                url['mbreleasegroupid'] = item['mbreleasegroupid']
            if 'dcalbumid' in item:
                url['dcalbumid'] = item['dcalbumid']
            xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=json.dumps(url), listitem=listitem, isFolder=True)

    def return_nfourl(self, item):
        url = {'artist':item['artist_description'], 'album':item['album'], 'mbalbumid':item['mbalbumid'], 'mbreleasegroupid':item['mbreleasegroupid']}
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=json.dumps(url), listitem=listitem, isFolder=True)

    def return_resolved(self, item):
        url = {'artist':item['artist_description'], 'album':item['album'], 'mbalbumid':item['mbalbumid'], 'mbreleasegroupid':item['mbreleasegroupid']}
        listitem = xbmcgui.ListItem(path=json.dumps(url), offscreen=True)
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)

    def return_details(self, item):
        if not 'album' in item:
            return
        listitem = xbmcgui.ListItem(item['album'], offscreen=True)
        if 'mbalbumid' in item:
            listitem.setProperty('album.musicbrainzid', item['mbalbumid'])
            listitem.setProperty('album.releaseid', item['mbalbumid'])
        if 'mbreleasegroupid' in item:
            listitem.setProperty('album.releasegroupid', item['mbreleasegroupid'])
        if 'scrapedmbid' in item:
            listitem.setProperty('album.scrapedmbid', item['scrapedmbid'])
        if 'artist' in item:
            listitem.setProperty('album.artists', str(len(item['artist'])))
            for count, artist in enumerate(item['artist']):
                listitem.setProperty('album.artist%i.name' % (count + 1), artist['artist'])
                listitem.setProperty('album.artist%i.musicbrainzid' % (count + 1), artist.get('mbartistid', ''))
                listitem.setProperty('album.artist%i.sortname' % (count + 1), artist.get('artistsort', ''))
        if 'genre' in item:
            listitem.setProperty('album.genre', item['genre'])
        if 'styles' in item:
            listitem.setProperty('album.styles', item['styles'])
        if 'moods' in item:
            listitem.setProperty('album.moods', item['moods'])
        if 'themes' in item:
            listitem.setProperty('album.themes', item['themes'])
        if 'description' in item:
            listitem.setProperty('album.review', item['description'])
        if 'releasedate' in item:
            listitem.setProperty('album.releasedate', item['releasedate'])
        if 'originaldate' in item:
            listitem.setProperty('album.originaldate', item['originaldate'])
        if 'releasestatus' in item:
            listitem.setProperty('album.releasestatus', item['releasestatus'])
        if 'artist_description' in item:
            listitem.setProperty('album.artist_description', item['artist_description'])
        if 'label' in item:
            listitem.setProperty('album.label', item['label'])
        if 'type' in item:
            listitem.setProperty('album.type', item['type'])
        if 'compilation' in item:
            listitem.setProperty('album.compilation', item['compilation'])
        if 'year' in item:
            listitem.setProperty('album.year', item['year'])
        if 'rating' in item:
            listitem.setProperty('album.rating', item['rating'])
        if 'votes' in item:
            listitem.setProperty('album.votes', item['votes'])
        art = {}
        if 'discart' in item:
            art['discart'] = item['discart']
        if 'multidiscart' in item:
            for k, v in item['multidiscart'].items():
                discart = 'discart%s' % k
                art[discart] = v[0]['image']
        if 'back' in item:
            art['back'] = item['back']
        if 'spine' in item:
            art['spine'] = item['spine']
        if '3dcase' in item:
            art['3dcase'] = item['3dcase']
        if '3dflat' in item:
            art['3dflat'] = item['3dflat']
        if '3dface' in item:
            art['3dface'] = item['3dface']
        listitem.setArt(art)
        if 'thumb' in item:
            listitem.setProperty('album.thumbs', str(len(item['thumb'])))
            for count, thumb in enumerate(item['thumb']):
                listitem.setProperty('album.thumb%i.url' % (count + 1), thumb['image'])
                listitem.setProperty('album.thumb%i.aspect' % (count + 1), thumb['aspect'])
                listitem.setProperty('album.thumb%i.preview' % (count + 1), thumb['preview'])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)
