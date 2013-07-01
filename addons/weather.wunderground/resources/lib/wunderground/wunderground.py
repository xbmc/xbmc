# -*- coding: utf-8 -*-

import urllib2, gzip, base64
from StringIO import StringIO

WAIK             = 'NDEzNjBkMjFkZjFhMzczNg=='
WUNDERGROUND_URL = 'http://api.wunderground.com/api/%s/%s/%s/q/%s.%s'
API_EXCLUDE      = ['hourly10day', 'yesterday', 'planner', 'webcams', 'animatedradar', 'animatedsatellite', 'currenthurricane']

def wundergroundapi(features, settings, query, format):

    """
    wunderground api module
    
    How to use:
    
    1) import the wunderground addon in your addon.xml:
            <requires>
                <import addon="weather.wunderground" version="0.1.12"/>
            </requires>
    
    2) import the wunderground api module in your script:
            from wunderground import wundergroundapi
    
    3) to fetch weather data:
            weatherdata = wundergroundapi(features, settings, query, format)
    
    see http://www.wunderground.com/weather/api/d/docs?d=data/index
    for api features, optional settings, query examples and response formats.
    """

    for item in API_EXCLUDE:
        if item in features:
            return 'api access to %s restricted' % item
    if not settings:
        settings = 'lang:EN'
    url = WUNDERGROUND_URL % (base64.b64decode(WAIK)[::-1], features, settings, query, format)
    try:
        req = urllib2.Request(url)
        req.add_header('Accept-encoding', 'gzip')
        response = urllib2.urlopen(req)
        if response.info().get('Content-Encoding') == 'gzip':
            buf = StringIO(response.read())
            compr = gzip.GzipFile(fileobj=buf)
            data = compr.read()
        else:
            data = response.read()
        response.close()
    except:
        data = ''
    return data
