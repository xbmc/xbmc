# -*- coding: utf-8 -*-

# *  This Program is free software; you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 2, or (at your option)
# *  any later version.
# *
# *  This Program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with XBMC; see the file COPYING. If not, write to
# *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
# *  http://www.gnu.org/copyleft/gpl.html


import os, sys, socket, unicodedata, urllib2, time, base64
from datetime import date
import xbmc, xbmcgui, xbmcaddon, xbmcvfs
if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__      = xbmcaddon.Addon()
__addonname__  = __addon__.getAddonInfo('name')
__addonid__    = __addon__.getAddonInfo('id')
__cwd__        = __addon__.getAddonInfo('path').decode("utf-8")
__version__    = __addon__.getAddonInfo('version')
__language__   = __addon__.getLocalizedString
__resource__   = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'lib' ).encode("utf-8") ).decode("utf-8")

sys.path.append(__resource__)

from utilities import *
from wunderground import wundergroundapi

WUNDERGROUND_LOC = 'http://autocomplete.wunderground.com/aq?query=%s&format=JSON'
WEATHER_FEATURES = 'hourly/conditions/forecast10day/astronomy/almanac/alerts/satellite'
FORMAT           = 'json'
DEBUG            = __addon__.getSetting('Debug')
WEATHER_ICON     = xbmc.translatePath('special://temp/weather/%s.png').decode("utf-8")
WEATHER_WINDOW   = xbmcgui.Window(12600)
LANGUAGE         = xbmc.getLanguage().lower()
SPEEDUNIT        = xbmc.getRegion('speedunit')
TEMPUNIT         = unicode(xbmc.getRegion('tempunit'),encoding='utf-8')
TIMEFORMAT       = xbmc.getRegion('meridiem')
DATEFORMAT       = xbmc.getRegion('dateshort')
MAXDAYS          = 6

socket.setdefaulttimeout(10)

def log(txt):
    if DEBUG == 'true':
        if isinstance (txt,str):
            txt = txt.decode("utf-8")
        message = u'%s: %s' % (__addonid__, txt)
        xbmc.log(msg=message.encode("utf-8"), level=xbmc.LOGDEBUG)

def set_property(name, value):
    WEATHER_WINDOW.setProperty(name, value)

def refresh_locations():
    locations = 0
    for count in range(1, 4):
        loc_name = __addon__.getSetting('Location%s' % count)
        if loc_name != '':
            locations += 1
        else:
            __addon__.setSetting('Location%sid' % count, '')
        set_property('Location%s' % count, loc_name)
    set_property('Locations', str(locations))
    log('available locations: %s' % str(locations))

def find_location(loc):
    url = WUNDERGROUND_LOC % urllib2.quote(loc)
    try:
        req = urllib2.urlopen(url)
        response = req.read()
        req.close()
    except:
        response = ''
    return response

def location(string):
    locs   = []
    locids = []
    log('location: %s' % string)
    loc = unicodedata.normalize('NFKD', unicode(string, 'utf-8')).encode('ascii','ignore')
    log('searching for location: %s' % loc)
    query = find_location(loc)
    log('location data: %s' % query)
    data = parse_data(query)
    if data != '' and data.has_key('RESULTS'):
        for item in data['RESULTS']:
            location   = item['name']
            locationid = item['l'][3:]
            locs.append(location)
            locids.append(locationid)
    return locs, locids

def geoip():
    retry = 0
    while (retry < 6) and (not xbmc.abortRequested):
        query = wundergroundapi('geolookup', None, 'autoip', FORMAT)
        if query != '':
            retry = 6
        else:
            retry += 1
            xbmc.sleep(10000)
            log('geoip download failed')
    log('geoip data: %s' % query)
    data = parse_data(query)
    if data != '' and data.has_key('location'):
        location   = data['location']['city']
        locationid = data['location']['l'][3:]
        __addon__.setSetting('Location1', location)
        __addon__.setSetting('Location1id', locationid)
        log('geoip location: %s' % location)
    else:
        locationid = ''
    return locationid

def forecast(loc):
    try:
        lang = LANG[LANGUAGE]
    except:
        lang = 'EN'
    opt = 'lang:' + lang
    log('weather location: %s' % loc)
    retry = 0
    while (retry < 6) and (not xbmc.abortRequested):
        query = wundergroundapi(WEATHER_FEATURES, opt, loc, FORMAT)
        if query != '':
            retry = 6
        else:
            retry += 1
            xbmc.sleep(10000)
            log('weather download failed')
    log('forecast data: %s' % query)
    data = parse_data(query)
    if data != '' and not data.has_key('error'):
        properties(data,loc)
    else:
        clear()

