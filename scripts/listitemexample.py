import xbmcgui

def debug( str ):
    xbmc.output( str )

class Window( xbmcgui.Window ):
    def __init__( self ):
        xbmcgui.Window.__init__( self )
        self.checkMark = None

    def create( self ):
        self.ctl = xbmcgui.ControlList( 50, 200, 400, 200 )
        self.addControl( self.ctl )
        self.setFocus( self.ctl )
        labels = { 0: 'zero', 1: 'one', 2: 'two', 3: 'three' }
        for k,v in labels.items():
            self.ctl.addItem(
                xbmcgui.ListItem(
                    label=str(k), label2=v,
                    thumbnailImage="q:\\scripts\\test.png" ) )

    def onAction( self, action ):
        debug( "> Window.onAction( action=[%s] )"%action )
        if action in (9,10):
            self.close()
        debug( "< Window.onAction( action=[%s] )"%action )

win = Window()
win.create()
win.doModal()
del win

