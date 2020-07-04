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
from .theaudiodb import theaudiodb_artistdetails
from .theaudiodb import theaudiodb_artistalbums
from .musicbrainz import musicbrainz_artistfind
from .musicbrainz import musicbrainz_artistdetails
from .discogs import discogs_artistfind
from .discogs import discogs_artistdetails
from .discogs import discogs_artistalbums
from .allmusic import allmusic_artistdetails
from .allmusic import allmusic_artistalbums
from .nfo import nfo_geturl
from .fanarttv import fanarttv_artistart
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
    def __init__(self, action, key, artist, url, nfo, settings):
        # get start time in milliseconds
        self.start = int(round(time.time() * 1000))
        # parse path settings
        self.parse_settings(settings)
        # return a dummy result, this is just for backward compitability with xml based scrapers https://github.com/xbmc/xbmc/pull/11632
        if action == 'resolveid':
            result = self.resolve_mbid(key)
            if result:
                self.return_resolved(result)
        # search for artist name matches
        elif action == 'find':
            # both musicbrainz and discogs allow 1 api per second. this query requires 1 musicbrainz api call and optionally 1 discogs api call
            RATELIMIT = 1000
            # try musicbrainz first
            result = self.find_artist(artist, 'musicbrainz')
            if result:
                self.return_search(result)
            # fallback to discogs
            else:
                result = self.find_artist(artist, 'discogs')
                if result:
                    self.return_search(result)
        # return info using artistname / id's
        elif action == 'getdetails':
            details = {}
            url = json.loads(url)
            artist = url['artist'].encode('utf-8')
            mbid = url.get('mbid', '')
            dcid = url.get('dcid', '')
            threads = []
            extrathreads = []
            # we have a musicbrainz id
            if mbid:
                # musicbrainz allows 1 api per second.
                RATELIMIT = 1000
                scrapers = [[mbid, 'musicbrainz'], [mbid, 'theaudiodb'], [mbid, 'fanarttv']]

                for item in scrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
                # wait for musicbrainz to finish
                threads[0].join()
                # check if we have a result:
                if 'musicbrainz' in details:
                    extrascrapers = []
                    # only scrape allmusic if we have an url provided by musicbrainz
                    if 'allmusic-url' in details['musicbrainz']:
                        extrascrapers.append([details['musicbrainz']['allmusic-url'], 'allmusic'])
                    # only scrape discogs if we have an url provided by musicbrainz and discogs scraping is explicitly enabled (as it is slower)
                    if 'discogs-url' in details['musicbrainz'] and self.usediscogs == 1:
                        dcid = int(details['musicbrainz']['discogs-url'].rsplit('/', 1)[1])
                        extrascrapers.append([dcid, 'discogs'])
                        # discogs allows 1 api per second. this query requires 2 discogs api calls
                        RATELIMIT = 2000
                    for item in extrascrapers:
                        thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                        extrathreads.append(thread)
                        thread.start()
            # we have a discogs id
            else:
                result = self.get_details(dcid, 'discogs', details)
                # discogs allow 1 api per second. this query requires 2 discogs api call
                RATELIMIT = 2000
            if threads:
                for thread in threads:
                    thread.join()
            if extrathreads:
                for thread in extrathreads:
                    thread.join()
            result = self.compile_results(details)
            if result:
                self.return_details(result)
        elif action == 'NfoUrl':
            mbid = nfo_geturl(nfo)
            if mbid:
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
        self.bio = settings['bio']
        self.discog = settings['discog']
        self.genre = settings['genre']
        self.lang = settings['lang']
        self.mood = settings['mood']
        self.style = settings['style']
        self.usediscogs = settings['usediscogs']

    def resolve_mbid(self, mbid):
        # create dummy result
        item = {}
        item['artist'] = ''
        item['mbartistid'] = mbid
        return item

    def find_artist(self, artist, site):
        json = True
        # musicbrainz
        if site == 'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZSEARCH % urllib.parse.quote_plus(artist))
            scraper = musicbrainz_artistfind
        # musicbrainz
        if site == 'discogs':
            url = DISCOGSURL % (DISCOGSSEARCH % (urllib.parse.quote_plus(artist), DISCOGSKEY , DISCOGSSECRET))
            scraper = discogs_artistfind
        result = get_data(url, json)
        if not result:
            return
        artistresults = scraper(result, artist)
        return artistresults

    def get_details(self, param, site, details):
        json = True
        # theaudiodb
        if site == 'theaudiodb':
            url = AUDIODBURL % (AUDIODBKEY, AUDIODBDETAILS % param)
            artistscraper = theaudiodb_artistdetails
        # musicbrainz
        elif site == 'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZDETAILS % param)
            artistscraper = musicbrainz_artistdetails
        # fanarttv
        elif site == 'fanarttv':
            url = FANARTVURL % (param, FANARTVKEY)
            artistscraper = fanarttv_artistart
        # discogs
        elif site == 'discogs':
            url = DISCOGSURL % (DISCOGSDETAILS % (param, DISCOGSKEY, DISCOGSSECRET))
            artistscraper = discogs_artistdetails
        # allmusic
        elif site == 'allmusic':
            url = param + '/discography'
            artistscraper = allmusic_artistdetails
            json = False
        result = get_data(url, json)
        if not result:
            return
        artistresults = artistscraper(result)
        if not artistresults:
            return
        if site == 'theaudiodb' or site == 'discogs' or site == 'allmusic':
            if site == 'theaudiodb':
                # theaudiodb - discography
                albumsurl = AUDIODBURL % (AUDIODBKEY, AUDIODBDISCOGRAPHY % artistresults['mbartistid'])
                scraper = theaudiodb_artistalbums
            elif site == 'discogs':
                # discogs - discography
                albumsurl = DISCOGSURL % (DISCOGSDISCOGRAPHY % (param, DISCOGSKEY, DISCOGSSECRET))
                scraper = discogs_artistalbums
            elif site == 'allmusic':
                # allmusic - discography
                albumsurl = param + '/discography'
                scraper = allmusic_artistalbums
            albumdata = get_data(albumsurl, json)
            if albumdata:
                albumresults = scraper(albumdata)
                if albumresults:
                    artistresults['albums'] = albumresults
        details[site] = artistresults
        return details

    def compile_results(self, details):
        result = {}
        thumbs = []
        fanart = []
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
                elif k == 'fanart':
                    fanart.append(v)
                if k == 'extras':
                    extras.append(v)
        if 'musicbrainz' in details:
            for k, v in details['musicbrainz'].items():
                result[k] = v
        if 'fanarttv' in details:
            for k, v in details['fanarttv'].items():
                result[k] = v
                if k == 'thumb':
                    thumbs.append(v)
                elif k == 'fanart':
                    fanart.append(v)
                if k == 'extras':
                    extras.append(v)
        # provide artwork from all scrapers for getthumb / getfanart option
        if result:
            # artworks from most accurate sources first
            thumbs.reverse()
            thumbnails = []
            fanart.reverse()
            fanarts = []
            # the order for extra art does not matter
            extraart = []
            for thumblist in thumbs:
                for item in thumblist:
                    thumbnails.append(item)
            for extralist in extras:
                for item in extralist:
                    extraart.append(item)
            # add the extra art to the end of the thumb list
            thumbnails.extend(extraart)
            result['thumb'] = thumbnails
            for fanartlist in fanart:
                for item in fanartlist:
                    fanarts.append(item)
            result['fanart'] = fanarts
        data = self.user_prefs(details, result)
        return data

    def user_prefs(self, details, result):
        # user preferences
        lang = 'biography' + self.lang
        if self.bio == 'theaudiodb' and 'theaudiodb' in details:
            if lang in details['theaudiodb']:
                result['biography'] = details['theaudiodb'][lang]
            elif 'biographyEN' in details['theaudiodb']:
                result['biography'] = details['theaudiodb']['biographyEN']
        elif self.bio == 'discogs' and 'discogs' in details:
            result['biography'] = details['discogs']['biography']
        if (self.discog in details) and ('albums' in details[self.discog]):
            result['albums'] = details[self.discog]['albums']
        if (self.genre in details) and ('genre' in details[self.genre]):
            result['genre'] = details[self.genre]['genre']
        if (self.style in details) and ('styles' in details[self.style]):
            result['styles'] = details[self.style]['styles']
        if (self.mood in details) and ('moods' in details[self.mood]):
            result['moods'] = details[self.mood]['moods']
        return result

    def return_search(self, data):
        for item in data:
            listitem = xbmcgui.ListItem(item['artist'], offscreen=True)
            listitem.setArt({'thumb': item['thumb']})
            listitem.setProperty('artist.genre', item['genre'])
            listitem.setProperty('artist.born', item['born'])
            listitem.setProperty('relevance', item['relevance'])
            if 'type' in item:
                listitem.setProperty('artist.type', item['type'])
            if 'gender' in item:
                listitem.setProperty('artist.gender', item['gender'])
            if 'disambiguation' in item:
                listitem.setProperty('artist.disambiguation', item['disambiguation'])
            url = {'artist':item['artist']}
            if 'mbid' in item:
                url['mbid'] = item['mbid']
            if 'dcid' in item:
                url['dcid'] = item['dcid']
            xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=json.dumps(url), listitem=listitem, isFolder=True)

    def return_nfourl(self, item):
        url = {'artist':item['artist'], 'mbid':item['mbartistid']}
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=json.dumps(url), listitem=listitem, isFolder=True)

    def return_resolved(self, item):
        url = {'artist':item['artist'], 'mbid':item['mbartistid']}
        listitem = xbmcgui.ListItem(path=json.dumps(url), offscreen=True)
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)

    def return_details(self, item):
        if not 'artist' in item:
            return
        listitem = xbmcgui.ListItem(item['artist'], offscreen=True)
        if 'mbartistid' in item:
            listitem.setProperty('artist.musicbrainzid', item['mbartistid'])
        if 'genre' in item:
            listitem.setProperty('artist.genre', item['genre'])
        if 'biography' in item:
            listitem.setProperty('artist.biography', item['biography'])
        if 'gender' in item:
            listitem.setProperty('artist.gender', item['gender'])
        if 'styles' in item:
            listitem.setProperty('artist.styles', item['styles'])
        if 'moods' in item:
            listitem.setProperty('artist.moods', item['moods'])
        if 'instruments' in item:
            listitem.setProperty('artist.instruments', item['instruments'])
        if 'disambiguation' in item:
            listitem.setProperty('artist.disambiguation', item['disambiguation'])
        if 'type' in item:
            listitem.setProperty('artist.type', item['type'])
        if 'sortname' in item:
            listitem.setProperty('artist.sortname', item['sortname'])
        if 'active' in item:
            listitem.setProperty('artist.years_active', item['active'])
        if 'born' in item:
            listitem.setProperty('artist.born', item['born'])
        if 'formed' in item:
            listitem.setProperty('artist.formed', item['formed'])
        if 'died' in item:
            listitem.setProperty('artist.died', item['died'])
        if 'disbanded' in item:
            listitem.setProperty('artist.disbanded', item['disbanded'])
        art = {}
        if 'clearlogo' in item:
            art['clearlogo'] = item['clearlogo']
        if 'banner' in item:
            art['banner'] = item['banner']
        if 'clearart' in item:
            art['clearart'] = item['clearart']
        if 'landscape' in item:
            art['landscape'] = item['landscape']
        listitem.setArt(art)
        if 'fanart' in item:
            listitem.setProperty('artist.fanarts', str(len(item['fanart'])))
            for count, fanart in enumerate(item['fanart']):
                listitem.setProperty('artist.fanart%i.url' % (count + 1), fanart['image'])
                listitem.setProperty('artist.fanart%i.preview' % (count + 1), fanart['preview'])
        if 'thumb' in item:
            listitem.setProperty('artist.thumbs', str(len(item['thumb'])))
            for count, thumb in enumerate(item['thumb']):
                listitem.setProperty('artist.thumb%i.url' % (count + 1), thumb['image'])
                listitem.setProperty('artist.thumb%i.preview' % (count + 1), thumb['preview'])
                listitem.setProperty('artist.thumb%i.aspect' % (count + 1), thumb['aspect'])
        if 'albums' in item:
            listitem.setProperty('artist.albums', str(len(item['albums'])))
            for count, album in enumerate(item['albums']):
                listitem.setProperty('artist.album%i.title' % (count + 1), album['title'])
                listitem.setProperty('artist.album%i.year' % (count + 1), album['year'])
                if 'musicbrainzreleasegroupid' in album:
                    listitem.setProperty('artist.album%i.musicbrainzreleasegroupid' % (count + 1), album['musicbrainzreleasegroupid'])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)
