# -*- coding: utf-8 -*-

import re

def nfo_geturl(data):
    result = re.search('https://musicbrainz.org/(ws/2/)?release/([0-9a-z\-]*)', data)
    if result:
        return result.group(2)
