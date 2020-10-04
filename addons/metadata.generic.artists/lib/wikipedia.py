# -*- coding: utf-8 -*-
import re

def wikipedia_artistdetails(data):
    artistdata = {}
    # check in case musicbrainz did not provide a direct link
    if 'extract' in data['query']['pages'][0] and not data['query']['pages'][0]['extract'].endswith('may refer to:'):
        artistdata['biography'] = re.sub('\n\n\n== .*? ==\n', ' ', data['query']['pages'][0]['extract'])
    return artistdata