def clear():
    set_property('Current.Condition'     , 'N/A')
    set_property('Current.Temperature'   , '0')
    set_property('Current.Wind'          , '0')
    set_property('Current.WindDirection' , 'N/A')
    set_property('Current.Humidity'      , '0')
    set_property('Current.FeelsLike'     , '0')
    set_property('Current.UVIndex'       , '0')
    set_property('Current.DewPoint'      , '0')
    set_property('Current.OutlookIcon'   , 'na.png')
    set_property('Current.FanartCode'    , 'na')
    for count in range (0, MAXDAYS+1):
        set_property('Day%i.Title'       % count, 'N/A')
        set_property('Day%i.HighTemp'    % count, '0')
        set_property('Day%i.LowTemp'     % count, '0')
        set_property('Day%i.Outlook'     % count, 'N/A')
        set_property('Day%i.OutlookIcon' % count, 'na.png')
        set_property('Day%i.FanartCode'  % count, 'na')

def parse_data(json):
    try:
        raw = json.replace('<br>',' ').replace('&auml;','ä') # wu api bugs
        reply = raw.replace('"-999%"','""').replace('"-9999.00"','""').replace('"-9998"','""').replace('"NA"','""') # wu will change these to null responses in the future
        data = simplejson.loads(reply)
    except:
        log('failed to parse weather data')
        data = ''
    return data

def properties(data,loc):
# standard properties
    weathercode = WEATHER_CODES[data['current_observation']['icon_url'][31:-4]]
    location = __addon__.getSetting('Location%s' % sys.argv[1])
    if (location == '') and (sys.argv[1] != '1'):
        location = __addon__.getSetting('Location1')
    set_property('Current.Location'      , location)
    set_property('Current.Condition'     , data['current_observation']['weather'])
    set_property('Current.Temperature'   , str(data['current_observation']['temp_c']))
    set_property('Current.Wind'          , str(data['current_observation']['wind_kph']))
    set_property('Current.WindDirection' , data['current_observation']['wind_dir'])
    set_property('Current.Humidity'      , data['current_observation']['relative_humidity'].rstrip('%'))
    set_property('Current.FeelsLike'     , data['current_observation']['feelslike_c'])
    set_property('Current.UVIndex'       , data['current_observation']['UV'])
    set_property('Current.DewPoint'      , str(data['current_observation']['dewpoint_c']))
    set_property('Current.OutlookIcon'   , '%s.png' % weathercode)
    set_property('Current.FanartCode'    , weathercode)
    for count, item in enumerate(data['forecast']['simpleforecast']['forecastday']):
        weathercode = WEATHER_CODES[item['icon_url'][31:-4]]
        set_property('Day%i.Title'       % count, item['date']['weekday'])
        set_property('Day%i.HighTemp'    % count, str(item['high']['celsius']))
        set_property('Day%i.LowTemp'     % count, str(item['low']['celsius']))
        set_property('Day%i.Outlook'     % count, item['conditions'])
        set_property('Day%i.OutlookIcon' % count, '%s.png' % weathercode)
        set_property('Day%i.FanartCode'  % count, weathercode)
        if count == MAXDAYS:
            break
# forecast properties
    set_property('Forecast.IsFetched'        , 'true')
    set_property('Forecast.City'             , data['current_observation']['display_location']['city'])
    set_property('Forecast.State'            , data['current_observation']['display_location']['state_name'])
    set_property('Forecast.Country'          , data['current_observation']['display_location']['country'])
    update = time.localtime(float(data['current_observation']['observation_epoch']))
    if DATEFORMAT[1] == 'm':
        localdate = WEEKDAY[update[6]] + ' ' + MONTH[update[1]] + ' ' + str(update[2]) + ', ' + str(update[0])
    else:
        localdate = WEEKDAY[update[6]] + ' ' + str(update[2]) + ' ' + MONTH[update[1]] + ' ' + str(update[0])
    if TIMEFORMAT != '/':
        localtime = time.strftime('%I:%M%p', update)
    else:
        localtime = time.strftime('%H:%M', update)
    set_property('Forecast.Updated'          , localdate + ' - ' + localtime)
