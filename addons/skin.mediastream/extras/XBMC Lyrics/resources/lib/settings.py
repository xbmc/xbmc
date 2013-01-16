"""
Settings module

Nuka1195
"""

import sys
import os
import xbmc
import xbmcgui

from resources.lib.utilities import *

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__
__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__


class GUI( xbmcgui.WindowXMLDialog ):
    """ Settings module: used for changing settings """
    def __init__( self, *args, **kwargs ):
        xbmcgui.WindowXMLDialog.__init__( self )

    def onInit( self ):
        self._get_settings()
        self._set_labels()
        self._set_functions()
        self._setup_special()
        self._set_restart_required()
        self._set_controls_values()

    def _get_settings( self, defaults=False ):
        """ reads settings """
        self.settings = Settings().get_settings( defaults=defaults )

    def _set_labels( self ):
        xbmcgui.lock()
        try:
            self.getControl( 30 ).setLabel( "%s: %s-%s" % ( _( 1006 ), __version__, __svn_revision__, ) )
            ## setEnabled( False ) if not used
            #self.getControl( 253 ).setVisible( False )
            #self.getControl( 253 ).setEnabled( False )
        except: pass
        xbmcgui.unlock()

    def _set_functions( self ):
        self.functions = {}
        self.functions[ 250 ] = self._save_settings
        self.functions[ 251 ] = self._close_dialog
        self.functions[ 252 ] = self._update_script
        self.functions[ 253 ] = self._show_credits
        self.functions[ 254 ] = self._play_music
        self.functions[ 255 ] = self._get_defaults
        for x in range( 1, len( self.settings ) ):
            self.functions[ 200 + x ] = eval( "self._change_setting%d" % x )

##### Special defs, script dependent, remember to call them from _setup_special #################
    
    def _setup_special( self ):
        """ calls any special defs """
        self._setup_scrapers()
        self._setup_filename_format()

    def _setup_scrapers( self ):
        """ special def for setting up scraper choices """
        import re
        self.scrapers_title = []
        pattern = """__title__.*?["'](.*?)["']"""
        base_path = os.path.join( BASE_RESOURCE_PATH, "scrapers" )
        self.scrapers = []
        scrapers = os.listdir( base_path )
        for scraper in scrapers:
            if ( os.path.isdir( os.path.join( base_path, scraper ) ) ):
                try:
                    self.scrapers += [ scraper ]
                    file_object = open( os.path.join( base_path, scraper, "lyricsScraper.py" ), "r" )
                    file_data = file_object.read()
                    file_object.close()
                    title = re.findall( pattern, file_data )
                    if ( not title ): raise
                except:
                    title = [ scraper ]
                self.scrapers_title += title
        try: self.current_scraper = self.scrapers.index( self.settings[ "scraper" ] )
        except: self.current_scraper = 0

    def _setup_filename_format( self ):
        self.filename_format = ( _( 2070 ), _( 2071 ), _( 2072 ), )
    
    def _set_restart_required( self ):
        """ copies self.settings and adds any settings that require a restart on change """
        self.settings_original = self.settings.copy()
        self.settings_restart = ( "scraper", )
        self.settings_refresh = ( "save_lyrics", "lyrics_path", "use_filename", "filename_format", )

