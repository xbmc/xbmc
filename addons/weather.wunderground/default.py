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
import xbmcgui, xbmcaddon

__addon__      = xbmcaddon.Addon()
__provider__   = __addon__.getAddonInfo('name')
__cwd__        = __addon__.getAddonInfo('path')
__resource__   = xbmc.translatePath(os.path.join(__cwd__, 'resources', 'lib'))

sys.path.append (__resource__)

from utilities import *

LOCATION_URL    = 'http://autocomplete.wunderground.com/aq?query=%s&format=JSON'
WEATHER_URL     = 'http://api.wunderground.com/api/%s/conditions/forecast7day/hourly%s.json'
A_I_K           = 'NDEzNjBkMjFkZjFhMzczNg=='
WEATHER_WINDOW  = xbmcgui.Window(12600)

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

def fetch(url):
    try:
        req = urllib2.urlopen(url)
        json_string = req.read()
        req.close()
    except:
        json_string = ''
    try:
        parsed_json = simplejson.loads(json_string)
    except:
        parsed_json = ''
    return parsed_json

def location(string):
    loc   = []
    locid = []
    query = fetch(LOCATION_URL % (urllib2.quote(string)))
    for item in query['RESULTS']:
        location   = item['name']
        locationid = item['l']
        loc.append(location)
        locid.append(locationid)
    return loc, locid

def forecast(city):
    data = fetch(WEATHER_URL % (aik[::-1], city))
    if data != '':
        properties(data)

def properties(query):
    weathercode = WEATHER_CODES[query['current_observation']['icon_url'][31:-4]]
    set_property('Current.Condition'     , query['current_observation']['weather'])
    set_property('Current.Temperature'   , str(query['current_observation']['temp_c']))
    set_property('Current.Wind'          , str(int(query['current_observation']['wind_mph'] * 1.609344)))
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
        if count == 3:
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
    location = __addon__.getSetting('Location%sid' % sys.argv[1])
    aik = base64.b64decode(A_I_K)
    forecast(location)

refresh_locations()
set_property('WeatherProvider', 'Weather Underground')
