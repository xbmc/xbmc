"""
Catchall module for shared functions and constants

Nuka1195
"""

import sys
import os
import xbmc
import xbmcgui

DEBUG_MODE = 0

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__
__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__

# comapatble versions
SETTINGS_VERSIONS = ( "1.6.0", )
# base paths
BASE_DATA_PATH = xbmc.translatePath( os.path.join( "P:\\script_data", os.path.basename( os.getcwd() ) ) )
BASE_SETTINGS_PATH = xbmc.translatePath( os.path.join( "P:\\script_data", os.path.basename( os.getcwd() ), "settings.txt" ) )
BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
# special button codes
SELECT_ITEM = ( 11, 256, 61453, )
EXIT_SCRIPT = ( 247, 275, 61467, )
CANCEL_DIALOG = EXIT_SCRIPT + ( 216, 257, 61448, )
GET_EXCEPTION = ( 216, 260, 61448, )
SETTINGS_MENU = ( 229, 259, 261, 61533, )
SHOW_CREDITS = ( 195, 274, 61507, )
MOVEMENT_UP = ( 166, 270, 61478, )
MOVEMENT_DOWN = ( 167, 271, 61480, )
# special action codes
ACTION_SELECT_ITEM = ( 7, )
ACTION_EXIT_SCRIPT = ( 10, )
ACTION_CANCEL_DIALOG = ACTION_EXIT_SCRIPT + ( 9, )
ACTION_GET_EXCEPTION = ( 0, 11 )
ACTION_SETTINGS_MENU = ( 117, )
ACTION_SHOW_CREDITS = ( 122, )
ACTION_MOVEMENT_UP = ( 3, )
ACTION_MOVEMENT_DOWN = ( 4, )
# Log status codes
LOG_INFO, LOG_ERROR, LOG_NOTICE, LOG_DEBUG = range( 1, 5 )

def _create_base_paths():
    """ creates the base folders """
    if ( not os.path.isdir( BASE_DATA_PATH ) ):
        os.makedirs( BASE_DATA_PATH )
_create_base_paths()

def get_keyboard( default="", heading="", hidden=False ):
    """ shows a keyboard and returns a value """
    keyboard = xbmc.Keyboard( default, heading, hidden )
    keyboard.doModal()
    if ( keyboard.isConfirmed() ):
        return keyboard.getText()
    return default

def get_numeric_dialog( default="", heading="", dlg_type=3 ):
    """ shows a numeric dialog and returns a value
        - 0 : ShowAndGetNumber		(default format: #)
        - 1 : ShowAndGetDate			(default format: DD/MM/YYYY)
        - 2 : ShowAndGetTime			(default format: HH:MM)
        - 3 : ShowAndGetIPAddress	(default format: #.#.#.#)
    """
    dialog = xbmcgui.Dialog()
    value = dialog.numeric( type, heading, default )
    return value

def get_browse_dialog( default="", heading="", dlg_type=1, shares="files", mask="", use_thumbs=False, treat_as_folder=False ):
    """ shows a browse dialog and returns a value
        - 0 : ShowAndGetDirectory
        - 1 : ShowAndGetFile
        - 2 : ShowAndGetImage
        - 3 : ShowAndGetWriteableDirectory
    """
    dialog = xbmcgui.Dialog()
    value = dialog.browse( dlg_type, heading, shares, mask, use_thumbs, treat_as_folder, default )
    return value

def LOG( status, format, *args ):
    if ( DEBUG_MODE >= status ):
        xbmc.output( "%s: %s\n" % ( ( "INFO", "ERROR", "NOTICE", "DEBUG", )[ status - 1 ], format % args, ) )

def show_credits():
    """ shows a credit window """
    import resources.lib.credits as credits
    c = credits.GUI( "script-%s-credits.xml" % ( __scriptname__.replace( " ", "_" ), ), os.getcwd(), "Default" )
    c.doModal()
    del c

def make_legal_filepath( path, compatible=False, extension=True ):
    environment = os.environ.get( "OS", "xbox" )
    path = path.replace( "\\", "/" )
    drive = os.path.splitdrive( path )[ 0 ]
    parts = os.path.splitdrive( path )[ 1 ].split( "/" )
    if ( not drive and parts[ 0 ].endswith( ":" ) and len( parts[ 0 ] ) == 2 and compatible ):
        drive = parts[ 0 ]
        parts[ 0 ] = ""
    if ( environment == "xbox" or environment == "win32" or compatible ):
        illegal_characters = """,*=|<>?;:"+"""
        for count, part in enumerate( parts ):
            tmp_name = ""
            for char in part:
                if ( char in illegal_characters ): char = ""
                tmp_name += char
            if ( environment == "xbox" or compatible ):
                if ( len( tmp_name ) > 42 ):
                    if ( count == len( parts ) - 1 and extension == True ):
                        ext = os.path.splitext( tmp_name )[ 1 ]
                        tmp_name = "%s%s" % ( os.path.splitext( tmp_name )[ 0 ][ : 42 - len( ext ) ].strip(), ext, )
                    else:
                        tmp_name = tmp_name[ : 42 ].strip()
            parts[ count ] = tmp_name
    filepath = drive + "/".join( parts )
    if ( environment == "win32" ):
        return filepath.encode( "utf-8" )
    else:
        return filepath


class Settings:
    """ Settings class """
    def get_settings( self, defaults=False ):
        """ read settings """
        try:
            settings = {}
            if ( defaults ): raise
            settings_file = open( BASE_SETTINGS_PATH, "r" )
            settings = eval( settings_file.read() )
            settings_file.close()
            if ( settings[ "version" ] not in SETTINGS_VERSIONS ):
                raise
        except:
            settings = self._use_defaults( settings, save=( defaults == False ) )
        return settings

    def _use_defaults( self, current_settings=None, save=True ):
        """ setup default values if none obtained """
        LOG( LOG_NOTICE, "%s (ver: %s) used default settings", __scriptname__, __version__ )
        settings = {}
        defaults = {  
            "scraper": "lyricwiki",
            "save_lyrics": True,
            "lyrics_path": os.path.join( BASE_DATA_PATH, "lyrics" ),
            "smooth_scrolling": False,
            "show_viz": True,
            "use_filename": False,
            "filename_format": 0,
            "music_path": "",
            "shuffle": True,
            "compatible": False,
            "use_extension": True,
            }
        for key, value in defaults.items():
            # add default values for missing settings
            settings[ key ] = current_settings.get( key, defaults[ key ] )
        settings[ "version" ] = __version__
        if ( save ):
            ok = self.save_settings( settings )
        return settings

    def save_settings( self, settings ):
        """ save settings """
        try:
            settings_file = open( BASE_SETTINGS_PATH, "w" )
            settings_file.write( repr( settings ) )
            settings_file.close()
            return True
        except:
            LOG( LOG_ERROR, "%s (rev: %s) %s::%s (%d) [%s]", __scriptname__, __svn_revision__, self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
            return False
