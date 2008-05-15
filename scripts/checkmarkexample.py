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

def debug( str ):
    xbmc.output( str )

class Window( xbmcgui.Window ):
    def __init__( self ):
        xbmcgui.Window.__init__( self )
        self.checkMark = None

    def create( self ):
        self.checkMark = xbmcgui.ControlCheckMark(
                x=50, y=200,
                width=200, height=20,
                label="This is my new check mark",
                checkWidth=30, checkHeight=30,
                alignment=0 )
        self.addControl( self.checkMark )
        self.setFocus( self.checkMark )

    def onAction( self, action ):
        debug( "> Window.onAction( action=[%s] )"%action )
        if action in (9,10):
            self.close()
        debug( "< Window.onAction( action=[%s] )"%action )

    def onControl( self, control ):
        debug( "> Window.onControl( control=[%s] )"%control )
        if control == self.checkMark:
            b = self.checkMark.getSelected()
            if b:
                xbmcgui.Dialog().ok( "info", "check mark is selected" )
            else:
                xbmcgui.Dialog().ok( "info", "check mark is NOT selected" )
        debug( "< Window.onControl( control=[%s] )"%control )

win = Window()
win.create()
win.doModal()
del win

