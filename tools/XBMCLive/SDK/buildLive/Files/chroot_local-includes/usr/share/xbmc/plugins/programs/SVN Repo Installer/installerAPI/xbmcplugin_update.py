"""

	Programs/Plugin to check for SVN Repo updates of your installed scripts and Plugins

	This version as part of "SVN Repo installer" Programs/Plugin

	Written by BigBellyBilly
	Contact me at BigBellyBilly at gmail dot com - Bugs reports and suggestions welcome.

""" 

import sys, os
import os.path
import xbmc, xbmcgui, xbmcplugin
import urllib
import re
from string import find
#from pprint import pprint
from xbmcplugin_lib import *

# Script constants
__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '14-09-2009'
log("Module: %s Dated: %s loaded!" % (__name__, __date__))

# check if build is special:// aware - set roots paths accordingly
XBMC_HOME = 'special://home'
if not os.path.isdir(xbmc.translatePath(XBMC_HOME)):    # if fails to convert to Q:, old builds
	XBMC_HOME = 'Q:'
log("XBMC_HOME=%s" % XBMC_HOME)


class Main:

	URL_BASE_SVN_SCRIPTING = "http://xbmc-scripting.googlecode.com/svn"
	INSTALLED_ITEMS_FILENAME = os.path.join( os.getcwd(), "installed_items.dat" )
	UPDATE_ALL_FILENAME = os.path.join( os.getcwd(), "update_all.dat" )

	def __init__( self ):
		log( "%s init!" % self.__class__ )
		ok = False
		# set our plugin category
		xbmcplugin.setPluginCategory( handle=int( sys.argv[ 1 ] ), category=xbmc.getLocalizedString( 30500 ) )

		# load settings
		self.showNoSVN = bool(xbmcplugin.getSetting( "show_no_svn" ) == "true")
		self.showNoVer = bool(xbmcplugin.getSetting( "show_no_ver" ) == "true")

		self.XBMC_REVISION = get_xbmc_revision()
		self.SVN_URL_LIST = load_repos()

		#['plugin://programs/SVN Repo Installer/', '-1', '?download_url="%2Ftrunk%2Fplugins%2Fmusic/iTunes%2F"&repo=\'xbmc-addons\'&install=""&ioffset=2&voffset=0']
		# create all XBMC script/plugin paths
		paths = ("plugins/programs","plugins/video","plugins/music","plugins/pictures","plugins/weather","scripts")
		self.XBMC_PATHS = []
		for p in paths:
			self.XBMC_PATHS.append( xbmc.translatePath( "/".join( [XBMC_HOME, p] ) ) )

		self.dialogProgress = xbmcgui.DialogProgress()
		self.dialogProgress.create(__plugin__)
		self.findInstalled()
		if self.INSTALLED:
			self.checkUpdates()
		self.dialogProgress.close()

