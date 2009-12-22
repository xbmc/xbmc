"""
    Plugin for downloading scripts/plugins/skins from SVN repositoriess
"""

# main imports
import sys
import xbmc

# plugin constants
__plugin__ = "SVN Repo Installer"
__author__ = "nuka1195/BigBellyBilly"
__url__ = "http://code.google.com/p/xbmc-addons/"
__svn_url__ = "http://xbmc-addons.googlecode.com/svn/trunk/plugins/programs/SVN%20Repo%20Installer"
__credits__ = "Team XBMC"
__version__ = "1.8.2b"
__date__ = "$Date$"
__svn_revision__ = "$Revision$"
__XBMC_Revision__ = "19001"

def _check_compatible():
    try:
        # spam plugin statistics to log
        xbmc.log( "[PLUGIN] '%s: Version - %s-r%s' initialized!" % ( __plugin__, __version__, __svn_revision__.replace( "$", "" ).replace( "Revision", "" ).replace( ":", "" ).strip() ), xbmc.LOGNOTICE )
        # get xbmc revision
        xbmc_rev = int( xbmc.getInfoLabel( "System.BuildVersion" ).split( " r" )[ -1 ] )
        # compatible?
        ok = xbmc_rev >= int( __XBMC_Revision__ )
    except:
        # error, so unknown, allow to run
        xbmc_rev = 0
        ok = 2
    # spam revision info
    xbmc.log( "     ** Required XBMC Revision: r%s **" % ( __XBMC_Revision__, ), xbmc.LOGNOTICE )
    xbmc.log( "     ** Found XBMC Revision: r%d [%s] **" % ( xbmc_rev, ( "Not Compatible", "Compatible", "Unknown", )[ ok ], ), xbmc.LOGNOTICE )
    # if not compatible, inform user
    if ( not ok ):
        import xbmcgui
        xbmcgui.Dialog().ok( "%s - %s: %s" % ( __plugin__, xbmc.getLocalizedString( 30700 ), __version__, ), xbmc.getLocalizedString( 30701 ) % ( __plugin__, ), xbmc.getLocalizedString( 30702 ) % ( __XBMC_Revision__, ), xbmc.getLocalizedString( 30703 ) )
    #return result
    return ok


if ( __name__ == "__main__" ):
    if ( not sys.argv[ 2 ] ):
        # check for compatibility, only need to check this once, continue if ok
        if ( _check_compatible() ):
            from installerAPI import xbmcplugin_list as plugin
    elif ( "show_info=" in sys.argv[ 2 ] ):
        from installerAPI import xbmcplugin_info as plugin
    elif ( "delete=" in sys.argv[ 2 ] ):
        from installerAPI import xbmcplugin_actions as plugin
    elif ( "self_update" in sys.argv[ 2 ] ):
        from installerAPI import xbmcplugin_actions as plugin
    elif ( "download_url=" in sys.argv[ 2 ] ):
        from installerAPI import xbmcplugin_downloader as plugin
    elif ( sys.argv[ 2 ] == "?category='updates'" ):
        from installerAPI import xbmcplugin_update as plugin
    elif ( "showreadme=" in sys.argv[ 2 ] or "showlog=" in sys.argv[ 2 ] ):
        from installerAPI import xbmcplugin_logviewer as plugin
    else:
        from installerAPI import xbmcplugin_list as plugin

    try:
        plugin.Main()
    except:
        import traceback
        traceback.print_exc()
#        pass
