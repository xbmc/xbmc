# -*- coding: utf-8 -*-

import urllib2, base64

WAIK             = 'NDEzNjBkMjFkZjFhMzczNg=='
WUNDERGROUND_URL = 'http://api.wunderground.com/api/KEY/%s/%s/q/%s.%s'
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
    query = WUNDERGROUND_URL % (features, settings, query, format)
    url = query.replace('KEY',(base64.b64decode(WAIK)[::-1]),1)
    try:
        req = urllib2.urlopen(url)
        response = req.read()
        req.close()
    except:
        response = ''
    return response
