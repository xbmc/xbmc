# -*- coding: utf-8 -*-

AUDIODBKEY = '95424d43204d6564696538'
AUDIODBURL = 'https://www.theaudiodb.com/api/v1/json/%s/%s'
AUDIODBSEARCH = 'searchalbum.php?s=%s&a=%s'
AUDIODBDETAILS = 'album-mb.php?i=%s'

MUSICBRAINZURL = 'https://musicbrainz.org/ws/2/%s'
MUSICBRAINZSEARCH = 'release/?query=release:"%s"%%20AND%%20(artistname:"%s"%%20OR%%20artist:"%s")&fmt=json'
MUSICBRAINZLINKS = 'release-group/%s?inc=url-rels&fmt=json'
MUSICBRAINZDETAILS = 'release/%s?inc=release-groups+artists+labels+ratings&fmt=json'
MUSICBRAINZART = 'https://coverartarchive.org/release-group/%s'

DISCOGSKEY = 'zACPgktOmNegwbwKWMaC'
DISCOGSSECRET = 'wGuSOeMtfdkQxtERKQKPquyBwExSHdQq'
DISCOGSURL = 'https://api.discogs.com/%s'
DISCOGSSEARCH = 'database/search?release_title=%s&type=release&artist=%s&page=1&per_page=100&key=%s&secret=%s'
DISCOGSMASTER = 'masters/%s?key=%s&secret=%s'
DISCOGSDETAILS = 'releases/%s?key=%s&secret=%s'

ALLMUSICURL = 'https://www.allmusic.com/%s'
ALLMUSICSEARCH = 'search/albums/%s+%s'
ALLMUSICDETAILS = '%s/releases'

FANARTVKEY = '88ca41db0d6878929f1f9771eade41fd'
FANARTVURL = 'https://webservice.fanart.tv/v3/music/albums/%s?api_key=%s'

WIKIDATAURL = 'https://www.wikidata.org/wiki/Special:EntityData/%s.json'
WIKIPEDIAURL = 'https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=%s&formatversion=2&exsentences=10&exlimit=1&explaintext=1'