# current properties
    set_property('Current.IsFetched'         , 'true')
    set_property('Current.WindDegree'        , str(data['current_observation']['wind_degrees']) + u'°')
    set_property('Current.SolarRadiation'    , str(data['current_observation']['solarradiation']))
    if 'F' in TEMPUNIT:
        set_property('Current.Pressure'      , data['current_observation']['pressure_in'] + ' inHg')
        set_property('Current.Precipitation' , data['current_observation']['precip_1hr_in'] + ' in')
        set_property('Current.HeatIndex'     , str(data['current_observation']['heat_index_f']) + TEMPUNIT)
        set_property('Current.WindChill'     , str(data['current_observation']['windchill_f']) + TEMPUNIT)
    else:
        set_property('Current.Pressure'      , data['current_observation']['pressure_mb'] + ' mb')
        set_property('Current.Precipitation' , data['current_observation']['precip_1hr_metric'] + ' mm')
        set_property('Current.HeatIndex'     , str(data['current_observation']['heat_index_c']) + TEMPUNIT)
        set_property('Current.WindChill'     , str(data['current_observation']['windchill_c']) + TEMPUNIT)
    if SPEEDUNIT == 'mph':
        set_property('Current.Visibility'    , data['current_observation']['visibility_mi'] + ' mi')
        set_property('Current.WindGust'      , str(data['current_observation']['wind_gust_mph']) + ' ' + SPEEDUNIT)
    elif SPEEDUNIT == 'Beaufort':
        set_property('Current.Visibility'    , data['current_observation']['visibility_km'] + ' km')
        set_property('Current.WindGust'      , KPHTOBFT(data['current_observation']['wind_gust_kph']))
    else:
        set_property('Current.Visibility'    , data['current_observation']['visibility_km'] + ' km')
        set_property('Current.WindGust'      , str(data['current_observation']['wind_gust_kph']) + ' ' + SPEEDUNIT)
# today properties
    set_property('Today.IsFetched'                     , 'true')
    if TIMEFORMAT != '/':
        AM = unicode(TIMEFORMAT.split('/')[0],encoding='utf-8')
        PM = unicode(TIMEFORMAT.split('/')[1],encoding='utf-8')
        hour = int(data['moon_phase']['sunrise']['hour']) % 24
        isam = (hour >= 0) and (hour < 12)
        if isam:
            hour = ('12' if (hour == 0) else '%02d' % (hour))
            set_property('Today.Sunrise'               , hour.lstrip('0') + ':' + data['moon_phase']['sunrise']['minute'] + ' ' + AM)
        else:
            hour = ('12' if (hour == 12) else '%02d' % (hour-12))
            set_property('Today.Sunrise'               , hour.lstrip('0') + ':' + data['moon_phase']['sunrise']['minute'] + ' ' + PM)
        hour = int(data['moon_phase']['sunset']['hour']) % 24
        isam = (hour >= 0) and (hour < 12)
        if isam:
            hour = ('12' if (hour == 0) else '%02d' % (hour))
            set_property('Today.Sunset'               , hour.lstrip('0') + ':' + data['moon_phase']['sunset']['minute'] + ' ' + AM)
        else:
            hour = ('12' if (hour == 12) else '%02d' % (hour-12))
            set_property('Today.Sunset'               , hour.lstrip('0') + ':' + data['moon_phase']['sunset']['minute'] + ' ' + PM)
    else:
        set_property('Today.Sunrise'                   , data['moon_phase']['sunrise']['hour'] + ':' + data['moon_phase']['sunrise']['minute'])
        set_property('Today.Sunset'                    , data['moon_phase']['sunset']['hour'] + ':' + data['moon_phase']['sunset']['minute'])
    set_property('Today.moonphase'                     , MOONPHASE(int(data['moon_phase']['ageOfMoon']), int(data['moon_phase']['percentIlluminated'])))
    if 'F' in TEMPUNIT:
        set_property('Today.AvgHighTemperature'        , data['almanac']['temp_high']['normal']['F'] + TEMPUNIT)
        set_property('Today.AvgLowTemperature'         , data['almanac']['temp_low']['normal']['F'] + TEMPUNIT)
        try:
            set_property('Today.RecordHighTemperature' , data['almanac']['temp_high']['record']['F'] + TEMPUNIT)
            set_property('Today.RecordLowTemperature'  , data['almanac']['temp_low']['record']['F'] + TEMPUNIT)
        except:
            set_property('Today.RecordHighTemperature' , '')
            set_property('Today.RecordLowTemperature'  , '')
    else:
        set_property('Today.AvgHighTemperature'        , data['almanac']['temp_high']['normal']['C'] + TEMPUNIT)
        set_property('Today.AvgLowTemperature'         , data['almanac']['temp_low']['normal']['C'] + TEMPUNIT)
        try:
            set_property('Today.RecordHighTemperature' , data['almanac']['temp_high']['record']['C'] + TEMPUNIT)
            set_property('Today.RecordLowTemperature'  , data['almanac']['temp_low']['record']['C'] + TEMPUNIT)
        except:
            set_property('Today.RecordHighTemperature' , '')
            set_property('Today.RecordLowTemperature'  , '')
    try:
        set_property('Today.RecordHighYear'            , data['almanac']['temp_high']['recordyear'])
        set_property('Today.RecordLowYear'             , data['almanac']['temp_low']['recordyear'])
    except:
        set_property('Today.RecordHighYear'            , '')
        set_property('Today.RecordLowYear'             , '')
