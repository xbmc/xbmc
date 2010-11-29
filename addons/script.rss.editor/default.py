import sys
import os
import xbmc
import xbmcaddon

# Script constants
__scriptname__ = "RSS Editor"
__author__     = "rwparris2"
__url__        = "http://code.google.com/p/xbmc-addons/"
__credits__    = "Team XBMC"
__version__    = "1.5.6"
__settings__   = xbmcaddon.Addon(id='script.rss.editor')
__language__   = __settings__.getLocalizedString
__cwd__        = __settings__.getAddonInfo('path')

print "[SCRIPT] '%s: version %s' initialized!" % (__scriptname__, __version__, )

if (__name__ == "__main__"):
    import resources.lib.rssEditor as rssEditor
    ui = rssEditor.GUI("script-RSS_Editor-rssEditor.xml", __cwd__, "default", setNum = 'set1')
    del ui

sys.modules.clear()
