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

import xbmcgui

"""
Usage Explained

xbmcgui.Dialog().browse(type,prompt,shares)
type can be
    0 for directory
    1  for files
    2 for images (eg it will show thumbnails if skin shows it)
    any other number will do directories

prompt can be text/string

shares can be
- files
- videos
- music
- programs
- pictures
"""

print xbmcgui.Dialog().browse(0,"a Directory","files")
print xbmcgui.Dialog().browse(1,"a file","files")
print xbmcgui.Dialog().browse(1,"a music file","music")
print xbmcgui.Dialog().browse(1,"a image","files")


