"""
	SVN Repo Installer Log Viewer
"""

import os
import sys
try:
	import xbmcgui
	import xbmc
	DEBUG = False
except:
	DEBUG = True
	
import urllib
import re
from xml.sax.saxutils import unescape

def log(msg):
	xbmc.log("[%s]: %s" % (__plugin__, msg), xbmc.LOGDEBUG)

class _Info:
	def __init__( self, *args, **kwargs ):
		self.__dict__.update( kwargs )

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '14-09-2009'
log( "Module: %s Dated: %s loaded!" % (__name__, __date__ ) )

class ChangelogParser:
	# TODO: make these settings
	BASE_URL = "http://code.google.com/p/%s/updates/list?start=%d"
	PAGES = 3

	def __init__( self, repo, category=None, revision=None, parse=True ):
		log( "%s __init__!" % (self.__class__))
		if ( DEBUG ):
			self.log = "[B]%s: %s[/B]\n----\n" % ( category or repo, "ChangeLog" )
		else:
			self.log = "[B]%s: %s[/B]\n----\n" % ( category or repo, xbmc.getLocalizedString( 30017 ) )
		self.repo = repo
		self.category = category
		self.revision = revision
		self.parse = parse

	def fetch_changelog( self ):
		try:
			if ( DEBUG ):
				base_path = os.getcwd()
			else:
				base_path = os.path.join( xbmc.translatePath( "special://profile/" ), "plugin_data", "programs", os.path.basename( os.getcwd() ) )
			# make path
			if ( not os.path.isdir( base_path ) ):
				os.makedirs( base_path )
			for page in range( self.PAGES ):
				path = os.path.join( base_path, "%s%d.txt" % ( self.repo, page, ) )
				# open socket
				if ( os.path.isfile( path ) and self.revision is None):
					log( "%s path=%s" % (self.__class__.__name__, path) )
					usock = open( path, "r" )
				else:
					url = self.BASE_URL % ( self.repo, page * 50, )
					log( "%s url=%s" % (self.__class__.__name__, url) )
					usock = urllib.urlopen( url )
				#read html source
				htmlSource = usock.read()
				# close socket
				usock.close()
				# save source for debugging
				if ( self.parse == False or not os.path.isfile( path ) ):
					file_object = open( path, "w" )
					file_object.write( htmlSource )
					file_object.close()
				# parse source
				if ( self.parse ):
					self._parse_html_source( htmlSource )
		except Exception, e:
			print str( e )
			self.log = str(e)
			return ""
		else:
			return self.log

	def _parse_html_source( self, htmlSource ):
		# regex's
		regex_items = re.compile( '<li>(.+?)</li>', re.DOTALL )
		regex_revisions = re.compile( '<a class="ot-revision-link" href="/p/[^/]+/source/detail\?r=[0-9]+">([^<]+)</a>' )
		regex_dates = re.compile( '<span class="date below-more" title="([^"]+)"' )
		regex_authors = re.compile( '<a class="ot-profile-link-2" href="/u/[^/]+/">([^<]+)</a></span>' )
		regex_details = re.compile( '<div class=\"details\"><span class=\"ot-logmessage\">(.+?)</span></div>', re.DOTALL )
		if ( self.category is not None ):
			regex_subst = re.compile( "\[%s\]" % self.category, re.IGNORECASE )
		# scrape info
		items = regex_items.findall( htmlSource )
		# enumerate thru and scrape and combine all info
		for item in items:
			try:
				# scrape info
				revision = regex_revisions.findall( item )[ 0 ]
				date = regex_dates.findall( item )[ 0 ]
				author = regex_authors.findall( item )[ 0 ]
				detail = regex_details.findall( item )[ 0 ]
				# add to log
				if ( self.category is not None and re.findall( "\[.*%s.*\]" % self.category, detail, re.IGNORECASE ) ):
					if ( self.revision is not None and int( revision[ 1 : ] ) <= self.revision ):
						self.log += "[I]%s - %s - %s[/I]\n%s\n----\n" % ( revision, date, author, unescape( re.sub( regex_subst, "", detail ).strip(), { "&#39;": "'", "&quot;": '"' } ), )
					elif ( self.revision is None ):
						self.log += "[I]%s - %s - %s[/I]\n%s\n----\n" % ( revision, date, author, unescape( re.sub( regex_subst, "", detail ).strip(), { "&#39;": "'", "&quot;": '"' } ), )
				elif ( self.category is None ):
					pos = ( detail.find( "]" ) )
					if ( pos >= 0 ):
						msg = unescape( "%s [I]%s - %s - %s[/I]\n%s\n----\n" % ( detail[ : pos + 1 ].strip(), revision, date, author, detail[ pos + 1 : ].strip() ), { "&#39;": "'", "&quot;": '"' } )
					else:
						msg = unescape( "[I]%s - %s - %s[/I]\n%s\n----\n" % ( revision, date, author, detail.strip(), ), { "&#39;": "'", "&quot;": '"' } )
					self.log += msg
			except:
				# not a valid log message
				pass

