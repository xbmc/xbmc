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

# ftp download script for xbmc

adres = 'ftp.planet.nl'
remotedir = 'planet/games/quake/maps'
remotefile = 'apleasure.zip'
localfile = 'Q:\\apleasure1.zip'

import sys, socket, xbmc, xbmcgui
from ftplib import FTP

class writer:
	def __init__(self):
		self.size = 0
		self.copied = 0;
	def write(self, data):
		f.write(data)
		self.copied = self.copied + len(data)
		dialog.update((self.copied * 100)/ self.size)

pwriter = writer()

dialog = xbmcgui.DialogProgress()

dialog.create("connecting",adres)
ftp = FTP(adres) # connect to host, default port

dialog.close()
dialog.create("logging in","username = anonymous")
ftp.login() # default, i.e.: user anonymous, passwd user@hostname

dialog.close()
dialog.create("changing directory to", remotedir)
ftp.cwd(remotedir)
pwriter.size = ftp.size(remotefile)
f = open(localfile, "wb")

dialog.close()
dialog.create("downloading", remotefile)
ftp.retrbinary('RETR ' + remotefile, pwriter.write, 8192)

f.close()
dialog.close()
ftp.quit()
