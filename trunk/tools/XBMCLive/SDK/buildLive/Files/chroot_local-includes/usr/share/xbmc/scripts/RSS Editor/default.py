import sys
import os
import xbmc

# Script constants
__scriptname__ = "RSS Editor"
__author__ = "rwparris2"
__url__ = "http://code.google.com/p/xbmc-addons/"
__credits__ = "Team XBMC"
__version__ = "1.5.5"

print "[SCRIPT] '%s: version %s' initialized!" % (__scriptname__, __version__, )

if (__name__ == "__main__"):
    import resources.lib.rssEditor as rssEditor
    ui = rssEditor.GUI("script-RSS_Editor-rssEditor.xml", os.getcwd(), "default", setNum = 'set1')
    del ui

sys.modules.clear()
