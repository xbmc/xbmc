"""
svn repo installer plugin

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
xbmc.log("[PLUGIN] Module: %s loaded!" % __name__, xbmc.LOGDEBUG)


class Parser:
    """ Parser Class: grabs all tag versions and urls """
    # regexpressions
    revision_regex = re.compile( '<h2>.+?Revision ([0-9]*): ([^<]*)</h2>' )
    asset_regex = re.compile( '<li><a href="([^"]*)">([^"]*)</a></li>' )

    def __init__( self, htmlSource ):
        xbmc.log("[PLUGIN] %s __init__!" % (self.__class__), xbmc.LOGDEBUG)
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


class _Info:
    def __init__( self, *args, **kwargs ):
        self.__dict__.update( kwargs )


class Main:
    # base path
    BASE_CACHE_PATH = os.path.join( xbmc.translatePath( "special://profile/" ), "Thumbnails", "Pictures" )

    def __init__( self ):
        xbmc.log("[PLUGIN] %s __init__!" % (self.__class__), xbmc.LOGDEBUG)
        # parse sys.argv for our current url
        self._parse_argv()
        # if this is first run list all the repos
        if ( sys.argv[ 2 ] == "" ):
            ok = self._get_repos()
        else:
            # get XBMC revision
            self._get_xbmc_revision()
            # get the repository info
            self._get_repo_info()
            # get the list
            ok = self._show_categories()
        # send notification we're finished, successfully or unsuccessfully
        xbmcplugin.endOfDirectory( handle=int( sys.argv[ 1 ] ), succeeded=ok )

    def _clear_log( self, repo ):
        base_path = os.path.join( xbmc.translatePath( "special://profile/" ), "plugin_data", "programs", os.path.basename( os.getcwd() ) )
        for page in range( 3 ):
            path = os.path.join( base_path, "%s%d.txt" % ( repo, page, ) )
            # remove log file
            if ( os.path.isfile( path ) ):
                os.remove( path )

    def _get_xbmc_revision( self ):
        try:
            self.XBMC_REVISION = re.search("r([0-9]+)",  xbmc.getInfoLabel( "System.BuildVersion" ), re.IGNORECASE).group(1)
        except:
            self.XBMC_REVISION = 0
        xbmc.log("XBMC_REVISION-%s" % self.XBMC_REVISION, xbmc.LOGDEBUG)

    def _parse_argv( self ):
        # if first run set title to blank
        if ( sys.argv[ 2 ] == "" ):
            self.args = _Info( title="" )
        else:
            # call _Info() with our formatted argv to create the self.args object
            exec "self.args = _Info(%s)" % ( urllib.unquote_plus( sys.argv[ 2 ][ 1 : ].replace( "&", ", " ) ), )

    def _get_repos( self ):
        try:
            # we fetch the log here only at start of plugin
            #import xbmcplugin_logviewer
            # add the check for updates item to the media list
            url = "%s?category='updates'" % ( sys.argv[ 0 ], )
            # set the default icon
            icon = "DefaultFolder.png"
            # set thumbnail
            thumbnail = os.path.join( os.getcwd(), "resources", "media", "update_checker.png" )
            # create our listitem, fixing title
            listitem = xbmcgui.ListItem( xbmc.getLocalizedString( 30500 ), iconImage=icon, thumbnailImage=thumbnail )
            # set the title
            listitem.setInfo( type="Video", infoLabels={ "Title": xbmc.getLocalizedString( 30500 ) } )
            cm = [ ( xbmc.getLocalizedString( 30610 ), "XBMC.RunPlugin(%s?showreadme=True&repo=None&readme=None)" % ( sys.argv[ 0 ], ), ) ]
            listitem.addContextMenuItems( cm, replaceItems=True )
            # add our item
            ok = xbmcplugin.addDirectoryItem( handle=int( sys.argv[ 1 ] ), url=url, listitem=listitem, isFolder=True )

            # now add all the repos
            repos = os.listdir( os.path.join( os.getcwd(), "resources", "repositories" ) )
            # enumerate through the list of categories and add the item to the media list
            for repo in repos:
                cm = []
                if ( os.path.isdir( os.path.join( os.getcwd(), "resources", "repositories", repo ) ) ):
                    # create the url
                    url = "%s?category='root'&repo=%s&title=%s" % ( sys.argv[ 0 ], repr( urllib.quote_plus( repo ) ), repr( urllib.quote_plus( repo ) ), )
                    # set thumbnail
                    thumbnail = os.path.join( os.getcwd(), "resources", "media", "svn_repo.png" )
                    # create our listitem, fixing title
                    listitem = xbmcgui.ListItem( repo, iconImage=icon, thumbnailImage=thumbnail )
                    # set the title
                    listitem.setInfo( type="Video", infoLabels={ "Title": repo } )
                    # grab the log for this repo
                    if ( "(tagged)" not in repo ):
                        #parser = xbmcplugin_logviewer.ChangelogParser( repo, parse=False )
                        #parser.fetch_changelog()
                        # clear logs on first run
                        self._clear_log( repo )
                        # add view log context menu item
                        cm += [ ( xbmc.getLocalizedString( 30600 ), "XBMC.RunPlugin(%s?showlog=True&repo=%s&category=None&revision=None&parse=True)" % ( sys.argv[ 0 ], urllib.quote_plus( repr( repo ) ), ), ) ]
                    # add view readme context menu item
                    cm += [ ( xbmc.getLocalizedString( 30610 ), "XBMC.RunPlugin(%s?showreadme=True&repo=None&readme=None)" % ( sys.argv[ 0 ], ), ) ]
                    # add context menu items
                    listitem.addContextMenuItems( cm, replaceItems=True )
                    # add the item to the media list
                    ok = xbmcplugin.addDirectoryItem( handle=int( sys.argv[ 1 ] ), url=url, listitem=listitem, isFolder=True )
                    # if user cancels, call raise to exit loop
                    if ( not ok ): raise
        except:
            # user cancelled dialog or an error occurred
            print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
            ok = False
        if ( ok ):
            xbmcplugin.addSortMethod( handle=int( sys.argv[ 1 ] ), sortMethod=xbmcplugin.SORT_METHOD_LABEL )
        return ok

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
            # root of repository
            self.REPO_ROOT = re.findall( '<root>([^<]*)</root>', info )[ 0 ]
            # structure of repo
            self.REPO_STRUCTURES = re.findall( '<structure name="([^"]+)" noffset="([^"]+)" install="([^"]*)" ioffset="([^"]+)" voffset="([^"]+)"', info )
            # if category is root, set our repo root
            if ( self.args.category == "root" ):
                self.args.category = self.REPO_ROOT
        except:
            # oops print error message
            print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )

    def _show_categories( self ):
        ok = False
        # fetch the html source
        items = self._get_items()
        # if successful
        if ( items and items[ "status" ] == "ok" ):
            # if there are assets, we have categories
            ok = self._fill_list( items[ "url" ], items[ "revision" ], items[ "assets" ] )
        return ok

    def _fill_list( self, repo_url, revision, assets ):
        try:
            ok = False
            # enumerate through the list of categories and add the item to the media list
            for item in assets:
                cm = []
                isFolder = True
                for name, noffset, install, ioffset, voffset in self.REPO_STRUCTURES:
                    try:
                        if ( repo_url.split( "/" )[ int( noffset ) ].lower() == name.lower() ):
                            isFolder = False
                            break
                    except:
                        pass
                if ( isFolder ):
                    heading = "category"
                    thumbnail = ""
                    label2 = ""
                    version = ""
                else:
                    # set special case if self updating
                    heading = "download_url"
                    thumbnail = "%s%s/%sdefault.tbn" % ( self.REPO_URL, repo_url.replace( " ", "%20" ), item.replace( " ", "%20" ), )
                    version, label2, path = self._check_compatible( "%s%s/%sdefault.py" % ( self.REPO_URL, repo_url.replace( " ", "%20" ), item.replace( " ", "%20" ), ), self.REPO_URL, install, int( ioffset ), int( voffset ) )
                    version = " (%s)" % version
                    readme = self._check_readme( "%s%s/%sresources/readme.txt" % ( self.REPO_URL, repo_url.replace( " ", "%20" ), item.replace( " ", "%20" ), ) )
                    
                if ( label2.startswith( "[COLOR=FF00FF00]" ) or label2.startswith( "[COLOR=FFFF0000]" ) ):
                    url = path
                elif "SVN%20Repo%20Installer" in item:
                    url = '%s?self_update=True&%s="%s/%s"&repo=%s&install="%s"&ioffset=%s&voffset=%s&title=%s' % ( sys.argv[ 0 ], heading, urllib.quote_plus( repo_url ), urllib.quote_plus( item ), repr( urllib.quote_plus( self.args.repo ) ), install, ioffset, voffset, repr( urllib.quote_plus( self.args.repo ) ), )
                else:
                    url = '%s?%s="%s/%s"&repo=%s&install="%s"&ioffset=%s&voffset=%s&title=%s' % ( sys.argv[ 0 ], heading, urllib.quote_plus( repo_url ), urllib.quote_plus( item ), repr( urllib.quote_plus( self.args.repo ) ), install, ioffset, voffset, repr( urllib.quote_plus( self.args.repo ) ), )
                # set the default icon
                icon = "DefaultFolder.png"
                # create our listitem, fixing title
                listitem = xbmcgui.ListItem( "%s%s" % ( urllib.unquote_plus( item[ : -1 ] ), version, ), label2=label2, iconImage=icon, thumbnailImage=thumbnail )
                # set the title
                listitem.setInfo( type="Video", infoLabels={ "Title": "%s%s" % ( urllib.unquote_plus( item[ : -1 ] ), version, ), "Genre": label2 } )
                if ( not isFolder ):
                    cm += [ ( xbmc.getLocalizedString( 30600 ), "XBMC.RunPlugin(%s?showlog=True&repo=%s&category=%s&revision=None&parse=True)" % ( sys.argv[ 0 ], urllib.quote_plus( repr( self.args.repo ) ), urllib.quote_plus( repr( item[ : -1 ].replace( "%20", " " )  )  ), ), ) ]
                    # add context menu items
                    if ( readme is not None ):
                        cm += [ ( xbmc.getLocalizedString( 30610 ), "XBMC.RunPlugin(%s?showreadme=True&repo=None&readme=%s)" % ( sys.argv[ 0 ], urllib.quote_plus( repr( readme ) ), ), ) ]
                listitem.addContextMenuItems( cm, replaceItems=True )
                # add the item to the media list
                ok = xbmcplugin.addDirectoryItem( handle=int( sys.argv[ 1 ] ), url=url, listitem=listitem, isFolder=isFolder, totalItems=len( assets ) )
                # if user cancels, call raise to exit loop
                if ( not ok ): raise
        except:
            # user cancelled dialog or an error occurred
            print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
            ok = False
        if ( ok ):
            # set our plugin category
            xbmcplugin.setPluginCategory( handle=int( sys.argv[ 1 ] ), category=self.args.title )
            # sort by genre so all update status' are grouped
            xbmcplugin.addSortMethod( handle=int( sys.argv[ 1 ] ), sortMethod=xbmcplugin.SORT_METHOD_GENRE )
        return ok

    def _check_readme( self, url ):
        xbmc.log("[PLUGIN] _check_readme() url=%s" % (url), xbmc.LOGDEBUG)
        try:
            # open url
            usock = urllib.urlopen( url )
            # read source
            htmlSource = usock.read()
            # close socket
            usock.close()
            # compatible
            if ( "404 Not Found" in htmlSource ):
                return None
            else:
                return url
        except:
            return None

    def _check_compatible( self, url, repo_url, install, ioffset, voffset ):
        xbmc.log("[PLUGIN] _check_compatible() url=%s" % (url), xbmc.LOGDEBUG)
        try:
            # get items svn info
            ok = True
            version = xbmc.getLocalizedString( 30013 )
            # open url
            usock = urllib.urlopen( url )
            # read source
            htmlSource = usock.read()
            # close socket
            usock.close()
            # parse source for revision and version
            version, revision = self._parse_version_revision( htmlSource )
            # compatible - 0 == unknown, so allow it
            ok = (self.XBMC_REVISION == 0 or self.XBMC_REVISION >= revision)
        except:
            pass

        # creat path
        items = url.replace( repo_url, "" ).split( "/" )
        # base path
        drive = xbmc.translatePath( "/".join( [ "special://home", install ] ) )
        if ( voffset != 0 ):
            items[ voffset - 1 ] = "%s - %s" % ( items[ voffset - 1 ].replace( "%20", " " ), items[ voffset ], )
            del items[ voffset ]
        path = os.path.join( drive, os.path.sep.join( items[ ioffset : ] ).replace( "%20", " " ) )
        # if compatible, check for existing install
        if ( not ok ):
            return version, "[COLOR=FFFF0000]%s[/COLOR]" % ( xbmc.getLocalizedString( 30015 ), ), path

        if ( os.path.isfile( path ) ):
            htmlSource = open( path, "r" ).read()
            # parse source for revision and version
            ver, revision = self._parse_version_revision( htmlSource )
            label2 = ( "[COLOR=FF00FFFF]%s[/COLOR]" % ( xbmc.getLocalizedString( 30016 ), ), "[COLOR=FF00FF00]%s[/COLOR]" % ( xbmc.getLocalizedString( 30011 ), ), )[ ver >= version ]
        else:
            label2 = "[COLOR=FF00FFFF]%s[/COLOR]" % ( xbmc.getLocalizedString( 30021 ), )
        return version, label2, path

    def _parse_version_revision( self, htmlSource ):
        try:
            revision = int( re.search( "__XBMC_Revision__.*?[\"'](.*?)[\"']",  htmlSource, re.IGNORECASE ).group(1) )
        except:
            revision = 0
        try:
            version = re.search( "__version__.*?[\"'](.*?)[\"']",  htmlSource, re.IGNORECASE ).group(1)
        except:
            version = ""
        return version, revision

    def _get_items( self ):
        try:
            # open url
            url = self.REPO_URL + self.args.category
            xbmc.log("[PLUGIN] _get_items() url=%s" % (url), xbmc.LOGDEBUG)
            usock = urllib.urlopen( url )
            # read source
            htmlSource = usock.read()
            # close socket
            usock.close()
            # parse source and return a dictionary
            return self._parse_html_source( htmlSource )
        except:
            # oops print error message
            print "ERROR: %s::%s (%d) - %s" % ( self.__class__.__name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
            return {}

    def _parse_html_source( self, htmlSource ):
        # initialize the parser
        parser = Parser( htmlSource )
        # return results
        return parser.dict
