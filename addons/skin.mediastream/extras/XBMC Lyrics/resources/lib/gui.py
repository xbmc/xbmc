"""
	Credits:	
		EnderW:					Original Author
		Stanley87:				PM3 Integration and AD Removal
		solexalex:
		TomKun:
		Thor918:				MyPlayer Class
		Smuto:					Skinning Mod
		Spiff:						Unicode support
		Nuka1195:				lyricwiki & embedded Scraper and Modulization
				
Please report any bugs: http://www.xboxmediacenter.com/forum/showthread.php?t=10187
"""

import sys
import os
import xbmc
import xbmcgui
import threading

from resources.lib.utilities import *

try:
    current_dlg_id = xbmcgui.getCurrentWindowDialogId()
except:
    current_dlg_id = 0
current_win_id = xbmcgui.getCurrentWindowId()

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__
__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__


class GUI( xbmcgui.WindowXMLDialog ):
    def __init__( self, *args, **kwargs ):
        xbmcgui.WindowXMLDialog.__init__( self )

    def onInit( self ):
        self.setup_all()#Start( function=self.setup_all ).start()

    def setup_all( self ):
        self.setup_variables()
        self.get_settings()
        self.get_scraper()
        self.getDummyTimer()
        self.getMyPlayer()
        self.show_viz_window()

    def get_settings( self ):
        self.settings = Settings().get_settings()
        
    def get_scraper( self ):
        exec "import resources.scrapers.%s.lyricsScraper as lyricsScraper" % ( self.settings[ "scraper" ], )
        self.LyricsScraper = lyricsScraper.LyricsFetcher()
        self.scraper_title = lyricsScraper.__title__
        self.scraper_exceptions = lyricsScraper.__allow_exceptions__

    def setup_variables( self ):
        self.artist = None
        self.song = None
        self.Timer = None
        self.controlId = -1
        self.allow_exception = False

    def show_viz_window( self, startup=True ):
        if ( self.settings[ "show_viz" ] ):
            xbmc.executebuiltin( "XBMC.ActivateWindow(2006)" )
        else:
            if ( current_dlg_id != 9999 or not startup ):
                xbmc.executebuiltin( "XBMC.ActivateWindow(%s)" % ( current_win_id, ) )

    def show_control( self, controlId ):
        self.getControl( 100 ).setVisible( controlId == 100 )
        self.getControl( 110 ).setVisible( controlId == 110 )
        self.getControl( 120 ).setVisible( controlId == 120 )
        page_control = ( controlId == 100 )
        xbmcgui.unlock()
        xbmc.sleep( 5 )
        try:
            self.setFocus( self.getControl( controlId + page_control ) )
        except:
            self.setFocus( self.getControl( controlId ) )

    def get_lyrics(self, artist, song):
        self.reset_controls()
        self.getControl( 200 ).setLabel( "" )
        self.menu_items = []
        self.allow_exception = False
        current_song = self.song
        lyrics, kind = self.get_lyrics_from_file( artist, song )
        if ( lyrics is not None ):
            if ( current_song == self.song ):
                self.show_lyrics( lyrics )
                self.getControl( 200 ).setEnabled( False )
                self.getControl( 200 ).setLabel( _( 101 + kind ) )
        else:
            self.getControl( 200 ).setEnabled( True )
            self.getControl( 200 ).setLabel( self.scraper_title )
            lyrics = self.LyricsScraper.get_lyrics( artist, song )
            if ( current_song == self.song ):
                if ( isinstance( lyrics, basestring ) ):
                    self.show_lyrics( lyrics, True )
                elif ( isinstance( lyrics, list ) and lyrics ):
                    self.show_choices( lyrics )
                else:
                    self.getControl( 200 ).setEnabled( False )
                    self.show_lyrics( _( 631 ) )
                    self.allow_exception = True

    def get_lyrics_from_list( self, item ):
        lyrics = self.LyricsScraper.get_lyrics_from_list( self.menu_items[ item ] )
        self.show_lyrics( lyrics, True )

    def get_lyrics_from_file( self, artist, song ):
        try:
            xbmc.sleep( 60 )
            if ( xbmc.getInfoLabel( "MusicPlayer.Lyrics" ) ):
                return unicode( xbmc.getInfoLabel( "MusicPlayer.Lyrics" ), "utf-8" ), True
            self.song_path = make_legal_filepath( unicode( os.path.join( self.settings[ "lyrics_path" ], artist.replace( "\\", "_" ).replace( "/", "_" ), song.replace( "\\", "_" ).replace( "/", "_" ) + ( "", ".txt", )[ self.settings[ "use_extension" ] ] ), "utf-8" ), self.settings[ "compatible" ], self.settings[ "use_extension" ] )
            lyrics_file = open( self.song_path, "r" )
            lyrics = lyrics_file.read()
            lyrics_file.close()
            return unicode( lyrics, "utf-8" ), False
        except IOError:
            return None, False

    def save_lyrics_to_file( self, lyrics ):
        try:
            if ( not os.path.isdir( os.path.dirname( self.song_path ) ) ):
                os.makedirs( os.path.dirname( self.song_path ) )
            lyrics_file = open( self.song_path, "w" )
            lyrics_file.write( lyrics.encode( "utf-8", "ignore" ) )
            lyrics_file.close()
            return True
        except IOError:
            LOG( LOG_ERROR, "%s (rev: %s) %s::%s (%d) [%s]", __scriptname__, __svn_revision__, self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
            return False

    def show_lyrics( self, lyrics, save=False ):
        xbmcgui.lock()
        if ( lyrics == "" ):
            self.getControl( 100 ).setText( _( 632 ) )
            self.getControl( 110 ).addItem( _( 632 ) )
        else:
            if ( "\r\n" in lyrics ):
                sep = "\r\n"
            else:
                # XBMC textbox does not handle "\r", so replace it with "\n"
                sep = "\n"
                lyrics = lyrics.replace( "\r" , "\n" )
            self.getControl( 100 ).setText( lyrics )
            for x in lyrics.split( sep ):
                self.getControl( 110 ).addItem( x )
            self.getControl( 110 ).selectItem( 0 )
            if ( self.settings[ "save_lyrics" ] and save ): success = self.save_lyrics_to_file( lyrics )
        self.show_control( 100 + ( self.settings[ "smooth_scrolling" ] * 10 ) )
        
    def show_choices( self, choices ):
        xbmcgui.lock()
        for song in choices:
            self.getControl( 120 ).addItem( song[ 0 ] )
        self.getControl( 120 ).selectItem( 0 )
        self.menu_items = choices
        self.show_control( 120 )
    
    def reset_controls( self ):
        self.getControl( 100 ).reset()
        self.getControl( 110 ).reset()
        self.getControl( 120 ).reset()
        
    def change_settings( self ):
        import resources.lib.settings as settings
        settings = settings.GUI( "script-%s-settings.xml" % ( __scriptname__.replace( " ", "_" ), ), os.getcwd(), "Default" )
        settings.doModal()
        ok = False
        if ( settings.changed ):
            self.get_settings()
            if ( settings.restart ):
                ok = xbmcgui.Dialog().yesno( __scriptname__, _( 240 ), "", _( 241 ) % ( __scriptname__, ), _( 271 ), _( 270 ) )
            if ( not ok ):
                self.show_control( ( 100 + ( self.settings[ "smooth_scrolling" ] * 10 ), 120, )[ self.controlId == 120 ] )
                self.show_viz_window( startup=False )
                if ( settings.refresh ):
                    self.myPlayerChanged( 2, True )
            else: self.exit_script( True )
        del settings

    def _show_credits( self ):
        """ shows a credit window """
        show_credits()

    def get_exception( self ):
        """ user modified exceptions """
        if ( self.scraper_exceptions ):
            artist = self.LyricsScraper._format_param( self.artist, False )
            alt_artist = get_keyboard( artist, "%s: %s" % ( _( 100 ), unicode( self.artist, "utf-8", "ignore" ), ) )
            if ( alt_artist != artist ):
                exception = ( artist, alt_artist, )
                self.LyricsScraper._set_exceptions( exception )
                self.myPlayerChanged( 2, True )

    def exit_script( self, restart=False ):
        if ( self.Timer is not None ): self.Timer.cancel()
        self.close()
        if ( restart ): xbmc.executebuiltin( "XBMC.RunScript(%s)" % ( os.path.join( os.getcwd(), "default.py" ), ) )

    def onClick( self, controlId ):
        if ( controlId == 120 ):
            self.get_lyrics_from_list( self.getControl( 120 ).getSelectedPosition() )

    def onFocus( self, controlId ):
        #xbmc.sleep( 5 )
        #self.controlId = self.getFocusId()
        self.controlId = controlId

    def onAction( self, action ):
        actionId = action.getId()
        if ( actionId in ACTION_EXIT_SCRIPT ):
            self.exit_script()
        elif ( actionId in ACTION_SETTINGS_MENU ):
            self.change_settings()
        #elif ( action.getButtonCode() in SHOW_CREDITS ):
        #    self._show_credits()
        elif ( self.allow_exception and actionId in ACTION_GET_EXCEPTION ):
            self.get_exception()

    def get_artist_from_filename( self, filename ):
        try:
            artist = filename
            song = filename
            basename = os.path.basename( filename )
            # Artist - Song.ext
            if ( self.settings[ "filename_format" ] == 0 ):
                artist = basename.split( "-", 1 )[ 0 ].strip()
                song = os.path.splitext( basename.split( "-", 1 )[ 1 ].strip() )[ 0 ]
            # Artist/Album/Song.ext or Artist/Album/Track Song.ext
            elif ( self.settings[ "filename_format" ] in ( 1, 2, ) ):
                artist = os.path.basename( os.path.split( os.path.split( filename )[ 0 ] )[ 0 ] )
                # Artist/Album/Song.ext
                if ( self.settings[ "filename_format" ] == 1 ):
                    song = os.path.splitext( basename )[ 0 ]
                # Artist/Album/Track Song.ext
                elif ( self.settings[ "filename_format" ] == 2 ):
                    song = os.path.splitext( basename )[ 0 ].split( " ", 1 )[ 1 ]
        except:
            # invalid format selected
            LOG( LOG_ERROR, "%s (rev: %s) %s::%s (%d) [%s]", __scriptname__, __svn_revision__, self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
        return artist, song

    # getDummyTimer() and self.Timer are currently used for the Player() subclass so when an onPlayback* event occurs, 
    # it calls myPlayerChanged() immediately.
    def getDummyTimer( self ):
        self.Timer = threading.Timer( 60*60*60, self.getDummyTimer,() )
        self.Timer.start()
    
    def getMyPlayer( self ):
        self.MyPlayer = MyPlayer( xbmc.PLAYER_CORE_PAPLAYER, function=self.myPlayerChanged )
        self.myPlayerChanged( 2 )

    def myPlayerChanged( self, event, force_update=False ):
        LOG( LOG_DEBUG, "%s (rev: %s) GUI::myPlayerChanged [%s]", __scriptname__, __svn_revision__, [ "stopped","ended","started" ][ event ] )
        if ( event < 2 ): 
            self.exit_script()
        else:
            for cnt in range( 5 ):
                song = xbmc.getInfoLabel( "MusicPlayer.Title" )
                artist = xbmc.getInfoLabel( "MusicPlayer.Artist" )
                if ( song and ( not artist or self.settings[ "use_filename" ] ) ):
                    artist, song = self.get_artist_from_filename( xbmc.Player().getPlayingFile() )
                if ( song and ( self.song != song or self.artist != artist or force_update ) ):
                    self.artist = artist
                    self.song = song
                    self.get_lyrics( artist, song )
                    break
                xbmc.sleep( 50 )


## Thanks Thor918 for this class ##
class MyPlayer( xbmc.Player ):
    """ Player Class: calls function when song changes or playback ends """
    def __init__( self, *args, **kwargs ):
        xbmc.Player.__init__( self )
        self.function = kwargs[ "function" ]

    def onPlayBackStopped( self ):
        self.function( 0 )
    
    def onPlayBackEnded( self ):
        xbmc.sleep( 300 )
        if ( not xbmc.Player().isPlayingAudio() ):
            self.function( 1 )
    
    def onPlayBackStarted( self ):
        self.function( 2 )


#class Start( threading.Thread ):
#    """ Thread Class used to allow gui to show before all checks are done at start of script """
#    def __init__( self, *args, **kwargs ):
#        threading.Thread.__init__( self )
#        self.function = kwargs[ "function" ]
#
#    def run(self):
#        self.function()
