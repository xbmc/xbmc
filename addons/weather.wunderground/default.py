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
# *

import os, sys, urllib2, base64, socket, simplejson
import xbmc, xbmcgui, xbmcaddon

__addon__      = xbmcaddon.Addon()
__provider__   = __addon__.getAddonInfo('name')
__cwd__        = __addon__.getAddonInfo('path')
__resource__   = xbmc.translatePath(os.path.join(__cwd__, 'resources', 'lib')).decode("utf-8")

sys.path.append (__resource__)

from utilities import *

LOCATION_URL    = 'http://autocomplete.wunderground.com/aq?query=%s&format=JSON'
WEATHER_URL     = 'http://api.wunderground.com/api/%s/conditions/forecast7day/hourly/q/%s.json'
GEOIP_URL       = 'http://api.wunderground.com/api/%s/geolookup/q/autoip.json'
GEOWIFI_URL     = 'http://www.google.com/loc/json'
REV_GEOCODE_URL = 'http://maps.googleapis.com/maps/api/geocode/json?latlng=%s,%s&sensor=false'
A_I_K           = 'NDEzNjBkMjFkZjFhMzczNg=='
WEATHER_WINDOW  = xbmcgui.Window(12600)
MAXDAYS         = 6

socket.setdefaulttimeout(10)

def set_property(name, value):
    WEATHER_WINDOW.setProperty(name, value)

def refresh_locations():
    location_set1 = __addon__.getSetting('Location1')
    location_set2 = __addon__.getSetting('Location2')
    location_set3 = __addon__.getSetting('Location3')
    locations = 0
    if location_set1 != '':
        locations += 1
        set_property('Location1', location_set1)
    else:
        set_property('Location1', '')
    if location_set2 != '':
        locations += 1 
        set_property('Location2', location_set2)
    else:
        set_property('Location2', '')
    if location_set3 != '':
        locations += 1
        set_property('Location3', location_set3)
    else:
        set_property('Location3', '')
    set_property('Locations', str(locations))

def fetch(url, post_data = None):
    try:
        req = urllib2.urlopen(url, post_data)
        json_string = req.read()
        req.close()
    except:
        json_string = ''
    try:
        json_clean = json_string.replace('"-9999.00"','""').replace('"-9998"','""').replace('"NA"','""')
        parsed_json = simplejson.loads(json_clean)
    except:
        parsed_json = ''
    return parsed_json

def location(string):
    loc   = []
    locid = []
    query = fetch(LOCATION_URL % (urllib2.quote(string)))
    if query != '':
        for item in query['RESULTS']:
            location   = item['name']
            locationid = item['l'][3:]
            loc.append(location)
            locid.append(locationid)
    return loc, locid

def geowifi():
    try:
        # This function has been added post-Eden
        aps = xbmc.getAccessPoints()
    except:
        return ''
    
    zip_code = ''
    
    wifi_towers = []
    for ap in aps:
        del ap['encryption'] # Not needed
        # Note, we expect the MAC address to be in the form %02x-%02x-%02x-%02x-%02x-%02x
        wifi_towers.append(simplejson.dumps(ap))
    post_data = '{"host":"xbmc.org","version":"1.1.0","request_address":true,"wifi_towers":[%s]}' % (','.join(wifi_towers))
    data = fetch(GEOWIFI_URL, post_data)
    
    try:
        if len(data['location']['address']['postal_code']) > 0:
            zip_code = data['location']['address']['postal_code']
        else:
            raise
    except:
        # Sometimes the query doesn't return a zip code. I have even observed this
        # inconsistent behavior across identical queries. In this case, if we have
        # a latitude & longitude to work with, do a reverse-geocode lookup
        try:
            data2 = fetch(REV_GEOCODE_URL % (data['location']['latitude'], data['location']['longitude']))
            for result in data2['results']:
                for address_component in result['address_components']:
                    for type in address_component['types']:
                        if type == 'postal_code':
                            zip_code = address_component['long_name']
                            break
                    # Because we can't triple-break, and three embeded try's are too ugly
                    if zip_code != '':
                        break
                if zip_code != '':
                    break
        except:
            return '' # Couldn't obtain a postal code
    
    # Now, use wunderground to turn the postal code into a valid id
    try:
        locations, locationids = location(zip_code)
        loc = locationids[0]
        __addon__.setSetting('Location1', locations[0])
        __addon__.setSetting('Location1id', loc)
    except:
        loc = ''
    return loc

def geoip():
    data = fetch(GEOIP_URL % aik[::-1])
    if data != '' and data.has_key('location'):
        location = data['location']['l'][3:]
        location_name = data['location']['city']
        # Add the state (if available) or country name to the city
        if data['location']['state']:
            location_name += ", " + data['location']['state']
        elif data['location']['country_name']:
            location_name += ", " + data['location']['country_name']
        __addon__.setSetting('Location1', location_name)
        __addon__.setSetting('Location1id', location)
    else:
        location = ''
    return location

def forecast(city):
    data = fetch(WEATHER_URL % (aik[::-1], city))
    if data != '':
        properties(data)

def properties(query):
    weathercode = WEATHER_CODES[query['current_observation']['icon_url'][31:-4]]
    set_property('Current.Condition'     , query['current_observation']['weather'])
    set_property('Current.Temperature'   , str(query['current_observation']['temp_c']))
    set_property('Current.Wind'          , str(query['current_observation']['wind_kph']))
    set_property('Current.WindDirection' , query['current_observation']['wind_dir'])
    set_property('Current.Humidity'      , query['current_observation']['relative_humidity'].rstrip('%'))
    set_property('Current.FeelsLike'     , str((int(query['hourly_forecast'][0]['feelslike']['english'])-32)*5/9))
    set_property('Current.UVIndex'       , query['hourly_forecast'][0]['uvi'])
    set_property('Current.DewPoint'      , str(query['current_observation']['dewpoint_c']))
    set_property('Current.OutlookIcon'   , '%s.png' % weathercode)
    set_property('Current.FanartCode'    , weathercode)
    for count, item in enumerate(query['forecast']['simpleforecast']['forecastday']):
        weathercode = WEATHER_CODES[item['icon_url'][31:-4]]
        day = DAYS[item['date']['weekday_short']]
        set_property('Day%i.Title'       % count, day)
        set_property('Day%i.HighTemp'    % count, str(item['high']['celsius']))
        set_property('Day%i.LowTemp'     % count, str(item['low']['celsius']))
        set_property('Day%i.Outlook'     % count, item['conditions'])
        set_property('Day%i.OutlookIcon' % count, '%s.png' % weathercode)
        set_property('Day%i.FanartCode'  % count, weathercode)
        if count == MAXDAYS:
            break

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
        else:
            dialog.ok(__provider__, xbmc.getLocalizedString(284))

else:
    location_id = __addon__.getSetting('Location%sid' % sys.argv[1])
    aik = base64.b64decode(A_I_K)
    if (location_id == '') and (sys.argv[1] != '1'):
        location_id = __addon__.getSetting('Location1id')
    if location_id == '':
        location_id = geowifi()
    #if location_id == '':
    #    location_id = geoip()
    if not location_id == '':
        if location_id.startswith('/q/'): # backwards compatibility
            location_id = location_id[3:]
        forecast(location_id)
    else:
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

refresh_locations()
set_property('WeatherProvider', 'Weather Underground')
