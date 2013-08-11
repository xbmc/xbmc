#
#      Copyright (C) 2005-2013 Team XBMC
#      http://xbmc.org
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
#  along with XBMC; see the file COPYING.  If not, see
#  <http://www.gnu.org/licenses/>.
#

import xbmc, xbmcgui, re, os

thumbDir = "Q:\\thumbs"
files = os.listdir(thumbDir)
test = re.compile("\.tbn$", re.IGNORECASE)
files = filter(test.search, files)

for file in files:
	srcPath = thumbDir + "\\" + file
	size = len(file)
	diff = 12 - size
	dest = ("0" * diff) + file.lower()
	subDir = dest[0]
	destPath = thumbDir + "\\" + subDir + "\\" + dest
	if not os.path.exists(destPath):
		os.rename(srcPath,destPath)
