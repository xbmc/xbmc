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
from .allmusic import allmusic_albumfind
from .allmusic import allmusic_albumdetails
from .discogs import discogs_albumfind
from .discogs import discogs_albummain
from .discogs import discogs_albumdetails
from .fanarttv import fanarttv_albumart
from .musicbrainz import musicbrainz_albumfind
from .musicbrainz import musicbrainz_albumdetails
from .musicbrainz import musicbrainz_albumlinks
from .musicbrainz import musicbrainz_albumart
from .nfo import nfo_geturl
from .theaudiodb import theaudiodb_albumdetails
from .wikipedia import wikipedia_albumdetails
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
    def __init__(self, action, key, artist, album, url, nfo, settings):
        # parse path settings
        self.parse_settings(settings)
        # this is just for backward compitability with xml based scrapers https://github.com/xbmc/xbmc/pull/11632
        if action == 'resolveid':
            # return the result
            result = self.resolve_mbid(key)
            self.return_resolved(result)
        # search for artist name / album title matches
        elif action == 'find':
            # try musicbrainz first
            result = self.find_album(artist, album, 'musicbrainz')
            if result:
                self.return_search(result)
            # fallback to discogs
            else:
                result = self.find_album(artist, album, 'discogs')
                if result:
                    self.return_search(result)
        # return info id's
        elif action == 'getdetails':
            details = {}
            links = {}
            url = json.loads(url)
            artist = url.get('artist')
            album = url.get('album')
            mbalbumid = url.get('mbalbumid')
            mbreleasegroupid = url.get('mbreleasegroupid')
            dcid = url.get('dcalbumid')
            threads = []
            extrascrapers = []
            # we have musicbrainz album id
            if mbalbumid:
                # get the mbreleasegroupid, artist and album if we don't have them
                if not mbreleasegroupid:
                    result = self.get_details(mbalbumid, 'musicbrainz', details)
                    if not result:
                        scrapers = [[mbalbumid, 'musicbrainz']]
                    else:
                        mbreleasegroupid = details['musicbrainz']['mbreleasegroupid']
                        artist = details['musicbrainz']['artist_description']
                        album = details['musicbrainz']['album']
                        scrapers = [[mbreleasegroupid, 'theaudiodb'], [mbreleasegroupid, 'fanarttv'], [mbreleasegroupid, 'coverarchive']]
                else:
                    scrapers = [[mbalbumid, 'musicbrainz'], [mbreleasegroupid, 'theaudiodb'], [mbreleasegroupid, 'fanarttv'], [mbreleasegroupid, 'coverarchive']]
                # get musicbrainz links to other metadata sites
                lthread = Thread(target = self.get_links, args = (mbreleasegroupid, links))
                lthread.start()
                for item in scrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
                # wait for the musicbrainz links to return
                lthread.join()
                if 'musicbrainz' in links:
                    # scrape allmusic if we have an url provided by musicbrainz
                    if 'allmusic' in links['musicbrainz']:
                        extrascrapers.append([{'url': links['musicbrainz']['allmusic']}, 'allmusic'])
                    # only scrape allmusic by artistname and albumtitle if explicitly enabled
                    elif self.inaccurate and artist and album:
                        extrascrapers.append([{'artist': artist, 'album': album}, 'allmusic'])
                    # scrape discogs if we have an url provided by musicbrainz
                    if 'discogs' in links['musicbrainz']:
                        extrascrapers.append([{'masterurl': links['musicbrainz']['discogs']}, 'discogs'])
                    # only scrape discogs by artistname and albumtitle if explicitly enabled
                    elif self.inaccurate and artist and album:
                        extrascrapers.append([{'artist': artist, 'album': album}, 'discogs'])
                    # scrape wikipedia if we have an url provided by musicbrainz
                    if 'wikipedia' in links['musicbrainz']:
                        extrascrapers.append([links['musicbrainz']['wikipedia'], 'wikipedia'])
                    elif 'wikidata' in links['musicbrainz']:
                        extrascrapers.append([links['musicbrainz']['wikidata'], 'wikidata'])
                for item in extrascrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
            # we have a discogs id
            else:
                thread = Thread(target = self.get_details, args = ({'url': dcid}, 'discogs', details))
                threads.append(thread)
                thread.start()
            for thread in threads:
                thread.join()
            result = self.compile_results(details)
            if result:
                self.return_details(result)
        # extract the mbalbumid from the provided musicbrainz url
        elif action == 'NfoUrl':
            # check if there is a musicbrainz url in the nfo file
            mbalbumid = nfo_geturl(nfo)
            if mbalbumid:
                # return the result
                result = self.resolve_mbid(mbalbumid)
                self.return_nfourl(result)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))

    def parse_settings(self, data):
        settings = json.loads(data)
        # note: path settings are taken from the db, they may not reflect the current settings.xml file
        self.review = settings['review']
        self.genre = settings['genre']
        self.lang = settings['lang']
        self.mood = settings['mood']
        self.rating = settings['rating']
        self.style = settings['style']
        self.theme = settings['theme']
        self.inaccurate = settings['inaccurate']

    def resolve_mbid(self, mbalbumid):
        item = {}
        item['artist_description'] = ''
        item['album'] = ''
        item['mbalbumid'] = mbalbumid
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
        result = get_data(url, json)
        if not result:
            return
        albumresults = scraper(result, artist, album)
        return albumresults

    def get_links(self, param, links):
        json = True
        url = MUSICBRAINZURL % (MUSICBRAINZLINKS % param)
        result = get_data(url, json)
        if result:
            linkresults = musicbrainz_albumlinks(result)
            links['musicbrainz'] = linkresults
            return links

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
            # musicbrainz provides a link to the master release, but we need the main release
            if 'masterurl' in param:
                masterdata = get_data(DISCOGSURL % (DISCOGSMASTER % (param['masterurl'], DISCOGSKEY , DISCOGSSECRET)), True)
                if masterdata:
                    url = discogs_albummain(masterdata)
                    if url:
                        param['url'] = url
                    else:
                        return
                else:
                    return
            # search by artistname and albumtitle if we do not have an url
            if not 'url' in param:
                url = DISCOGSURL % (DISCOGSSEARCH % (urllib.parse.quote_plus(param['album']), urllib.parse.quote_plus(param['artist']), DISCOGSKEY , DISCOGSSECRET))
                albumresult = get_data(url, json)
                if albumresult:
                    albums = discogs_albumfind(albumresult, param['artist'], param['album'])
                    if albums:
                        albumresult = sorted(albums, key=lambda k: k['relevance'], reverse=True)
                        param['url'] = albumresult[0]['dcalbumid']
                    else:
                        return
                else:
                    return
            url = DISCOGSURL % (DISCOGSDETAILS % (param['url'], DISCOGSKEY, DISCOGSSECRET))
            albumscraper = discogs_albumdetails
        # wikipedia
        elif site == 'wikipedia':
            url = WIKIPEDIAURL % param
            albumscraper = wikipedia_albumdetails
        elif site == 'wikidata':
            # resolve wikidata to wikipedia url
            result = get_data(WIKIDATAURL % param, json)
            try:
                album = result['entities'][param]['sitelinks']['enwiki']['url'].rsplit('/', 1)[1]
            except:
                return
            site = 'wikipedia'
            url = WIKIPEDIAURL % album
            albumscraper = wikipedia_albumdetails
        # allmusic
        elif site == 'allmusic':
            json = False
            # search by artistname and albumtitle if we do not have an url
            if not 'url' in param:
                url = ALLMUSICURL % (ALLMUSICSEARCH % (urllib.parse.quote_plus(param['artist']), urllib.parse.quote_plus(param['album'])))
                albumresult = get_data(url, json)
                if albumresult:
                    albums = allmusic_albumfind(albumresult, param['artist'], param['album'])
                    if albums:
                        param['url'] = albums[0]['url']
                    else:
                        return
                else:
                    return
            url = ALLMUSICDETAILS % param['url']
            albumscraper = allmusic_albumdetails
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
                if k == 'extras' and v:
                    extras.append(v)
        if 'musicbrainz' in details:
            for k, v in details['musicbrainz'].items():
                if v:
                    result[k] = v
        if 'coverarchive' in details:
            for k, v in details['coverarchive'].items():
                if v:
                    result[k] = v
                if k == 'thumb' and v:
                    thumbs.append(v)
                if k == 'extras' and v:
                    extras.append(v)
        # prefer artwork from fanarttv
        if 'fanarttv' in details:
            for k, v in details['fanarttv'].items():
                if v:
                    result[k] = v
                if k == 'thumb' and v:
                    thumbs.append(v)
                if k == 'extras' and v:
                    extras.append(v)
        # use musicbrainz artist as it provides the mbartistid (used for resolveid in the artist scraper)
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
            if extraart:
                thumbnails.extend(extraart)
            if thumbnails:
                result['thumb'] = thumbnails
        data = self.user_prefs(details, result)
        return data

    def user_prefs(self, details, result):
        # user preferences
        lang = 'description' + self.lang
        if self.review == 'theaudiodb' and 'theaudiodb' in details:
            if lang in details['theaudiodb']:
                result['description'] = details['theaudiodb'][lang]
            elif 'descriptionEN' in details['theaudiodb']:
                result['description'] = details['theaudiodb']['descriptionEN']
        elif (self.review in details) and ('description' in details[self.review]):
            result['description'] = details[self.review]['description']
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
        items = []
        for item in data:
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
                url['mbreleasegroupid'] = item['mbreleasegroupid']
            if 'dcalbumid' in item:
                url['dcalbumid'] = item['dcalbumid']
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
        if 'thumb' in item:
            listitem.setProperty('album.thumbs', str(len(item['thumb'])))
            for count, thumb in enumerate(item['thumb']):
                listitem.setProperty('album.thumb%i.url' % (count + 1), thumb['image'])
                listitem.setProperty('album.thumb%i.aspect' % (count + 1), thumb['aspect'])
                listitem.setProperty('album.thumb%i.preview' % (count + 1), thumb['preview'])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)
