"""
Update module

Nuka1195
"""

# main imports
import sys
import os
import xbmc
import xbmcgui
import xbmcplugin
import urllib
import re
from xml.sax.saxutils import unescape
from xbmcplugin_lib import *

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '19-06-2009'
log("Module: %s Dated: %s loaded!" % (__name__, __date__))

class Parser:
	""" Parser Class: grabs all tag versions and urls """
	# regexpressions
	revision_regex = re.compile( '<h2>.+?Revision ([0-9]*): ([^<]*)</h2>' )
	asset_regex = re.compile( '<li><a href="([^"]*)">([^"]*)</a></li>' )

	def __init__( self, htmlSource ):
		# set our initial status
		self.dict = { "status": "fail", "revision": 0, "assets": [], "url": "" }
		# fetch revision number
		self._fetch_revision( htmlSource )
		# if we were successful, fetch assets
		if ( self.dict[ "revision" ] != 0 ):
			self._fetch_assets( htmlSource )

	def _fetch_revision( self, htmlSource ):
		try:
			# parse revision and current dir level
			revision, url = self.revision_regex.findall( htmlSource )[ 0 ]
			# we succeeded :), set our info
			self.dict[ "url" ] = url
			self.dict[ "revision" ] = int( revision )
		except:
			pass

	def _fetch_assets( self, htmlSource ):
		try:
			assets = self.asset_regex.findall( htmlSource )
			if ( len( assets ) ):
				for asset in assets:
					if ( asset[ 0 ] != "../" ):
						self.dict[ "assets" ] += [ unescape( asset[ 0 ] ) ]
				self.dict[ "status" ] = "ok"
		except:
			pass

class Main:
	def __init__( self ):
		log( "%s started!" % self.__class__ )

		self.dialog = xbmcgui.DialogProgress()

		if "update_all" in sys.argv[ 2 ]:
			self.update_all()
		else:
			# parse sys.argv for our current url
			self._parse_argv()
			# get the repository info
			self._get_repo_info()
			self._create_title()
			# get the list
			self._download_item()

	def _get_repo_info( self ):
		# path to info file
		repopath = os.path.join( os.getcwd(), "resources", "repositories", self.args.repo, "repo.xml" )
		try:
			# grab a file object
			fileobject = open( repopath, "r" )
			# read the info
			info = fileobject.read()
			# close the file object
			fileobject.close()
			# repo's base url
			self.REPO_URL = re.findall( '<url>([^<]+)</url>', info )[ 0 ]
			log("_get_repo_info() repo_url=%s" % self.REPO_URL)
		except:
			# oops print error message
			print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )

	def _parse_argv( self ):
		# call _Info() with our formatted argv to create the self.args object
		exec "self.args = Info(%s)" % ( sys.argv[ 2 ][ 1 : ].replace( "&", ", " ), )
		self.args.download_url = urllib.unquote_plus( self.args.download_url )
		self.args.repo = urllib.unquote_plus( self.args.repo )

	def _create_title( self ):
		# create the script/plugin/skin title
		parts = self.args.download_url.split( "/" )
		version = ""
		if ( self.args.voffset != 0 ):
			version = " - %s" % ( parts[ self.args.voffset ].replace( "%20", " " ) )
			del parts[ self.args.voffset ]
		self.title = parts[ -2 ].replace( "%20", " " ) + version
		log("_create_title() %s" % self.title)

	def update_all( self ):
		log("> _update_all()")
		""" Download and install all new Addons as stored in update file """
		# eg 'download_url="%2Ftrunk%2Fplugins%2Fmusic/iTunes%2F"&repo=\'xbmc-addons\'&install=""&ioffset=2&voffset=0'
		fn = os.path.join( os.getcwd(), "update_all.dat" )
		items = loadFileObj(fn)
		if items:
			if xbmcgui.Dialog().yesno( __plugin__, xbmc.getLocalizedString( 30019 ) + " ?", "", "", xbmc.getLocalizedString( 30020 ), xbmc.getLocalizedString( 30021 ) ):
				# enumerate throu item to download & install
				sz = len(items)
				for url_args in items:
					log("_update_all() updating: %s" % url_args)
					# load item args
					sys.argv[ 2 ] = url_args
					# parse sys.argv for our current url
					self._parse_argv()
					# get the repository info
					self._get_repo_info()
					# create title
					self._create_title()
					# download & install
					self._download_item( forceInstall=True )

				self.dialog.close()
				# force list refresh