#		pprint (self.INSTALLED)
		if self.INSTALLED:
			ok = self.showUpdates()
			if ok:
				saveFileObj(self.INSTALLED_ITEMS_FILENAME, self.INSTALLED)
		else:
			xbmcgui.Dialog().ok(__plugin__, "No installed Addons found!")

		log("endOfDirectory() ok=%s" % ok)
		xbmcplugin.endOfDirectory( int( sys.argv[ 1 ] ), ok, cacheToDisc=False)


	def findInstalled(self):
		log("> findInstalled()")
		
		self.INSTALLED = []
		ignoreList = (".","..",".backups",".svn")
		self.dialogProgress.update(0, xbmc.getLocalizedString( 30009 )) # Looking for installed addons
		TOTAL_PATHS = len(self.XBMC_PATHS)
		for count, p in enumerate(self.XBMC_PATHS):
			if not os.path.isdir(p): continue

			files = os.listdir(p)
			for f in files:
				# ignore some
				if f in ignoreList: continue

				percent = int( count * 100.0 / TOTAL_PATHS )
				self.dialogProgress.update(percent)

				# extract version
				try:
					filepath = os.path.join( p, f )
					log("filepath=" + filepath)
					doc = open( os.path.join(filepath, "default.py"), "r" ).read()
					docTags = parseAllDocTags( doc )

					if docTags and (docTags["version"] or self.showNoVer):
						thumb = os.path.join(filepath, "default.tbn")
						if not os.path.isfile( thumb ):
							thumb = "DefaultFile.png"
						cat = parseCategory(filepath)
						# if no title, parse category for it
						if not docTags["title"]:
							docTags["title"] = cat.split("/")[-1]
						tags = {"filepath": filepath,
								"thumb": thumb,
								"category": cat
								}
						tags.update(docTags)
						self.INSTALLED.append( tags )
				except:
					logError()

		log("< findInstalled()  installed count=%d" % len(self.INSTALLED))

	#####################################################################################################
	def checkUpdates(self):
		log("> checkUpdates()")

		actionMsg = xbmc.getLocalizedString( 30010 )
		self.dialogProgress.update(0, actionMsg)

		TOTAL_PATHS = len(self.INSTALLED)
		quit = False
		# enumarate throu each installed Addon, checking svn for update
		for count, info in enumerate(self.INSTALLED):
			log("%d) checking installed=%s" % (count, info))

			# unknown installed ver is OK, missing doc tag isn't
			if info.get('version',None) == None:
				log("ignored, missing a version tag")
				continue

			# find installed category from filepath
			installedCategory  = info['category']

			for repo in self.SVN_URL_LIST:
				try:
					REPO_URL, REPO_ROOT, REPO_STRUCTURES = get_repo_info( repo )
					repo_name, repo_noffset, repo_install, repo_ioffset, repo_voffset = REPO_STRUCTURES[0]
				except:
					continue	# repo info missing

				base_url = REPO_URL + REPO_ROOT
				# xbmc-scripting has no 'scripts' in svn path, so remove it from installedCategory
				if base_url.startswith(self.URL_BASE_SVN_SCRIPTING):
					installedCategory = installedCategory.replace('scripts/','')
				url = "/".join( [base_url, installedCategory, "default.py"] )

				percent = int( count * 100.0 / TOTAL_PATHS )
				self.dialogProgress.update(percent, actionMsg,"%s: %s" % (repo, installedCategory))
				if self.dialogProgress.iscanceled():
					quit = True
					break

				# download default.py
				doc = readURL( url.replace(' ','%20' ) )
				if doc == None:	# error
					quit = True
					break
				elif doc:
					# check remote file __version__ tag
					svn_ver = parseDocTag(doc, "version")
					if svn_ver:
						try:
							svn_xbmc_rev = int(parseDocTag( doc, "XBMC_Revision" ))
						except:
							svn_xbmc_rev = 0
						self.INSTALLED[count]['svn_ver'] = svn_ver
						self.INSTALLED[count]['svn_url'] = url.replace('/default.py','')
						self.INSTALLED[count]['XBMC_Revision'] = svn_xbmc_rev
						self.INSTALLED[count]['readme'] = check_readme( "/".join( [base_url, installedCategory] ) )
						self.INSTALLED[count]['repo'] = repo
						self.INSTALLED[count]['install'] = repo_install
						self.INSTALLED[count]['ioffset'] = repo_ioffset
						self.INSTALLED[count]['voffset'] = repo_voffset
						self.INSTALLED[count]['date'] = parseDocTag( doc, "date" )
						
						break # found in svn, move to next installed

			if quit: break

		log("< checkUpdates() updated count=%d" % len(self.INSTALLED))


	#####################################################################################################
	def showUpdates(self):
		log("> showUpdates()")
		ok = False

		# create update_all file that contains all listitem details
		deleteFile(self.UPDATE_ALL_FILENAME)
		updateAllItems = []

		# create display list
		sz = len(self.INSTALLED)
		log("showing INSTALLED count=%s" % sz)
		for info in self.INSTALLED:
			try:
				svn_url = info.get('svn_url','')
				svn_ver = info.get('svn_ver','')

				# ignore those not in SVN as per settings
				if (not svn_url or not svn_ver) and not self.showNoSVN:
					continue

				# get addon details
				path = ""
				filepath = info.get('filepath', '')
				ver = info.get('version', '')
				xbmc_rev = info.get('XBMC_Revision', 0)
				svn_xbmc_rev = info.get('XBMC_Revision',0)
				readme = info.get('readme','')
				category = info.get('category','')
				repo = info.get('repo','SVN ?')
				labelColour = "FFFFFFFF"

				# add ContextMenu: Delete (unless already deleted status)
				if "SVN Repo Installer" not in category:
					cm =  self._contextMenuItem( 30022, { "delete": filepath, "title": category } )	# 'Delete'
				else:
					cm = []

				# make update state according to found information
				if ".backups" in filepath:
					verState = xbmc.getLocalizedString( 30018 )				# Deleted
					labelColour = "66FFFFFF"
				elif not svn_url:
					verState = xbmc.getLocalizedString( 30012 )             # not in SVN
				elif not svn_ver:
					verState = xbmc.getLocalizedString( 30013 )				# unknown version
				elif ver >= svn_ver:
					verState = xbmc.getLocalizedString( 30011 )				# OK
					url_args = "show_info=%s" % urllib.quote_plus( repr(filepath) )
					path = '%s?%s' % ( sys.argv[ 0 ], url_args, )
				elif (svn_xbmc_rev and self.XBMC_REVISION and self.XBMC_REVISION >= svn_xbmc_rev) or \
					(not svn_xbmc_rev or not self.XBMC_REVISION):
					# Compatible, NEW AVAILABLE - setup callback url for plugin SVN Repo Installer
					verState = "v%s (%s)" % ( svn_ver, xbmc.getLocalizedString( 30014 ) )        # eg. !New! v1.1
					trunk_url = re.search('(/(?:trunk|branch|tag).*?)$', svn_url, re.IGNORECASE).group(1)
