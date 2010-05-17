import os, sys
import xbmc, xbmcgui
import urllib
from pprint import pprint
from xbmcplugin_lib import *
from shutil import rmtree, copytree

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '23-06-2009'
log("Module: %s Dated: %s loaded!" % (__name__, __date__))

#################################################################################################################
class InfoDialog( xbmcgui.WindowXMLDialog ):
	""" Show skinned Dialog with our information """

	XML_FILENAME = "script-svnri-iteminfo.xml"
	EXIT_CODES = (9, 10, 216, 257, 275, 216, 61506, 61467,)

	def __init__( self, *args, **kwargs):
		log( "%s init!" % self.__class__ )
		self.action = None
		self.buttons = {}

	def onInit( self ):
		xbmcgui.lock()

		thumb = xbmc.translatePath(self.info.get('thumb', ''))
		if thumb:
			self.getControl( 31 ).setImage( thumb )

		self.getControl( 4 ).setLabel("[B]"+self.info.get('title','?')+"[/B]")
		self.getControl( 6 ).setLabel(self.info.get('author','?'))
		version = self.info.get('version','?')
		if not version: version = '?'
		svn_ver = self.info.get('svn_ver','?')
		if not svn_ver: svn_ver = '?'
		verText = "v%s -> v%s" % (version,svn_ver)
		self.getControl( 8 ).setLabel(verText)
		self.getControl( 10 ).setLabel(self.info.get('date','?'))
		self.getControl( 12 ).setLabel(self.info.get('category','?'))
		self.getControl( 19 ).setLabel(self.info.get('compatibility',''))
		if self.info.get('changelog',''):
			self.showChangelog()
		elif self.info.get('readme',''):
			self.showReadme()

		# set btns enabled state
		btnIDs = {'install': 20,'uninstall': 21,'readme': 22,'changelog': 23 }
		for btnName, isEnabled in self.buttons.items():
			id = btnIDs[btnName]
			self.getControl( id ).setEnabled( isEnabled )

		xbmcgui.unlock()

	def showChangelog(self):
		self.getControl( 30 ).setText(self.info.get('changelog',''))
	def showReadme(self):
		self.getControl( 30 ).setText(self.info.get('readme',''))

	def onClick( self, controlId ):
		if controlId in (20,21,22,23,24):
			if controlId == 22:
				self.showReadme()
			elif controlId == 23:
				self.showChangelog()
			elif controlId == 20:
				self.action = "install"
			elif controlId == 21:
				self.action = "uninstall"
			else:
				self.action = None

			if controlId not in (22,23):
				self.close()

	def onFocus( self, controlId ):
		pass

	def onAction( self, action ):
		try:
			buttonCode =  action.getButtonCode()
			actionID   =  action.getId()
		except: return
		if actionID in self.EXIT_CODES or buttonCode in self.EXIT_CODES:
			self.close()

	def ask(self, info, buttons ):
#		pprint (info)
		if info:
			self.info = info
			self.buttons = buttons
			self.doModal()
		log("ask() action=%s" % self.action)
		return self.action

########################################################################################################################
class Main:

	INSTALLED_ITEMS_FILENAME = os.path.join( os.getcwd(), "installed_items.dat" )

	def __init__( self, *args, **kwargs):
		log( "%s started!" % self.__class__ )
		try:
			self._parse_argv()
			if self.args.has_key("show_info"):
				info = self._load_item()
				if info:
					self.show_info( info )
		except Exception, e:
			xbmcgui.Dialog().ok(__plugin__ + " ERROR!", str(e))

	########################################################################################################################
	def _parse_argv(self):
		# call Info() with our formatted argv to create the self.args object
		exec "self.args = Info(%s)" % ( unquote_plus( sys.argv[ 2 ][ 1 : ].replace( "&", ", " ) ), )

	########################################################################################################################
	def _load_item( self ):
		info = {}
		items = loadFileObj(self.INSTALLED_ITEMS_FILENAME)
		filepath = self.args.show_info
		log("looking for filepath=%s" % filepath)
		# find addon from installed list
		for i, item in enumerate(items):
			if item.get('filepath','') == filepath:
				info = item
				break
		log("_load_item() info=%s" % info)
		return info

	########################################################################################################################
	def show_info( self, info ):
		log("> show_info() ")
		pprint (info)
		quit = False
		buttons = {'install': True,'uninstall': True, 'readme': True, 'changelog': True }

		# fetch changelog
		dialog = xbmcgui.DialogProgress()
		dialog.create( __plugin__, xbmc.getLocalizedString( 30001 ),  xbmc.getLocalizedString( 30017 ))
		from xbmcplugin_logviewer import ChangelogParser
		parser = ChangelogParser( info['repo'] , info['title']  )
		info['changelog'] = parser.fetch_changelog()

		# fetch readme
		readme_url = info.get('readme','')
		if readme_url.startswith('http'):
			dialog.update( 50, xbmc.getLocalizedString( 30001 ),  os.path.basename(readme_url))
			info['readme'] = readURL(readme_url)
		dialog.close()
		if not readme_url or not info.get('readme',''):
			buttons['readme'] = False

		# setup compatibility text
		svn_xbmc_rev = info.get('XBMC_Revision',0)
		info['compatibility'] = "XBMC: %s - " % xbmc.getInfoLabel( "System.BuildVersion" )
		if svn_xbmc_rev and svn_xbmc_rev > get_xbmc_revision():
			info['compatibility'] += "[COLOR=FFFF0000]%s[/COLOR] - Requires XBMC: r%s" % (xbmc.getLocalizedString( 30015 ), svn_xbmc_rev) # incomp
		else:
			info['compatibility'] += "[COLOR=FF00FF00]%s[/COLOR]" % (xbmc.getLocalizedString( 30704 ))		# ok, comp

		# set INSTALL button
		if not info.get('download_url',''):
			buttons['install'] = False

		# set UNINSTALL button
		if not os.path.exists(info.get('filepath','')) or "SVN Repo Installer" in info['title']:
			buttons['uninstall'] = False

		while info:
			action = InfoDialog( InfoDialog.XML_FILENAME, os.getcwd(), "Default" ).ask( info, buttons )
			if not action:
				break
			elif action == "install":
				# install
				url_args = info['download_url']
				if "SVN Repo Installer" in info['title']:
					url_args = "self_update=True&" + url_args
				path = '%s?%s' % ( sys.argv[ 0 ], url_args, )
				command = 'XBMC.RunPlugin(%s)' % path
				xbmc.executebuiltin(command)
				break
			elif action == "uninstall":
				path = '%s?delete=%s&title=%s' % ( sys.argv[ 0 ], urllib.quote_plus( repr(info['filepath']) ), urllib.quote_plus( repr(info['title']) ),)
				command = 'XBMC.RunPlugin(%s)' % path
				xbmc.executebuiltin(command)
				break

		log("< show_info()")
