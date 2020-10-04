# -*- coding: utf-8 -*-
import re

def wikipedia_albumdetails(data):
    albumdata = {}
    # check in case musicbrainz did not provide a direct link
    if 'extract' in data['query']['pages'][0] and not data['query']['pages'][0]['extract'].endswith('may refer to:'):
        albumdata['description'] = re.sub('\n\n\n== .*? ==\n', ' ', data['query']['pages'][0]['extract'])
    return albumdata
