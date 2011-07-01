import xbmcplugin,xbmcgui,xbmcaddon

# plugin constants
__version__ = "0.0.1"
__plugin__ = "spotyXBMC-" + __version__
__author__ = "David Erenger"
__url__ = "https://github.com/akezeke/xbmc"
__settings__ = xbmcaddon.Addon(id='plugin.music.spotyXBMC')
__language__ = __settings__.getLocalizedString

def startPlugin():	
    xbmc.executebuiltin('XBMC.ReplaceWindow(10005,"musicdb://spotify/menu/main/")')

startPlugin()
