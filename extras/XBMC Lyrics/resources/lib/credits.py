"""
Credits modules

Nuka1195
"""

import sys
import xbmcgui

from resources.lib.utilities import *

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__
__svn_url__ = sys.modules[ "__main__" ].__svn_url__
__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__


class GUI( xbmcgui.WindowXMLDialog ):
    def __init__( self, *args, **kwargs ):
        xbmcgui.WindowXMLDialog.__init__( self )

    def onInit( self ):
        self._show_credits()

    def _show_credits( self ):
        try:
            xbmcgui.lock()
            #team credits
            self.getControl( 20 ).setLabel( __scriptname__ )
            self.getControl( 30 ).setLabel( "%s: %s-%s" % ( _( 1006 ), __version__, __svn_revision__, ) )
            self.getControl( 40 ).addLabel( __svn_url__ )
            self.getControl( 901 ).setLabel( _( 901 ) )
            self.getControl( 101 ).reset()
            list_item = xbmcgui.ListItem( sys.modules[ "__main__" ].__credits_l1__, sys.modules[ "__main__" ].__credits_r1__ )
            self.getControl( 101 ).addItem( list_item )
            list_item = xbmcgui.ListItem( sys.modules[ "__main__" ].__credits_l2__, sys.modules[ "__main__" ].__credits_r2__ )
            self.getControl( 101 ).addItem( list_item )
            list_item = xbmcgui.ListItem( sys.modules[ "__main__" ].__credits_l3__, sys.modules[ "__main__" ].__credits_r3__ )
            self.getControl( 101 ).addItem( list_item )
            # Additional credits
            self.getControl( 902 ).setLabel( _( 902 ) )
            self.getControl( 102 ).reset()
            list_item = xbmcgui.ListItem( sys.modules[ "__main__" ].__add_credits_l1__, sys.modules[ "__main__" ].__add_credits_r1__ )
            self.getControl( 102 ).addItem( list_item )
            list_item = xbmcgui.ListItem( sys.modules[ "__main__" ].__add_credits_l2__, sys.modules[ "__main__" ].__add_credits_r2__ )
            self.getControl( 102 ).addItem( list_item )
            list_item = xbmcgui.ListItem( sys.modules[ "__main__" ].__add_credits_l3__, sys.modules[ "__main__" ].__add_credits_r3__ )
            self.getControl( 102 ).addItem( list_item )
            # Skin credits
            self.getControl( 903 ).setLabel( _( 903 ) )
        except:
            pass
        xbmcgui.unlock()

    def _close_dialog( self ):
        self.close()

    def onClick( self, controlId ):
        pass

    def onFocus( self, controlId ):
        pass

    def onAction( self, action ):
        if ( action in ACTION_CANCEL_DIALOG ):
            self._close_dialog()