# daily properties
    set_property('Daily.IsFetched', 'true')
    for count, item in enumerate(data['forecast']['simpleforecast']['forecastday']):
        weathercode = WEATHER_CODES[item['icon_url'][31:-4]]
        set_property('Daily.%i.LongDay'              % (count+1), item['date']['weekday'])
        set_property('Daily.%i.ShortDay'             % (count+1), item['date']['weekday_short'])
        if DATEFORMAT[1] == 'm':
            set_property('Daily.%i.LongDate'         % (count+1), item['date']['monthname'] + ' ' + str(item['date']['day']))
            set_property('Daily.%i.ShortDate'        % (count+1), MONTH[item['date']['month']] + ' ' + str(item['date']['day']))
        else:
            set_property('Daily.%i.LongDate'         % (count+1), str(item['date']['day']) + ' ' + item['date']['monthname'])
            set_property('Daily.%i.ShortDate'        % (count+1), str(item['date']['day']) + ' ' + MONTH[item['date']['month']])
        set_property('Daily.%i.Outlook'              % (count+1), item['conditions'])
        set_property('Daily.%i.OutlookIcon'          % (count+1), WEATHER_ICON % weathercode)
        set_property('Daily.%i.FanartCode'           % (count+1), weathercode)
        if SPEEDUNIT == 'mph':
            set_property('Daily.%i.WindSpeed'        % (count+1), str(item['avewind']['mph']) + ' ' + SPEEDUNIT)
            set_property('Daily.%i.MaxWind'          % (count+1), str(item['maxwind']['mph']) + ' ' + SPEEDUNIT)
        elif SPEEDUNIT == 'Beaufort':
            set_property('Daily.%i.WindSpeed'        % (count+1), KPHTOBFT(item['avewind']['kph']))
            set_property('Daily.%i.MaxWind'          % (count+1), KPHTOBFT(item['maxwind']['kph']))
        else:
            set_property('Daily.%i.WindSpeed'        % (count+1), str(item['avewind']['kph']) + ' ' + SPEEDUNIT)
            set_property('Daily.%i.MaxWind'          % (count+1), str(item['maxwind']['kph']) + ' ' + SPEEDUNIT)
        set_property('Daily.%i.WindDirection'        % (count+1), item['avewind']['dir'])
        set_property('Daily.%i.ShortWindDirection'   % (count+1), item['avewind']['dir'])
        set_property('Daily.%i.WindDegree'           % (count+1), str(item['avewind']['degrees']) + u'°')
        set_property('Daily.%i.Humidity'             % (count+1), str(item['avehumidity']) + '%')
        set_property('Daily.%i.MinHumidity'          % (count+1), str(item['minhumidity']) + '%')
        set_property('Daily.%i.MaxHumidity'          % (count+1), str(item['maxhumidity']) + '%')
        if 'F' in TEMPUNIT:
            set_property('Daily.%i.HighTemperature'  % (count+1), str(item['high']['fahrenheit']) + TEMPUNIT)
            set_property('Daily.%i.LowTemperature'   % (count+1), str(item['low']['fahrenheit']) + TEMPUNIT)
            set_property('Daily.%i.LongOutlookDay'   % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count]['fcttext'])
            set_property('Daily.%i.LongOutlookNight' % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count+1]['fcttext'])
            set_property('Daily.%i.Precipitation'    % (count+1), str(item['qpf_day']['in']) + ' in')
            set_property('Daily.%i.Snow'             % (count+1), str(item['snow_day']['in']) + ' in')
        else:
            set_property('Daily.%i.HighTemperature'  % (count+1), str(item['high']['celsius']) + TEMPUNIT)
            set_property('Daily.%i.LowTemperature'   % (count+1), str(item['low']['celsius']) + TEMPUNIT)
            set_property('Daily.%i.LongOutlookDay'   % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count]['fcttext_metric'])
            set_property('Daily.%i.LongOutlookNight' % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count+1]['fcttext_metric'])
            set_property('Daily.%i.Precipitation'    % (count+1), str(item['qpf_day']['mm']) + ' mm')
            set_property('Daily.%i.Snow'             % (count+1), str(item['snow_day']['cm']) + ' mm')
        set_property('Daily.%i.ChancePrecipitation'  % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count]['pop'] + '%')
