# -*- coding: utf-8 -*-
import sys
from urllib.parse import parse_qsl
from lib.scraper import Scraper


class Main:
    def __init__(self):
        action, key, artist, url, nfo, settings = self._parse_argv()
        Scraper(action, key, artist, url, nfo, settings)

    def _parse_argv(self):
        params = dict(parse_qsl(sys.argv[2].lstrip('?')))
        # actions: find, resolveid, NfoUrl, getdetails
        action = params['action']
        # key: musicbrainz id
        key = params.get('key', '')
        # artist: artistname
        artist = params.get('artist', '')
        # url: provided by the scraper on previous run
        url = params.get('url', '')
        # nfo: musicbrainz url from .nfo file
        nfo = params.get('nfo', '')
        # path specific settings
        settings = params.get('pathSettings', {})
        return action, key, artist, url, nfo, settings


if (__name__ == '__main__'):
    Main()