###### End of Special defs #####################################################

    def _set_controls_values( self ):
        """ sets the value labels """
        xbmcgui.lock()
        try:
            self.getControl( 201 ).setLabel( self.getControl( 201 ).getLabel(), label2=self.scrapers_title[ self.current_scraper ] )
            self.getControl( 202 ).setSelected( self.settings[ "save_lyrics" ] )
            self.getControl( 203 ).setLabel( self.getControl( 203 ).getLabel(), label2=self.settings[ "lyrics_path" ] )
            self.getControl( 203 ).setEnabled( self.settings[ "save_lyrics" ] )
            self.getControl( 204 ).setSelected( self.settings[ "smooth_scrolling" ] )
            self.getControl( 205 ).setSelected( self.settings[ "show_viz" ] )
            self.getControl( 206 ).setSelected( self.settings[ "use_filename" ] )
            self.getControl( 207 ).setLabel( self.getControl( 207 ).getLabel(), label2=self.filename_format[ self.settings[ "filename_format" ] ] )
            self.getControl( 208 ).setLabel( self.getControl( 208 ).getLabel(), label2=self.settings[ "music_path" ] )
            self.getControl( 209 ).setSelected( self.settings[ "shuffle" ] )
            self.getControl( 209 ).setEnabled( self.settings[ "music_path" ] != "" )
            self.getControl( 210 ).setSelected( self.settings[ "compatible" ] )
            self.getControl( 210 ).setEnabled( self.settings[ "save_lyrics" ] )
            self.getControl( 211 ).setSelected( self.settings[ "use_extension" ] )
            self.getControl( 211 ).setEnabled( self.settings[ "save_lyrics" ] )
            self.getControl( 250 ).setEnabled( self.settings_original != self.settings )
            self.getControl( 254 ).setEnabled( not xbmc.Player().isPlayingAudio() )
        except:
            pass
        xbmcgui.unlock()

    def _change_setting1( self ):
        """ changes settings #1 """
        self.current_scraper += 1
        if ( self.current_scraper == len( self.scrapers ) ): self.current_scraper = 0
        self.settings[ "scraper" ] = self.scrapers[ self.current_scraper ]
        self._set_controls_values()

    def _change_setting2( self ):
        """ changes settings #2 """
        self.settings[ "save_lyrics" ] = not self.settings[ "save_lyrics" ]
        self._set_controls_values()

    def _change_setting3( self ):
        """ changes settings #3 """
        self.settings[ "lyrics_path" ] = get_browse_dialog( self.settings[ "lyrics_path" ], _( self.controlId ), 3 )
        self._set_controls_values()

    def _change_setting4( self ):
        """ changes settings #4 """
        self.settings[ "smooth_scrolling" ] = not self.settings[ "smooth_scrolling" ]
        self._set_controls_values()

    def _change_setting5( self ):
        """ changes settings #5 """
        self.settings[ "show_viz" ] = not self.settings[ "show_viz" ]
        self._set_controls_values()

    def _change_setting6( self ):
        """ changes settings #6 """
        self.settings[ "use_filename" ] = not self.settings[ "use_filename" ]
        self._set_controls_values()

    def _change_setting7( self ):
        """ changes settings #7 """
        self.settings[ "filename_format" ] += 1
        if ( self.settings[ "filename_format" ] == len( self.filename_format ) ):
            self.settings[ "filename_format" ] = 0
        self._set_controls_values()

    def _change_setting8( self ):
        """ changes settings #8 """
        self.settings[ "music_path" ] = get_browse_dialog( self.settings[ "music_path" ], _( self.controlId ), 0 )
        self._set_controls_values()

    def _change_setting9( self ):
        """ changes settings #9 """
        self.settings[ "shuffle" ] = not self.settings[ "shuffle" ]
        self._set_controls_values()

    def _change_setting10( self ):
        """ changes settings #10 """
        self.settings[ "compatible" ] = not self.settings[ "compatible" ]
        self._set_controls_values()

    def _change_setting11( self ):
        """ changes settings #11 """
        self.settings[ "use_extension" ] = not self.settings[ "use_extension" ]
        self._set_controls_values()

##### End of unique defs ######################################################
    
    def _save_settings( self ):
        """ saves settings """
        ok = Settings().save_settings( self.settings )
        if ( not ok ):
            ok = xbmcgui.Dialog().ok( __scriptname__, _( 230 ) )
        else:
            self._check_for_restart()

    def _check_for_restart( self ):
        """ checks for any changes that require a restart to take effect """
        restart = False
        refresh = False
        for setting in self.settings_restart:
            if ( self.settings_original[ setting ] != self.settings[ setting ] ):
                restart = True
                break
        for setting in self.settings_refresh:
            if ( self.settings_original[ setting ] != self.settings[ setting ] ):
                refresh = True
                break
        self._close_dialog( True, restart, refresh )
    
    def _update_script( self ):
        """ checks for updates to the script """
        import resources.lib.update as update
        updt = update.Update()
        del updt

    def _show_credits( self ):
        """ shows a credit window """
        show_credits()

    def _play_music( self ):
        """ plays a folder of music """
        if ( self.settings_original != self.settings ):
            self._save_settings()
        else:
            self._close_dialog()
        import resources.lib.playlist as playlist
        playlist = playlist.create_playlist( ( self.settings[ "music_path" ], ), self.settings[ "shuffle" ] )
        xbmc.Player().play( playlist )
        xbmc.executebuiltin( "RunScript(%s)" % os.path.join( os.getcwd(), "default.py" ) )
        
    def _get_defaults( self ):
        """ resets values to defaults """
        self._get_settings( defaults=True )
        self._set_controls_values()

    def _close_dialog( self, changed=False, restart=False, refresh=False ):
        """ closes this dialog window """
        self.changed = changed
        self.restart = restart
        self.refresh = refresh
        self.close()

    def onClick( self, controlId ):
        #xbmc.sleep(5)
        self.functions[ controlId ]()

    def onFocus( self, controlId ):
        xbmc.sleep( 5 )
        self.controlId = self.getFocusId()

    def onAction( self, action ):
        if ( action in ACTION_CANCEL_DIALOG ):
            self._close_dialog()