# weekend properties
    set_property('Weekend.IsFetched', 'true')
    if __addon__.getSetting('Weekend') == '2':
        weekend = [4,5]
    elif __addon__.getSetting('Weekend') == '1':
        weekend = [5,6]
    else:
        weekend = [6,7]
    count = 0
    for item in data['forecast']['simpleforecast']['forecastday']:
        if date(item['date']['year'], item['date']['month'], item['date']['day']).isoweekday() in weekend:
            weathercode = WEATHER_CODES[item['icon_url'][31:-4]]
            set_property('Weekend.%i.LongDay'                  % (count+1), item['date']['weekday'])
            set_property('Weekend.%i.ShortDay'                 % (count+1), item['date']['weekday_short'])
            if DATEFORMAT[1] == 'm':
                set_property('Weekend.%i.LongDate'             % (count+1), item['date']['monthname'] + ' ' + str(item['date']['day']))
                set_property('Weekend.%i.ShortDate'            % (count+1), MONTH[item['date']['month']] + ' ' + str(item['date']['day']))
            else:
                set_property('Weekend.%i.LongDate'             % (count+1), str(item['date']['day']) + ' ' + item['date']['monthname'])
                set_property('Weekend.%i.ShortDate'            % (count+1), str(item['date']['day']) + ' ' + MONTH[item['date']['month']])
            set_property('Weekend.%i.Outlook'                  % (count+1), item['conditions'])
            set_property('Weekend.%i.OutlookIcon'              % (count+1), WEATHER_ICON % weathercode)
            set_property('Weekend.%i.FanartCode'               % (count+1), weathercode)
            if SPEEDUNIT == 'mph':
                set_property('Weekend.%i.WindSpeed'            % (count+1), str(item['avewind']['mph']) + ' ' + SPEEDUNIT)
                set_property('Weekend.%i.MaxWind'              % (count+1), str(item['maxwind']['mph']) + ' ' + SPEEDUNIT)
            elif SPEEDUNIT == 'Beaufort':
                set_property('Weekend.%i.WindSpeed'            % (count+1), KPHTOBFT(item['avewind']['kph']))
                set_property('Weekend.%i.MaxWind'              % (count+1), KPHTOBFT(item['maxwind']['kph']))
            else:
                set_property('Weekend.%i.WindSpeed'            % (count+1), str(item['avewind']['kph']) + ' ' + SPEEDUNIT)
                set_property('Weekend.%i.MaxWind'              % (count+1), str(item['maxwind']['kph']) + ' ' + SPEEDUNIT)
            set_property('Weekend.%i.WindDirection'            % (count+1), item['avewind']['dir'])
            set_property('Weekend.%i.ShortWindDirection'       % (count+1), item['avewind']['dir'])
            set_property('Weekend.%i.WindDegree'               % (count+1), str(item['avewind']['degrees']) + u'°')
            set_property('Weekend.%i.Humidity'                 % (count+1), str(item['avehumidity']) + '%')
            set_property('Weekend.%i.MinHumidity'              % (count+1), str(item['minhumidity']) + '%')
            set_property('Weekend.%i.MaxHumidity'              % (count+1), str(item['maxhumidity']) + '%')
            set_property('Weekend.%i.ChancePrecipitation'      % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count]['pop'] + '%')
            if 'F' in TEMPUNIT:
                set_property('Weekend.%i.HighTemperature'      % (count+1), str(item['high']['fahrenheit']) + TEMPUNIT)
                set_property('Weekend.%i.LowTemperature'       % (count+1), str(item['low']['fahrenheit']) + TEMPUNIT)
                set_property('Weekend.%i.Precipitation'        % (count+1), str(item['qpf_day']['in']) + ' in')
                set_property('Weekend.%i.Snow'                 % (count+1), str(item['snow_day']['in']) + ' in')
                set_property('Weekend.%i.LongOutlookDay'       % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count]['fcttext'])
                set_property('Weekend.%i.LongOutlookNight'     % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count+1]['fcttext'])
            else:
                set_property('Weekend.%i.HighTemperature'      % (count+1), str(item['high']['celsius']) + TEMPUNIT)
                set_property('Weekend.%i.LowTemperature'       % (count+1), str(item['low']['celsius']) + TEMPUNIT)
                set_property('Weekend.%i.Precipitation'        % (count+1), str(item['qpf_day']['mm']) + ' mm')
                set_property('Weekend.%i.Snow'                 % (count+1), str(item['snow_day']['cm']) + ' mm')
                if data['current_observation']['display_location']['country'] == 'UK': # for the brits
                    dfcast_e = data['forecast']['txt_forecast']['forecastday'][2*count]['fcttext'].split('.')
                    dfcast_m = data['forecast']['txt_forecast']['forecastday'][2*count]['fcttext_metric'].split('.')
                    nfcast_e = data['forecast']['txt_forecast']['forecastday'][2*count+1]['fcttext'].split('.')
                    nfcast_m = data['forecast']['txt_forecast']['forecastday'][2*count+1]['fcttext_metric'].split('.')
                    for field in dfcast_e:
                        if field.endswith('mph'): # find windspeed in mph
                            wind = field
                            break
                    for field in dfcast_m:
                        if field.endswith('km/h'): # find windspeed in km/h
                            dfcast_m[dfcast_m.index(field)] = wind # replace windspeed in km/h with windspeed in mph
                            break
                    for field in nfcast_e:
                        if field.endswith('mph'): # find windspeed in mph
                            wind = field
                            break
                    for field in nfcast_m:
                        if field.endswith('km/h'): # find windspeed in km/h
                            nfcast_m[nfcast_m.index(field)] = wind # replace windspeed in km/h with windspeed in mph
                            break
                    set_property('Weekend.%i.LongOutlookDay'   % (count+1), '. '.join(dfcast_m))
                    set_property('Weekend.%i.LongOutlookNight' % (count+1), '. '.join(nfcast_m))
                else:
                    set_property('Weekend.%i.LongOutlookDay'   % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count]['fcttext_metric'])
                    set_property('Weekend.%i.LongOutlookNight' % (count+1), data['forecast']['txt_forecast']['forecastday'][2*count+1]['fcttext_metric'])
            count += 1
            if count == 2:
                break