if ( not DEBUG ):
	class GUI( xbmcgui.WindowXMLDialog ):
		# action codes
		ACTION_EXIT_SCRIPT = ( 9, 10, )

		def __init__( self, *args, **kwargs ):
			log( "%s __init__!" % (self.__class__) )
			xbmcgui.WindowXMLDialog.__init__( self )
			self._parse_argv()

		def onInit( self ):
			self.dialog = xbmcgui.DialogProgress()
			self.dialog.create( __plugin__, xbmc.getLocalizedString( 30001 ) )
			if ( self.args.repo is None ):
				log = self._fetch_readme()
			else:
				log = self._fetch_changelog()
			self.dialog.close()
			self._paste_log( log )

		def _parse_argv( self ):
			# call _Info() with our formatted argv to create the self.args object
			exec "self.args = _Info(%s)" % ( urllib.unquote_plus( sys.argv[ 2 ][ 1 : ].replace( "&", ", " ) ), )

		def _fetch_changelog( self ):
			if ( self.args.revision == True ):
				try:
					self.args.revision = int( re.findall( "\$Revision: ([0-9]+) \$", sys.modules[ "__main__" ].__svn_revision__ )[ 0 ] )
				except:
					self.args.revision = None
			parser = ChangelogParser( self.args.repo, self.args.category, self.args.revision, self.args.parse )
			return parser.fetch_changelog()

		def getReadmePath(self):
			base_path = os.path.join( os.getcwd(), "resources", "language" )
			path = os.path.join( base_path, xbmc.getLanguage(), "readme.txt" )
			if not os.path.isfile(path):
				path = os.path.join( base_path, "English", "readme.txt" )
				if not os.path.isfile(path):
					path = os.path.join( os.getcwd(), "resources", "readme.txt" )
					if not os.path.isfile(path):
						path = os.path.join( os.getcwd(), "readme.txt" )
						if not os.path.isfile(path):
							path = None
			return path

		def _fetch_readme( self ):
			try:
				self.args.category = "readme.txt"
				usock = None
				readme = None
				if ( self.args.readme is None ):
					# local readme - determine correct language path
					path = self.getReadmePath()
					if path:
						# open socket
						log("%s path=%s" % (self.__class__.__name__, path) )
						usock = open( path, "r" )
				else:
					log("%s url=%s" % (self.__class__.__name__, self.args.readme) )
					usock = urllib.urlopen( self.args.readme )

				#read html source
				if usock:
					readme = usock.read()
					# close socket
					usock.close()

				if not usock or not readme or "404 Not Found" in readme:
					readme = "Readme not found."

			except Exception, e:
				print str(e)
				readme = str(e)
			return readme

		def _paste_log( self, log ):
			try:
				title = ( self.args.category or self.args.repo )
				self.getControl( 5 ).setText( log )
				self.getControl( 3 ).setText( title )
			except:
				pass

		def onClick( self, controlId ):
			pass

		def onFocus( self, controlId ):
			pass

		def onAction( self, action ):
			if ( action in self.ACTION_EXIT_SCRIPT ):
				self.close()

def Main():
	ui = GUI( "DialogScriptInfo.xml", os.getcwd(), "Default" )
	ui.doModal()
	del ui

if ( __name__ == "__main__" ):
	print "is logviewer __main__"
	if ( not DEBUG ):
		Main()
	else:
		parser = ChangelogParser( "xbmc-addons", "SVN Repo Installer" )
		parser.fetch_changelog()
		print parser.log

