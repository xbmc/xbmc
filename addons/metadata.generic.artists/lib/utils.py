# -*- coding: utf-8 -*-

AUDIODBKEY = '58424d43204d6564696120'
AUDIODBURL = 'https://www.theaudiodb.com/api/v1/json/%s/%s'
AUDIODBSEARCH = 'search.php?s=%s'
AUDIODBDETAILS = 'artist-mb.php?i=%s'
AUDIODBDISCOGRAPHY = 'discography-mb.php?s=%s'

MUSICBRAINZURL = 'https://musicbrainz.org/ws/2/artist/%s'
MUSICBRAINZSEARCH = '?query="%s"&fmt=json'
MUSICBRAINZDETAILS = '%s?inc=url-rels+release-groups&type=album&fmt=json'

DISCOGSKEY = 'zACPgktOmNegwbwKWMaC'
DISCOGSSECRET = 'wGuSOeMtfdkQxtERKQKPquyBwExSHdQq'
DISCOGSURL = 'https://api.discogs.com/%s'
DISCOGSSEARCH = 'database/search?q=%s&type=artist&key=%s&secret=%s'
DISCOGSDETAILS = 'artists/%i?key=%s&secret=%s'
DISCOGSDISCOGRAPHY = 'artists/%i/releases?sort=format&page=1&per_page=100&key=%s&secret=%s'

ALLMUSICURL = 'https://www.allmusic.com/%s'
ALLMUSICSEARCH = 'search/artists/%s'

FANARTVKEY = 'ed4b784f97227358b31ca4dd966a04f1'
FANARTVURL = 'https://webservice.fanart.tv/v3/music/%s?api_key=%s'