#				xbmc.executebuiltin('Container.Refresh')
		else:
			xbmcgui.Dialog().ok("update_all()", "'Update All' file missing/empty!", os.path.split(fn)[0], os.path.basename(fn))
		log("< _update_all()")

	def _download_item( self, forceInstall=False ):
		log("> _download_item() forceInstall=%s" % forceInstall)
		try:
			if ( forceInstall or xbmcgui.Dialog().yesno( self.title, xbmc.getLocalizedString( 30000 ), "", "", xbmc.getLocalizedString( 30020 ), xbmc.getLocalizedString( 30021 ) ) ):
				self.dialog.create( self.title, xbmc.getLocalizedString( 30002 ), xbmc.getLocalizedString( 30003 ) )
				asset_files = []
				folders = [ self.args.download_url.replace( " ", "%20" ) ]
				while folders:
					try:
						htmlsource = readURL( self.REPO_URL + folders[ 0 ] )
						if ( not htmlsource ): raise
						items = self._parse_html_source( htmlsource )
						if ( not items or items[ "status" ] == "fail" ): raise
						files, dirs = self._parse_items( items )
						for file in files:
							asset_files.append( "%s/%s" % ( items[ "url" ], file, ) )
						for folder in dirs:
							folders.append( folders[ 0 ] + folder )
						folders = folders[ 1 : ]
					except:
						folders = []

				finished_path = self._get_files( asset_files )
				self.dialog.close()
				if finished_path and not forceInstall:
					xbmcgui.Dialog().ok( self.title, xbmc.getLocalizedString( 30008 ), finished_path )
					# force list refresh
#					xbmc.executebuiltin('Container.Refresh')
		except:
			# oops print error message
			print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
			self.dialog.close()
			xbmcgui.Dialog().ok( self.title, xbmc.getLocalizedString( 30030 ) )
		log("< _download_item()")
		
	def _get_files( self, asset_files ):
		""" fetch the files """
		try:
			finished_path = ""
			for cnt, url in enumerate( asset_files ):
				items = os.path.split( url )
				# base path
				drive = xbmc.translatePath( "/".join( [ "special://home", self.args.install ] ) )
				# create the script/plugin/skin title
				parts = items[ 0 ].split( "/" )
				version = ""
				if ( self.args.voffset != 0 ):
					version = " - %s" % ( parts[ self.args.voffset ], )
					del parts[ self.args.voffset ]
					parts[ self.args.voffset - 1 ] = parts[ self.args.voffset - 1 ].replace( "%20", " " ) + version
				path = os.path.join( drive, os.path.sep.join( parts[ self.args.ioffset : ] ).replace( "%20", " " ) )
				if ( not finished_path ): finished_path = path
				file = items[ 1 ].replace( "%20", " " )
				pct = int( ( float( cnt ) / len( asset_files ) ) * 100 )
				self.dialog.update( pct, "%s %s" % ( xbmc.getLocalizedString( 30005 ), url, ), "%s %s" % ( xbmc.getLocalizedString( 30006 ), path, ), "%s %s" % ( xbmc.getLocalizedString( 30007 ), file, ) )
				if ( self.dialog.iscanceled() ): raise
				if ( not os.path.isdir( path ) ): os.makedirs( path )
				url = self.REPO_URL + url.replace( " ", "%20" )
				fpath = os.path.join( path, file )
				log("urlretrieve() %s -> %s" % (url, fpath))
				urllib.urlretrieve( url, fpath )
		except:
			finished_path = ""
			# oops print error message
			print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
			raise

		log("_get_files() finished_path=%s" % finished_path)
		return finished_path
			
	def _parse_html_source( self, htmlsource ):
		""" parse html source for tagged version and url """
		try:
			parser = Parser( htmlsource )
			return parser.dict
		except:
			return {}
			
	def _parse_items( self, items ):
		""" separates files and folders """
		folders = []
		files = []
		for item in items[ "assets" ]:
			if ( item.endswith( "/" ) ):
				folders.append( item )
			else:
				files.append( item )
		return files, folders