# 36 hour properties
    set_property('36Hour.IsFetched', 'true')
    for count, item in enumerate(data['forecast']['txt_forecast']['forecastday']):
        weathercode = WEATHER_CODES[item['icon_url'][31:-4]]
        if 'F' in TEMPUNIT:
            try:
                fcast = item['fcttext'].split('.')
                for line in fcast:
                    if line.endswith('F'):
                        set_property('36Hour.%i.TemperatureHeading' % (count+1), line.rsplit(' ',1)[0])
                        set_property('36Hour.%i.Temperature'        % (count+1), line.rsplit(' ',1)[1].rstrip('F').strip() + TEMPUNIT)
                        break
            except:
                set_property('36Hour.%i.TemperatureHeading'         % (count+1), '')
                set_property('36Hour.%i.Temperature'                % (count+1), '')
            set_property('36Hour.%i.Forecast'                       % (count+1), item['fcttext'])
        else:
            try:
                fcast = item['fcttext_metric'].split('.')
                for line in fcast:
                    if line.endswith('C'):
                        set_property('36Hour.%i.TemperatureHeading' % (count+1), line.rsplit(' ',1)[0])
                        set_property('36Hour.%i.Temperature'        % (count+1), line.rsplit(' ',1)[1].rstrip('C').strip() + TEMPUNIT)
                        break
            except:
                set_property('36Hour.%i.TemperatureHeading' % (count+1), '')
                set_property('36Hour.%i.Temperature'        % (count+1), '')
            if data['current_observation']['display_location']['country'] == 'UK': # for the brits
                fcast_e = item['fcttext'].split('.')
                for field in fcast_e:
                    if field.endswith('mph'): # find windspeed in mph
                        wind = field
                        break
                for field in fcast:
                    if field.endswith('km/h'): # find windspeed in km/h
                        fcast[fcast.index(field)] = wind # replace windspeed in km/h with windspeed in mph
                        break
                set_property('36Hour.%i.Forecast'                   % (count+1), '. '.join(fcast))
            else:
                set_property('36Hour.%i.Forecast'                   % (count+1), item['fcttext_metric'])
        set_property('36Hour.%i.Heading'                    % (count+1), item['title'])
        set_property('36Hour.%i.ChancePrecipitation'        % (count+1), item['pop']  + '%')
        set_property('36Hour.%i.OutlookIcon'                % (count+1), WEATHER_ICON % weathercode)
        set_property('36Hour.%i.FanartCode'                 % (count+1), weathercode)
        if count == 2:
            break
