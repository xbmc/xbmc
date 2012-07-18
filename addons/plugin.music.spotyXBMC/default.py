import xbmcplugin,xbmcgui,xbmcaddon

# plugin constants
__version__ = "0.0.1"
__plugin__ = "spotyXBMC-" + __version__
__author__ = "David Erenger"
__url__ = "https://github.com/akezeke/xbmc"
__settings__ = xbmcaddon.Addon(id='plugin.music.spotyXBMC')
__language__ = __settings__.getLocalizedString

def startPlugin():
	if (__settings__.getSetting("enable") == "false"):
		__settings__.openSettings()	
		xbmc.executebuiltin('XBMC.ReplaceWindow(home)')

	else:
        	xbmc.executebuiltin('XBMC.ReplaceWindow(musiclibrary,musicdb://)')
        	xbmc.executebuiltin('XBMC.Action(parentdir)')	

startPlugin()
