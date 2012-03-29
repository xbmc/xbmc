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

import xbmcgui, xbmcaddon, simplejson, urllib2

SYSTEMINFO_WINDOW = xbmcgui.Window(10007)

# Fetch
try:
  req = urllib2.urlopen('http://freegeoip.net/json/')
  json_string = req.read()
  req.close()
  parsed_json = simplejson.loads(json_string)
  SYSTEMINFO_WINDOW.setProperty('GeoRegion',parsed_json['country_code']) 
except:
  pass
