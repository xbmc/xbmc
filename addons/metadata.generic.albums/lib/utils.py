# -*- coding: utf-8 -*-

AUDIODBKEY = '58424d43204d6564696120'
AUDIODBURL = 'https://www.theaudiodb.com/api/v1/json/%s/%s'
AUDIODBSEARCH = 'searchalbum.php?s=%s&a=%s'
AUDIODBDETAILS = 'album-mb.php?i=%s'

MUSICBRAINZURL = 'https://musicbrainz.org/ws/2/release/%s'
MUSICBRAINZSEARCH = '?query=release:"%s"%%20AND%%20(artistname:"%s"%%20OR%%20artist:"%s")&fmt=json'
MUSICBRAINZDETAILS = '%s?inc=recordings+release-groups+artists+labels+ratings&fmt=json'
MUSICBRAINZART = 'https://coverartarchive.org/release-group/%s'

DISCOGSKEY = 'zACPgktOmNegwbwKWMaC'
DISCOGSSECRET = 'wGuSOeMtfdkQxtERKQKPquyBwExSHdQq'
DISCOGSURL = 'https://api.discogs.com/%s'
DISCOGSSEARCH = 'database/search?release_title=%s&type=release&artist=%s&page=1&per_page=100&key=%s&secret=%s'
DISCOGSDETAILS = 'releases/%i?key=%s&secret=%s'

ALLMUSICURL = 'https://www.allmusic.com/%s'
ALLMUSICSEARCH = 'search/albums/%s+%s'
ALLMUSICDETAILS = '%s/releases'

FANARTVKEY = 'ed4b784f97227358b31ca4dd966a04f1'
FANARTVURL = 'https://webservice.fanart.tv/v3/music/albums/%s?api_key=%s'
