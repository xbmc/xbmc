#
#      Copyright (C) 2005-2008 Team XBMC
#      http://www.xbmc.org
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XBMC; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html
#

# RSS Feed example
# This one uses a existing window in xbmc which allows us to draw for example in
# the "home window"
# No use to capture action codes in here since they are not supported when using an
# existing window, same goes for buttons.

#get windows from key.h

WINDOW_HOME			= 10000
WINDOW_PROGRAMS		= 10001
WINDOW_PICTURES		= 10002
WINDOW_FILES		= 10003

RSS_URL = 'http://www.xbox-scene.com/xbox1data/xbox-scene.xml'

import xbmc
import xbmcgui
import sys, urllib, string, urlparse, re, time

class RssReader:
	def __init__(self):
		self.news = []
		
	def parsexml(self, line, tag):
		result= re.search('<' + tag + '>.*' + tag + '>', line, re.DOTALL)
		try:
			if result.group(0):
				mod = string.replace(result.group(0), '<' + tag + '>','')
				mod = string.replace(mod, '</' + tag + '>', '')
				mod = string.lstrip(mod)
				return mod
		except:
			return
	
	def getNews(self):
		data = urllib.urlopen(RSS_URL)
		items = string.split(data.read(), '<item>')
		
		for item in items:
			self.news.append(self.parsexml(item, 'title'))
			
		return self.news

class Window(xbmcgui.Window):
	def __init__(self, id):
		self.strTitle = xbmcgui.ControlLabel(50, 400, 100, 20, '', 'font14', '0xFFB2D4F5')
		self.labelsNews = []
		self.labelsNews.append(xbmcgui.ControlLabel(50, 420, 100, 20, '', 'font13', '0xFFFFFFFF'))
		self.labelsNews.append(xbmcgui.ControlLabel(50, 440, 100, 20, '', 'font13', '0xFFFFFFFF'))
		self.labelsNews.append(xbmcgui.ControlLabel(50, 460, 100, 20, '', 'font13', '0xFFFFFFFF'))
		self.labelsNews.append(xbmcgui.ControlLabel(50, 480, 100, 20, '', 'font13', '0xFFFFFFFF'))
		self.labelsNews.append(xbmcgui.ControlLabel(50, 500, 100, 20, '', 'font13', '0xFFFFFFFF'))
		
		self.addControl(self.strTitle)
		self.addControl(self.labelsNews[0])
		self.addControl(self.labelsNews[1])
		self.addControl(self.labelsNews[2])
		self.addControl(self.labelsNews[3])
		self.addControl(self.labelsNews[4])
		
		self.running = True
		self.rssreader = RssReader()
		
	def run(self):
		while self.running:
			self.update()
			#time.sleep(1800) # wait 30 min before updating again

	def update(self):
		self.news = self.rssreader.getNews()
		self.strTitle.setLabel(self.news[0])
		for i in range(1, 5):
			self.labelsNews[i].setLabel(self.news[i])

		self.running = False
		
w = Window(WINDOW_HOME)
w.show()
w.run()

# wait 60 sec before closing window
# loop forever if you never want to stop this script
for i in range(6):
	time.sleep(10)

del w
