# -*- coding: utf-8 -*-

import re

def nfo_geturl(data):
    result = re.search(r'https://musicbrainz.org/(ws/2/)?artist/([0-9a-z\-]*)', data)
    if result:
        return result.group(2)
