# XBMC Lyrics script revision: 2346 - built with build.bat version 1.0 #

# main import's 
import sys
import os
import xbmcgui
import xbmc

# Script constants 
__scriptname__ = "XBMC Lyrics"
__author__ = "XBMC Lyrics Team"
__url__ = "http://code.google.com/p/xbmc-scripting/"
__svn_url__ = "http://xbmc-scripting.googlecode.com/svn/trunk/XBMC%20Lyrics"
__credits__ = "XBMC TEAM, freenode/#xbmc-scripting"
__version__ = "1.6.0"
__svn_revision__ = "2346"

# Shared resources 
BASE_RESOURCE_PATH = xbmc.translatePath( os.path.join( os.getcwd(), "resources" ) )
__language__ = xbmc.Language( os.getcwd() ).getLocalizedString

# Main team credits 
__credits_l1__ = __language__( 910 )#"Head Developer & Coder"
__credits_r1__ = "Nuka1195"
__credits_l2__ = __language__( 911 )#"Original author"
__credits_r2__ = "EnderW"
__credits_l3__ = __language__( 912 )#"Original skinning"
__credits_r3__ = "Smuto"

# additional credits 
__add_credits_l1__ = __language__( 1 )#"Xbox Media Center"
__add_credits_r1__ = "Team XBMC"
__add_credits_l2__ = __language__( 913 )#"Unicode support"
__add_credits_r2__ = "Spiff"
__add_credits_l3__ = __language__( 914 )#"Language file"
__add_credits_r3__ = __language__( 2 )#"Translators name"


# Start the main gui or settings gui 
if ( __name__ == "__main__" ):
    if ( xbmc.Player().isPlayingAudio() ):
        import resources.lib.gui as gui
        window = "main"
    else:
        import resources.lib.settings as gui
        window = "settings"
    ui = gui.GUI( "script-%s-%s.xml" % ( __scriptname__.replace( " ", "_" ), window, ), os.getcwd(), "Default" )
    ui.doModal()
    del ui
    sys.modules.clear()