# hourly properties
    set_property('Hourly.IsFetched', 'true')
    for count, item in enumerate(data['hourly_forecast']):
        weathercode = WEATHER_CODES[item['icon_url'][31:-4]]
        if TIMEFORMAT != '/':
            set_property('Hourly.%i.Time'            % (count+1), item['FCTTIME']['civil'])
        else:
            set_property('Hourly.%i.Time'            % (count+1), item['FCTTIME']['hour_padded'] + ':' + item['FCTTIME']['min'])
        if DATEFORMAT[1] == 'm':
            set_property('Hourly.%i.ShortDate'       % (count+1), item['FCTTIME']['month_name_abbrev'] + ' ' + item['FCTTIME']['mday_padded'])
            set_property('Hourly.%i.LongDate'        % (count+1), item['FCTTIME']['month_name'] + ' ' + item['FCTTIME']['mday_padded'])
        else:
            set_property('Hourly.%i.ShortDate'       % (count+1), item['FCTTIME']['mday_padded'] + ' ' + item['FCTTIME']['month_name_abbrev'])
            set_property('Hourly.%i.LongDate'        % (count+1), item['FCTTIME']['mday_padded'] + ' ' + item['FCTTIME']['month_name'])
        if 'F' in TEMPUNIT:
            set_property('Hourly.%i.Temperature'     % (count+1), item['temp']['english'] + TEMPUNIT)
            set_property('Hourly.%i.DewPoint'        % (count+1), item['dewpoint']['english'] + TEMPUNIT)
            set_property('Hourly.%i.FeelsLike'       % (count+1), item['feelslike']['english'] + TEMPUNIT)
            set_property('Hourly.%i.Precipitation'   % (count+1), item['qpf']['english'] + ' in')
            set_property('Hourly.%i.Snow'            % (count+1), item['snow']['english'] + ' in')
            set_property('Hourly.%i.HeatIndex'       % (count+1), item['heatindex']['english'] + TEMPUNIT)
            set_property('Hourly.%i.WindChill'       % (count+1), item['windchill']['english'] + TEMPUNIT)
            set_property('Hourly.%i.Mslp'            % (count+1), item['mslp']['english'] + ' inHg')
        else:
            set_property('Hourly.%i.Temperature'     % (count+1), item['temp']['metric'] + TEMPUNIT)
            set_property('Hourly.%i.DewPoint'        % (count+1), item['dewpoint']['metric'] + TEMPUNIT)
            set_property('Hourly.%i.FeelsLike'       % (count+1), item['feelslike']['metric'] + TEMPUNIT)
            set_property('Hourly.%i.Precipitation'   % (count+1), item['qpf']['metric'] + ' mm')
            set_property('Hourly.%i.Snow'            % (count+1), item['snow']['metric'] + ' mm')
            set_property('Hourly.%i.HeatIndex'       % (count+1), item['heatindex']['metric'] + TEMPUNIT)
            set_property('Hourly.%i.WindChill'       % (count+1), item['windchill']['metric'] + TEMPUNIT)
            set_property('Hourly.%i.Mslp'            % (count+1), item['mslp']['metric'] + ' inHg')
        if SPEEDUNIT == 'mph':
            set_property('Hourly.%i.WindSpeed'       % (count+1), item['wspd']['english'] + ' ' + SPEEDUNIT)
        elif SPEEDUNIT == 'Beaufort':
            set_property('Hourly.%i.WindSpeed'       % (count+1), KPHTOBFT(int(item['wspd']['metric'])))
        else:
            set_property('Hourly.%i.WindSpeed'       % (count+1), item['wspd']['metric'] + ' ' + SPEEDUNIT)
        set_property('Hourly.%i.WindDirection'       % (count+1), item['wdir']['dir'])
        set_property('Hourly.%i.ShortWindDirection'  % (count+1), item['wdir']['dir'])
        set_property('Hourly.%i.WindDegree'          % (count+1), item['wdir']['degrees'] + u'°')
        set_property('Hourly.%i.Humidity'            % (count+1), item['humidity'] + '%')
        set_property('Hourly.%i.UVIndex'             % (count+1), item['uvi'])
        set_property('Hourly.%i.ChancePrecipitation' % (count+1), item['pop'] + '%')
        set_property('Hourly.%i.Outlook'             % (count+1), item['condition'])
        set_property('Hourly.%i.OutlookIcon'         % (count+1), WEATHER_ICON % weathercode)
        set_property('Hourly.%i.FanartCode'          % (count+1), weathercode)
        if count == 35: # workaround: wunderground provides a 10day hourly forecast
            break
