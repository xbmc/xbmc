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

