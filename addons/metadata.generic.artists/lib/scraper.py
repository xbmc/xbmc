# -*- coding: utf-8 -*-

import json
import socket
import sys
import time
import urllib.parse
import urllib.request
import _strptime # https://bugs.python.org/issue7980
from socket import timeout
from threading import Thread
from urllib.error import HTTPError, URLError
import xbmc
import xbmcaddon
import xbmcgui
import xbmcplugin
from .allmusic import allmusic_artistfind
from .allmusic import allmusic_artistdetails
from .allmusic import allmusic_artistalbums
from .discogs import discogs_artistfind
from .discogs import discogs_artistdetails
from .discogs import discogs_artistalbums
from .fanarttv import fanarttv_artistart
from .musicbrainz import musicbrainz_artistfind
from .musicbrainz import musicbrainz_artistdetails
from .nfo import nfo_geturl
from .theaudiodb import theaudiodb_artistdetails
from .theaudiodb import theaudiodb_artistalbums
from .theaudiodb import theaudiodb_mvids
from .wikipedia import wikipedia_artistdetails
from .utils import *

ADDONID = xbmcaddon.Addon().getAddonInfo('id')
ADDONNAME = xbmcaddon.Addon().getAddonInfo('name')
ADDONVERSION = xbmcaddon.Addon().getAddonInfo('version')


def log(txt):
    message = '%s: %s' % (ADDONID, txt)
    xbmc.log(msg=message, level=xbmc.LOGDEBUG)

def get_data(url, jsonformat, retry=True):
    try:
        if url.startswith('https://musicbrainz.org/'):
            api_timeout('musicbrainztime')
        elif url.startswith('https://api.discogs.com/'):
            api_timeout('discogstime')
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
        if retry:
            xbmc.sleep(1000)
            get_data(url, jsonformat, retry=False)
        else:
            return
    elif resp.getcode() == 429:
        log('exceeding discogs api limit')
        if retry:
            xbmc.sleep(1000)
            get_data(url, jsonformat, retry=False)
        else:
            return
    if jsonformat:
        respdata = json.loads(respdata)
    return respdata

def api_timeout(scraper):
    currenttime = round(time.time() * 1000)
    previoustime = xbmcgui.Window(10000).getProperty(scraper)
    if previoustime:
        timeout = currenttime - int(previoustime)
        if timeout < 1000:
            xbmc.sleep(1000 - timeout)
    xbmcgui.Window(10000).setProperty(scraper, str(round(time.time() * 1000)))