# alert properties
    set_property('Alerts.IsFetched', 'true')
    if str(data['alerts']) != '[]':
        rss = ''
        alerts = ''
        for count, item in enumerate(data['alerts']):
            set_property('Alerts.%i.Description'     % (count+1), item['description'])
            set_property('Alerts.%i.Message'         % (count+1), item['message'])
            set_property('Alerts.%i.StartDate'       % (count+1), item['date'])
            set_property('Alerts.%i.EndDate'         % (count+1), item['expires'])
            set_property('Alerts.%i.Significance'    % (count+1), SEVERITY[item['significance']])
            rss    = rss + item['description'] + ' - '
            alerts = alerts + item['message']
        set_property('Alerts.RSS'   , rss.rstrip(' - '))
        set_property('Alerts'       , alerts)
        set_property('Alerts.Count' , str(count+1))
    else:
        set_property('Alerts.RSS'   , '')
        set_property('Alerts'       , '')
        set_property('Alerts.Count' , '0')
# map properties
    set_property('Map.IsFetched', 'true')
    filelist = []
    locid = base64.b16encode(loc)
    addondir = os.path.join(__cwd__, 'resources', 'logo')
    mapdir = xbmc.translatePath('special://profile/addon_data/%s/map' % __addonid__)
    set_property('MapPath', addondir)
    if not xbmcvfs.exists(mapdir):
        xbmcvfs.mkdir(mapdir)
    json_query = xbmc.executeJSONRPC('{ "jsonrpc" : "2.0" , "method" : "Files.GetDirectory" , "params" : { "directory" : "%s" , "sort" : { "method" : "file" } } , "id" : 1 }' % mapdir.replace('\\', '\\\\'))
    json_query = unicode(json_query, 'utf-8', errors='ignore')
    json_response = simplejson.loads(json_query)
    if (json_response['result'] != None) and (json_response['result'].has_key('files')) and (json_response['result']['files'] != None):
        for item in json_response['result']['files']:
            if item['filetype'] == 'file':
                filelist.append(item['file'])
    animate = __addon__.getSetting('Animate')
    for item in filelist:
        if animate == 'true':
            if (time.time() - os.path.getmtime(item) > 14400) or (not locid in item):
                xbmcvfs.delete(item)
        else:
            xbmcvfs.delete(item)
    zoom = __addon__.getSetting('Zoom')
    if zoom == '10': # default setting does not return decimals, changed setting will
        zoom = '10.0'
    url = data['satellite']['image_url_ir4'].replace('width=300&height=300','width=640&height=360').replace('radius=75','radius=%i' % int(1000/int(zoom.rstrip('0').rstrip('.,'))))
    log('map url: %s' % url)
    try:
        req = urllib2.urlopen(url)
        response = req.read()
        req.close()
        log('satellite image downloaded')
    except:
        response = ''
        log('satellite image downloaded failed')
    if response != '':
        timestamp = time.strftime('%Y%m%d%H%M%S')
        mapfile = xbmc.translatePath('special://profile/addon_data/%s/map/%s-%s.png' % (__addonid__,locid,timestamp)).decode("utf-8")
        try:
            tmpmap = open(mapfile, 'wb')
            tmpmap.write(response)
            tmpmap.close()
            set_property('MapPath', mapdir)
        except:
            log('failed to save satellite image')

log('version %s started: %s' % (__version__, sys.argv))
log('lang: %s'    % LANGUAGE)
log('speed: %s'   % SPEEDUNIT)
log('temp: %s'    % TEMPUNIT[1])
log('time: %s'    % TIMEFORMAT)
log('date: %s'    % DATEFORMAT)

if sys.argv[1].startswith('Location'):
    keyboard = xbmc.Keyboard('', xbmc.getLocalizedString(14024), False)
    keyboard.doModal()
    if (keyboard.isConfirmed() and keyboard.getText() != ''):
        text = keyboard.getText()
        locations, locationids = location(text)
        dialog = xbmcgui.Dialog()
        if locations != []:
            selected = dialog.select(xbmc.getLocalizedString(396), locations)
            if selected != -1:
                __addon__.setSetting(sys.argv[1], locations[selected])
                __addon__.setSetting(sys.argv[1] + 'id', locationids[selected])
                log('selected location: %s' % locations[selected])
                log('selected location id: %s' % locationids[selected])
        else:
            dialog.ok(__addonname__, xbmc.getLocalizedString(284))

else:
    location = __addon__.getSetting('Location%sid' % sys.argv[1])
    if (location == '') and (sys.argv[1] != '1'):
        location = __addon__.getSetting('Location1id')
        log('trying location 1 instead')
    if location == '':
        log('fallback to geoip')
        location = geoip()
    if not location == '':
        if location.startswith('/q/'): # backwards compatibility
            location = location[3:]
        forecast(location)
    else:
        log('no location found')
        clear()
    refresh_locations()

set_property('WeatherProvider', __addonname__)
log('finished')