#['plugin://programs/SVN Repo Installer/', '-1', '?download_url="%2Ftrunk%2Fplugins%2Fmusic/iTunes%2F"&repo=\'xbmc-addons\'&install=""&ioffset=2&voffset=0']
					info['download_url'] = "download_url=%s&repo=%s&install=%s&ioffset=%s&voffset=%s" % \
								(repr(urllib.quote_plus(trunk_url + "/")),
								repr(urllib.quote_plus(repo)),
								repr(info["install"]),
								info["ioffset"],
								info["voffset"],)

					url_args = "show_info=%s" % urllib.quote_plus( repr(filepath) )

					# exclude self update from "update all"
					if "SVN Repo Installer" not in category:
						updateAllItems.append("?" + info['download_url'])
					path = '%s?%s' % ( sys.argv[ 0 ], url_args, )
				else:
					verState = "v%s (%s)" % ( svn_ver, xbmc.getLocalizedString( 30015 ), )	# eg. Incompatible

				if not path:
					path = os.path.join(filepath, 'default.tbn')
				if not svn_ver:
					svn_ver = '?'
				if not ver:
					ver = '?'

				# Addon status text as label2
				text = "[COLOR=%s][%s] %s (v%s)[/COLOR]" % (labelColour, repo, category, ver)
				label2 = makeLabel2( verState )

				# determine default icon according to addon type
				icon = ""
				if find(filepath,'scripts') != -1:
					icon = "DefaultScriptBig.png"
				elif find(filepath,'programs') != -1:
					icon = "DefaultProgramBig.png"
				elif find(filepath,'music') != -1:
					icon = "defaultAudioBig.png"
				elif find(filepath,'pictures') != -1:
					icon = "defaultPictureBig.png"
				elif find(filepath,'video') != -1:
					icon = "defaultVideoBig.png"
				# check skin for image, else fallback DefaultFile
				if not icon or not xbmc.skinHasImage(icon):
					icon = "DefaultFile.png"

				# assign thumb, in order from: local, svn, icon
				thumb = info.get('thumb','')
				if not thumb:
					thumb = icon

				li=xbmcgui.ListItem( text, label2, icon, thumb)
				li.setInfo( type="Video", infoLabels={ "Title": text, "Genre": label2 } )

				# add ContextMenu: Changelog
				cm +=  self._contextMenuItem( 30600, { "showlog": True, "repo": repo, "category": category.split( "/" )[ -1 ], "revision": None, "parse": True } )		# view readme
				# add ContextMenu: Readme
				if readme:
					cm +=  self._contextMenuItem( 30610, { "showreadme": True, "repo": None, "readme": readme } )

				li.addContextMenuItems( cm, replaceItems=True )
				ok = xbmcplugin.addDirectoryItem( handle=int( sys.argv[ 1 ] ), url=path, listitem=li, isFolder=False, totalItems=sz )
				if ( not ok ): break
			except:
				# user cancelled dialog or an error occurred
				logError()
				print info

		# if New Updates; add Update All item
		log("Updated Count=%d" % len(updateAllItems))
		if updateAllItems:
			icon = "DefaultFile.png"
			text = xbmc.getLocalizedString( 30019 )
			li=xbmcgui.ListItem( text, "", icon, icon)
			li.setInfo( type="Video", infoLabels={ "Title": text, "Genre": "" } )
			path = '%s?download_url=update_all' % ( sys.argv[ 0 ], )
			xbmcplugin.addDirectoryItem( handle=int( sys.argv[ 1 ] ), url=path, listitem=li, isFolder=False, totalItems=sz )
			# save update_all dict to file
			saveFileObj(self.UPDATE_ALL_FILENAME, updateAllItems)

		# add list sort methods
		xbmcplugin.addSortMethod( handle=int( sys.argv[ 1 ] ), sortMethod=xbmcplugin.SORT_METHOD_GENRE )
#            xbmcplugin.setContent( handle=int( sys.argv[ 1 ] ), content="files")
		ok = True

		log("< showUpdates() ok=%s" % ok)
		return ok

	#####################################################################################################
	def _contextMenuItem(self, stringId, args={}, runType="RunPlugin"):
		""" create a tuple suitable for adding to a list of Context Menu items """
		if isinstance(stringId, int):
			title = xbmc.getLocalizedString(stringId)
		else:
			title = stringId

		if args:
			argsStr = ""
			for key, value in args.items():
				argsStr += "&%s=%s" % ( key, urllib.quote_plus( repr(value) ) )
			runStr = ("XBMC.%s(%s?%s)" % ( runType, sys.argv[ 0 ], argsStr )).replace('?&','?')
		else:
			runStr = "XBMC.%s(%s)" % ( runType, sys.argv[ 0 ] )
		return [ (title, runStr), ]

	def show_info( self ):
		log("show_info()")
		

if ( __name__ == "__main__" ):
	try:
		Main()
	except:
		handleException("Main()")

