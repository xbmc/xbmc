import sys
import os
import xbmc
import string
import tempfile
__scriptname__ = "OpenSubtitles_OSD"
__author__ = "Amet"
__url__ = "http://code.google.com/p/opensubtitles-osd/"
__svn_url__ = "http://xbmc-addons.googlecode.com/svn/trunk/scripts/OpenSubtitles_OSD"
__credits__ = ""
__version__ = "1.44"
__XBMC_Revision__ = "22240"

### ------ thanks to hentar for adding the third language option ----- ###

BASE_RESOURCE_PATH = xbmc.translatePath( os.path.join( os.getcwd(), 'resources', 'lib' ) )

sys.path.append (BASE_RESOURCE_PATH)




__language__ = xbmc.Language( os.getcwd() ).getLocalizedString
_ = sys.modules[ "__main__" ].__language__
__settings__ = xbmc.Settings( uuid=os.getcwd() )



if __settings__.getSetting( "new_ver" ) == "true":
	import re
	import urllib
	if not xbmc.getCondVisibility('Player.Paused') : xbmc.Player().pause() #Pause if not paused	
	usock = urllib.urlopen(__svn_url__ + "/default.py")
	htmlSource = usock.read()
	usock.close()

	version = re.search( "__version__.*?[\"'](.*?)[\"']",  htmlSource, re.IGNORECASE ).group(1)
	print "SVN Latest Version :[ "+version+"]"
	
	if version > __version__:
		import xbmcgui
		dialog = xbmcgui.Dialog()
		
		selected = dialog.ok("OpenSubtitles_OSD v" + str(__version__), "Version "+ str(version)+ " of OpenSubtitles_OSD is available" ,"Please use SVN repo Installer or XBMC zone Installer to update " )



#############-----------------Is script runing from OSD? -------------------------------###############


if  not xbmc.getCondVisibility('videoplayer.isfullscreen') :

	import xbmcgui
	dialog = xbmcgui.Dialog()
	selected = dialog.ok("OpenSubtitles_OSD", "This script needs to run from OSD" ,"For More Info Visit ", "http://code.google.com/p/opensubtitles-osd/" )




else:
   window = False
   skin = "main"
   skin1 = str(xbmc.getSkinDir().lower())
   skin1 = skin1.replace("-"," ")
   skin1 = skin1.replace("."," ")
   skin1 = skin1.replace("_"," ")
   if ( skin1.find( "eedia" ) > -1 ):
	skin = "MiniMeedia"
   if ( skin1.find( "tream" ) > -1 ):
	skin = "MediaStream"
   if ( skin1.find( "edux" ) > -1 ):
	skin = "MediaStream_Redux"
   if ( skin1.find( "aeon" ) > -1 ):
	skin = "Aeon"
   if ( skin1.find( "alaska" ) > -1 ):
	skin = "Aeon"
   if ( skin1.find( "confluence" ) > -1 ):
	skin = "confluence"	
   
   if __settings__.getSetting( "debug" ) == "true":	
   		print "OpenSubtitles_OSD version [" +  __version__ +"]"
   		print "Skin Folder: [ " + skin1 +" ]"
   		print "OpenSubtitles_OSD skin XML: [ " + skin +" ]"
   		debug = True
   else:
    	debug = False   
   

###-------------------Extract Media files -----------------------------------################
   
   mediafolder = os.path.join(xbmc.translatePath("special://home/scripts/"), __scriptname__ ,"resources","skins" , "Default" , "media" )
   zip_file = os.path.join(xbmc.translatePath("special://home/scripts/"), __scriptname__ ,"resources","lib" , "media.zip" )
   if os.path.exists(zip_file):
		if os.path.exists(mediafolder):
			import shutil
			shutil.rmtree(mediafolder)
   
		import unzip
		
		un = unzip.unzip()	
		un.extract_med( zip_file, mediafolder )
		os.remove(zip_file)

###-------------------------- Set Search String and Path string -------------################

   if ( __name__ == "__main__" ):
	
		
		search_string = ""
		
		if len(xbmc.getInfoLabel("VideoPlayer.TVshowtitle")) > 1: # TvShow
	
				year = 0
				season = str(xbmc.getInfoLabel("VideoPlayer.Season"))
				episode = str (xbmc.getInfoLabel("VideoPlayer.Episode"))
				showname = xbmc.getInfoLabel("VideoPlayer.TVshowtitle")
				
				epchck = episode.lower()
				if epchck.find("s") > -1: # Check if season is "Special"
					season = "0"
					episode = episode.replace("s","")
					episode = episode.replace("S","")
				if ( int( season ) < 10 ):
					search_string = "S0" + season
				else:
					search_string = "S" + season
	
				if ( int( episode ) < 10 ):
					search_string = search_string + "E0" + episode
				else:
					search_string = search_string + "E" + episode
	
				search_string = showname + "+" + search_string 	
			
		else: # Movie or not in Library
		
				year = xbmc.getInfoLabel("VideoPlayer.Year")
				title = xbmc.getInfoLabel("VideoPlayer.Title")
	
				if str(year) == "": # Not in Library
					from utilities import getMovieTitleAndYear
					search_string, year = getMovieTitleAndYear( title )
					
				else: # Movie in Library
					search_string = title
						
		search_string = search_string.replace(" ","+")
	
	
	#### ------------------------------ Get User Settings ---------------------------#####
	
		temp = False
		access = True
		rar = False
		movieFullPath = xbmc.Player().getPlayingFile()
		path = __settings__.getSetting( "subfolder" ) == "true"
		print str(path)


		if (movieFullPath.find("http://") > -1 ):
			temp = True

		if (movieFullPath.find("rar://") > -1 ) and path:
			rar = True
			import urllib
			sub_folder = os.path.dirname(urllib.unquote(movieFullPath))
			sub_folder = sub_folder.replace("rar://","")
			sub_folder = os.path.dirname( sub_folder )
			
		if not path and not rar:
			sub_folder = xbmc.translatePath(__settings__.getSetting( "subfolderpath" ))
			if len(sub_folder) < 1 :
				sub_folder = os.path.dirname( movieFullPath )
				
		
		if path and not rar:
			sub_folder = xbmc.translatePath(__settings__.getSetting( "subfolderpath" ))
			if not access or sub_folder.find("smb://") > -1:
				if temp:
					import xbmcgui
					dialog = xbmcgui.Dialog()
					sub_folder = dialog.browse( 0, "Choose Subtitle folder", "files")
				else:
					sub_folder = os.path.dirname( movieFullPath )
			else:
				sub_folder = os.path.dirname( movieFullPath )	
	
	#### ------------------------------ Get the main window going ---------------------------#####
		import gui
	
		if not xbmc.getCondVisibility('Player.Paused') : xbmc.Player().pause() #Pause if not paused
		
		
		ui = gui.GUI( "script-OpenSubtitles_OSD-"+ skin +".xml" , os.getcwd(), "Default")
		service_present = ui.set_allparam ( movieFullPath,search_string,temp,sub_folder, year, debug )
		if service_present > -1 : ui.doModal()
		
		if xbmc.getCondVisibility('Player.Paused'): xbmc.Player().pause() # if Paused, un-pause
		del ui
		sys.modules.clear()
