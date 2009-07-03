"""
 Common functions
"""

import sys
import xbmc
from urllib import unquote_plus

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '22-04-2009'
xbmc.log("[PLUGIN] Module: %s Date: %s loaded!" % (__name__, __date__), xbmc.LOGDEBUG)

#################################################################################################################
def log(msg):
	xbmc.log("[%s]: %s" % (__plugin__, msg), xbmc.LOGDEBUG)

log("Module: %s Dated: %s loaded!" % (__name__, __date__))

#################################################################################################################
def handleException(msg=""):
	import traceback
	import xbmcgui
	traceback.print_exc()
	xbmcgui.Dialog().ok(__plugin__ + " ERROR!", msg, str(sys.exc_info()[ 1 ]))

#################################################################################################################
class Info:
	def __init__( self, *args, **kwargs ):
		self.__dict__.update( kwargs )
		log( "Info() self.__dict__=%s" % self.__dict__ )
	def has_key(self, key):
		return self.__dict__.has_key(key)

#################################################################################################################
def loadFileObj( filename ):
	log( "loadFileObj() " + filename)
	try:
		file_handle = open( filename, "r" )
		loadObj = eval( file_handle.read() )
		file_handle.close()
	except Exception, e:
		log( "loadFileObj() " + str(e) )
		loadObj = None
	return loadObj

#################################################################################################################
def saveFileObj( filename, saveObj ):
	log( "saveFileObj() " + filename)
	try:
		file_handle = open( filename, "w" )
		file_handle.write( repr( saveObj ) )
		file_handle.close()
		return True
	except Exception, e:
		log( "save_file_obj() " + str(e) )
		return False
