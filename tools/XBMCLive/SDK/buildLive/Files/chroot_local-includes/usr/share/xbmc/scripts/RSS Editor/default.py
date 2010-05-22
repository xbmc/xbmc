# -*- coding: utf-8 -*-
#   Copyright (C) 2009-2010 Team XBMC
#   http://www.xbmc.org
#
#   This Program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This Program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with XBMC; see the file COPYING.  If not, write to
#   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#   http://www.gnu.org/copyleft/gpl.html

import sys
import os
import xbmc

# Script constants
__scriptname__ = "RSS Editor"
__author__ = "rwparris2"
__url__ = "http://code.google.com/p/xbmc-addons/"
__credits__ = "Team XBMC"
__version__ = "1.5.5"

print "[SCRIPT] '%s: version %s' initialized!" % (__scriptname__, __version__, )

if (__name__ == "__main__"):
    import resources.lib.rssEditor as rssEditor
    ui = rssEditor.GUI("script-RSS_Editor-rssEditor.xml", os.getcwd(), "default", setNum = 'set1')
    del ui

sys.modules.clear()
