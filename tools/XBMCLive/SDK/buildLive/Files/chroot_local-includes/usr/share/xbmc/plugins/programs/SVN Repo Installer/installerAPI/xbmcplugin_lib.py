"""
 Common functions
"""

import os,sys,re
import xbmc
from urllib import unquote_plus, urlopen

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '24-06-2009'

#################################################################################################################
def log(msg):
	xbmc.log("[%s]: %s" % (__plugin__, msg), xbmc.LOGDEBUG)

log("Module: %s Dated: %s loaded!" % (__name__, __date__))

#################################################################################################################
def logError():
	log("ERROR: %s (%d) - %s" % (sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], ) )
	
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
		log( "Info() dict=%s" % self.__dict__ )
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

#################################################################################################################
def readURL( url ):
	log("readURL() url=" + url)
	if not url:
		return ""
	try:
		sock = urlopen( url )
		doc = sock.read()
		sock.close()
		if ( "404 Not Found" in doc ):
			log("readURL() 404, Not found")
			doc = ""
		return doc
	except:
		log("readURL() %s" % sys.exc_info()[ 1 ])
		return None

#################################################################################################################
def deleteFile( fn ):
	try:
		os.remove( fn )
		log("deleteFile() deleted: " + fn)
	except: pass

#####################################################################################################
def get_repo_info( repo ):
	# path to info file
	repopath = os.path.join( os.getcwd(), "resources", "repositories", repo, "repo.xml" )
	try:
		info = open( repopath, "r" ).read()
		# repo's base url
		REPO_URL = re.findall( '<url>([^<]+)</url>', info )[ 0 ]
		# root of repository
		REPO_ROOT = re.findall( '<root>([^<]*)</root>', info )[ 0 ]
		# structure of repo
		REPO_STRUCTURES = re.findall( '<structure name="([^"]+)" noffset="([^"]+)" install="([^"]*)" ioffset="([^"]+)" voffset="([^"]+)"', info )
		log("get_repo_info() REPO_URL=%s REPO_ROOT=%s REPO_STRUCTURES=%s" % (REPO_URL, REPO_ROOT, REPO_STRUCTURES))
		return ( REPO_URL, REPO_ROOT, REPO_STRUCTURES, )
	except:
		logError("get_repo_info()")
		return None

#####################################################################################################
def load_repos( ):
	repo_list = []
	repos = os.listdir( os.path.join( os.getcwd(), "resources", "repositories" ) )
	for repo in repos:
		if ("(tagged)" not in repo and repo != ".svn") and (os.path.isdir( os.path.join( os.getcwd(), "resources", "repositories", repo ) ) ):
			repo_list.append( repo )
	log("load_repos() %s" % repo_list)
	return repo_list

#####################################################################################################
def check_readme( base_url ):
	# try to get readme from: language, resources, root
	urlList = ( "/".join( [base_url, "resources", "language", xbmc.getLanguage(), "readme.txt"] ),
				"/".join( [base_url, "resources", "readme.txt" ] ),
				"/".join( [base_url, "readme.txt" ] ) )

	for url in urlList:
		url = url.replace(' ','%20')
		doc = readURL( url )
		if doc == None:
			break
		elif doc:
			return url
	return ""

#################################################################################################################
def get_xbmc_revision():
    try:
        rev = int(re.search("r([0-9]+)",  xbmc.getInfoLabel( "System.BuildVersion" ), re.IGNORECASE).group(1))
    except:
        rev = 0
    log("get_xbmc_revision() %d" % rev)
    return rev

#####################################################################################################
def parseDocTag(doc, tag):
	try:
		match = re.search("__%s__.*?[\"'](.*?)[\"']" % tag,  doc, re.IGNORECASE).group(1)
		match = match.replace( "$", "" ).replace( "Revision", "" ).replace( "Date", "" ).replace( ":", "" ).strip()
	except:
		match = ""
	log("parseDocTag() %s=%s" % (tag, match))
	return match

#####################################################################################################
def parseAllDocTags( doc ):
	tagInfo = {}
	# strings
	for tag in ( "author", "version", "date", ):
		tagInfo[tag] = parseDocTag( doc, tag )

	# title
	title = parseDocTag( doc, "plugin" )
	if not title:
		title = parseDocTag( doc, "scriptname" )
	tagInfo['title'] = title

	# ints
	try:
		value = int(parseDocTag( doc, "XBMC_Revision" ))
	except:
		value = 0
	tagInfo["XBMC_Revision"] = value

	# svn_revision
	try:
		value = int(re.search("\$Revision: (\d+)", doc).group(1))
	except:
		value = 0
	tagInfo["svn_revision"] = value

	log("parseAllDocTags() tagInfo=%s" % tagInfo)
	return tagInfo

#####################################################################################################
def makeLabel2( verState ):
	if xbmc.getLocalizedString( 30014 ) in verState:						# New
		label2 = "[COLOR=FF00FFFF]%s[/COLOR]" % verState
	elif xbmc.getLocalizedString( 30015 ) in verState:						# Incompatible
		label2 = "[COLOR=FFFF0000]%s[/COLOR]" % verState
	elif verState == xbmc.getLocalizedString( 30011 ):						# OK
		label2 = "[COLOR=FF00FF00]%s[/COLOR]" % verState
	elif verState == xbmc.getLocalizedString( 30018 ):						# Deleted
		label2 = "[COLOR=66FFFFFF]%s[/COLOR]" % verStat
	else:
		label2 = "[COLOR=FFFFFF00]%s[/COLOR]" % verState
	return label2

#####################################################################################################
def parseCategory(filepath):
    try:
        cat = re.search("(plugins.*|scripts.*)$",  filepath, re.IGNORECASE).group(1)
        cat = cat.replace("\\", "/")
    except:
        cat = ""
    log("parseCategory() cat=%s" % cat)
    return cat