class Scraper():
    def __init__(self, action, key, artist, url, nfo, settings):
        # parse path settings
        self.parse_settings(settings)
        # this is just for backward compitability with xml based scrapers https://github.com/xbmc/xbmc/pull/11632
        if action == 'resolveid':
            # return the result
            result = self.resolve_mbid(key)
            self.return_resolved(result)
        # search for artist name matches
        elif action == 'find':
            # try musicbrainz first
            result = self.find_artist(artist, 'musicbrainz')
            if result:
                self.return_search(result)
            # fallback to discogs
            else:
                result = self.find_artist(artist, 'discogs')
                if result:
                    self.return_search(result)
        # return info using id's
        elif action == 'getdetails':
            details = {}
            discography = {}
            mvids = {}
            url = json.loads(url)
            artist = url.get('artist')
            mbartistid = url.get('mbartistid')
            dcid = url.get('dcid')
            threads = []
            extrascrapers = []
            discographyscrapers = []
            # we have a musicbrainz id
            if mbartistid:
                scrapers = [[mbartistid, 'musicbrainz'], [mbartistid, 'theaudiodb'], [mbartistid, 'fanarttv']]
                for item in scrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
                # theaudiodb discograhy
                thread = Thread(target = self.get_discography, args = (mbartistid, 'theaudiodb', discography))
                threads.append(thread)
                thread.start()
                # theaudiodb mvids
                thread = Thread(target = self.get_videolinks, args = (mbartistid, 'theaudiodb', mvids))
                threads.append(thread)
                thread.start()
                # wait for musicbrainz to finish
                threads[0].join()
                # check if we have a result:
                if 'musicbrainz' in details:
                    if not artist:
                        artist = details['musicbrainz']['artist']
                    # scrape allmusic if we have an url provided by musicbrainz
                    if 'allmusic' in details['musicbrainz']:
                        extrascrapers.append([{'url': details['musicbrainz']['allmusic']}, 'allmusic'])
                        # allmusic discograhy
                        discographyscrapers.append([{'url': details['musicbrainz']['allmusic']}, 'allmusic'])
                    # only scrape allmusic by artistname if explicitly enabled
                    elif self.inaccurate and artist:
                        extrascrapers.append([{'artist': artist}, 'allmusic'])
                    # scrape wikipedia if we have an url provided by musicbrainz
                    if 'wikipedia' in details['musicbrainz']:
                        extrascrapers.append([details['musicbrainz']['wikipedia'], 'wikipedia'])
                    elif 'wikidata' in details['musicbrainz']:
                        extrascrapers.append([details['musicbrainz']['wikidata'], 'wikidata'])
                    # scrape discogs if we have an url provided by musicbrainz
                    if 'discogs' in details['musicbrainz']:
                        extrascrapers.append([{'url': details['musicbrainz']['discogs']}, 'discogs'])
                        # discogs discograhy
                        discographyscrapers.append([{'url': details['musicbrainz']['discogs']}, 'discogs'])
                    # only scrape discogs by artistname if explicitly enabled
                    elif self.inaccurate and artist:
                        extrascrapers.append([{'artist': artist}, 'discogs'])
                    for item in extrascrapers:
                        thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                        threads.append(thread)
                        thread.start()
                    # get allmusic / discogs discography if we have an url
                    for item in discographyscrapers:
                        thread = Thread(target = self.get_discography, args = (item[0], item[1], discography))
                        threads.append(thread)
                        thread.start()
            # we have a discogs id
            else:
                thread = Thread(target = self.get_details, args = ({'url': dcid}, 'discogs', details))
                threads.append(thread)
                thread.start()
                thread = Thread(target = self.get_discography, args = ({'url': dcid}, 'discogs', discography))
                threads.append(thread)
                thread.start()
            if threads:
                for thread in threads:
                    thread.join()
            # merge discography items
            for site, albumlist in discography.items():
                if site in details:
                    details[site]['albums'] = albumlist
                else:
                    details[site] = {}
                    details[site]['albums'] = albumlist
            for site, mvidlist in mvids.items():
                if site in details:
                    details[site]['mvids'] = mvidlist
                else:
                    details[site] = {}
                    details[site]['mvids'] = mvidlist
            result = self.compile_results(details)
            if result:
                self.return_details(result)
        elif action == 'NfoUrl':
            # check if there is a musicbrainz url in the nfo file
            mbartistid = nfo_geturl(nfo)
            if mbartistid:
                # return the result
                result = self.resolve_mbid(mbartistid)
                self.return_nfourl(result)
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
        self.inaccurate = settings['inaccurate']

    def resolve_mbid(self, mbartistid):
        item = {}
        item['artist'] = ''
        item['mbartistid'] = mbartistid
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

    def get_details(self, param, site, details, discography={}):
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
            # search by artistname if we do not have an url
            if 'artist' in param:
                url = DISCOGSURL % (DISCOGSSEARCH % (urllib.parse.quote_plus(param['artist']), DISCOGSKEY , DISCOGSSECRET))
                artistresult = get_data(url, json)
                if artistresult:
                    artists = discogs_artistfind(artistresult, param['artist'])
                    if artists:
                        artistresult = sorted(artists, key=lambda k: k['relevance'], reverse=True)
                        param['url'] = artistresult[0]['dcid']
                    else:
                        return
                else:
                    return
            url = DISCOGSURL % (DISCOGSDETAILS % (param['url'], DISCOGSKEY, DISCOGSSECRET))
            artistscraper = discogs_artistdetails
        # wikipedia
        elif site == 'wikipedia':
            url = WIKIPEDIAURL % param
            artistscraper = wikipedia_artistdetails
        elif site == 'wikidata':
            # resolve wikidata to wikipedia url
            result = get_data(WIKIDATAURL % param, json)
            try:
                artist = result['entities'][param]['sitelinks']['enwiki']['url'].rsplit('/', 1)[1]
            except:
                return
            site = 'wikipedia'
            url = WIKIPEDIAURL % artist
            artistscraper = wikipedia_artistdetails
        # allmusic
        elif site == 'allmusic':
            json = False
            # search by artistname if we do not have an url
            if 'artist' in param:
                url = ALLMUSICURL % urllib.parse.quote_plus(param['artist'])
                artistresult = get_data(url, json)
                if artistresult:
                    artists = allmusic_artistfind(artistresult, param['artist'])
                    if artists:
                        param['url'] = artists[0]['url']
                    else:
                        return
                else:
                    return
            url = param['url']
            artistscraper = allmusic_artistdetails
        result = get_data(url, json)
        if not result:
            return
        artistresults = artistscraper(result)
        if not artistresults:
            return
        details[site] = artistresults
        # get allmusic / discogs discography if we searched by artistname
        if (site == 'discogs' or site == 'allmusic') and 'artist' in param:
            albums = self.get_discography(param, site, {})
            if albums:
                details[site]['albums'] = albums[site]
        return details

    def get_discography(self, param, site, discography):
        json = True
        if site == 'theaudiodb':
            # theaudiodb - discography
            albumsurl = AUDIODBURL % (AUDIODBKEY, AUDIODBDISCOGRAPHY % param)
            scraper = theaudiodb_artistalbums
        elif site == 'discogs':
            # discogs - discography
            albumsurl = DISCOGSURL % (DISCOGSDISCOGRAPHY % (param['url'], DISCOGSKEY, DISCOGSSECRET))
            scraper = discogs_artistalbums
        elif site == 'allmusic':
            # allmusic - discography
            json = False
            albumsurl = param['url'] + '/discography'
            scraper = allmusic_artistalbums
        albumdata = get_data(albumsurl, json)
        if not albumdata:
            return
        albumresults = scraper(albumdata)
        if not albumresults:
            return
        discography[site] = albumresults
        return discography

    def get_videolinks(self, param, site, videolinks):
        json = True
        if site == 'theaudiodb':
            #theaudiodb mvid links
            mvidurl = AUDIODBURL % (AUDIODBKEY, AUDIODBMVIDS % param)
            scraper = theaudiodb_mvids
        mviddata = get_data(mvidurl, json)
        if not mviddata:
            return
        mvidresults = scraper(mviddata)
        if not mvidresults:
            return
        videolinks[site] = mvidresults
        return videolinks

    def compile_results(self, details):
        result = {}
        thumbs = []
        fanart = []
        extras = []
        # merge metadata results, start with the least accurate sources
        if 'discogs' in details:
            for k, v in details['discogs'].items():
                if v:
                    result[k] = v
                if k == 'thumb' and v:
                    thumbs.append(v)
        if 'wikipedia' in details:
            for k, v in details['wikipedia'].items():
                if v:
                    result[k] = v
        if 'allmusic' in details:
            for k, v in details['allmusic'].items():
                if v:
                    result[k] = v
                if k == 'thumb' and v:
                    thumbs.append(v)
        if 'theaudiodb' in details:
            for k, v in details['theaudiodb'].items():
                if v:
                    result[k] = v
                if k == 'thumb' and v:
                    thumbs.append(v)
                elif k == 'fanart' and v:
                    fanart.append(v)
                if k == 'extras' and v:
                    extras.append(v)
        if 'musicbrainz' in details:
            for k, v in details['musicbrainz'].items():
                if v:
                    result[k] = v
        if 'fanarttv' in details:
            for k, v in details['fanarttv'].items():
                if v:
                    result[k] = v
                if k == 'thumb' and v:
                    thumbs.append(v)
                elif k == 'fanart' and v:
                    fanart.append(v)
                if k == 'extras' and v:
                    extras.append(v)
        # merge artwork from all scrapers
        if result:
            # artworks from most accurate sources first
            thumbs.reverse()
            thumbnails = []
            fanart.reverse()
            fanarts = []
            # extra art from most accurate sources first
            extras.reverse()
            extraart = []
            for thumblist in thumbs:
                for item in thumblist:
                    thumbnails.append(item)
            for extralist in extras:
                for item in extralist:
                    extraart.append(item)
            # add the extra art to the end of the thumb list
            if extraart:
                thumbnails.extend(extraart)
            for fanartlist in fanart:
                for item in fanartlist:
                    fanarts.append(item)
            # add the fanart to the end of the thumb list
            if fanarts:
                thumbnails.extend(fanarts)
            if thumbnails:
                result['thumb'] = thumbnails
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
        elif (self.bio in details) and ('biography' in details[self.bio]):
            result['biography'] = details[self.bio]['biography']
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
        items = []
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
            if 'mbartistid' in item:
                url['mbartistid'] = item['mbartistid']
            if 'dcid' in item:
                url['dcid'] = item['dcid']
            items.append((json.dumps(url), listitem, True))
        if items:
            xbmcplugin.addDirectoryItems(handle=int(sys.argv[1]), items=items)

    def return_nfourl(self, item):
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=json.dumps(item), listitem=listitem, isFolder=True)

    def return_resolved(self, item):
        listitem = xbmcgui.ListItem(path=json.dumps(item), offscreen=True)
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
        if 'mvids' in item:
            listitem.setProperty('artist.videolinks', str(len(item['mvids'])))
            for count, mvid in enumerate(item['mvids']):
                listitem.setProperty('artist.videolink%i.title' % (count + 1), mvid['title'])
                listitem.setProperty('artist.videolink%i.mbtrackid' % (count + 1), mvid['mbtrackid'])
                listitem.setProperty('artist.videolink%i.url' % (count + 1), mvid['url'])
                listitem.setProperty('artist.videolink%i.thumb' % (count + 1), mvid['thumb'])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)
