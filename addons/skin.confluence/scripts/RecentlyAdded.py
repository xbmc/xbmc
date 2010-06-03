import xbmc
from xbmcgui import Window
from urllib import quote_plus, unquote_plus
import re
import sys
import os


class Main:
    # grab the home window
    WINDOW = Window( 10000 )

    def _clear_properties( self ):
        # reset Totals property for visible condition
        self.WINDOW.clearProperty( "Database.Totals" )
        # we enumerate thru and clear individual properties in case other scripts set window properties
        for count in range( self.LIMIT ):
            # we clear title for visible condition
            self.WINDOW.clearProperty( "LatestMovie.%d.Title" % ( count + 1, ) )
            self.WINDOW.clearProperty( "LatestEpisode.%d.ShowTitle" % ( count + 1, ) )
            self.WINDOW.clearProperty( "LatestSong.%d.Title" % ( count + 1, ) )

    def _get_media( self, path, file ):
        # set default values
        play_path = fanart_path = thumb_path = path + file
        # we handle stack:// media special
        if ( file.startswith( "stack://" ) ):
            play_path = fanart_path = file
            thumb_path = file[ 8 : ].split( " , " )[ 0 ]
        # we handle rar:// and zip:// media special
        if ( file.startswith( "rar://" ) or file.startswith( "zip://" ) ):
            play_path = fanart_path = thumb_path = file
        # return media info
        return xbmc.getCacheThumbName( thumb_path ), xbmc.getCacheThumbName( fanart_path ), play_path

    def _parse_argv( self ):
        try:
            # parse sys.argv for params
            params = dict( arg.split( "=" ) for arg in sys.argv[ 1 ].split( "&" ) )
        except:
            # no params passed
            params = {}
        # set our preferences
        self.LIMIT = int( params.get( "limit", "5" ) )
        self.RECENT = not params.get( "partial", "" ) == "True"
        self.ALBUMS = params.get( "albums", "" ) == "True"
        self.UNPLAYED = params.get( "unplayed", "" ) == "True"
        self.TOTALS = params.get( "totals", "" ) == "True"
        self.PLAY_TRAILER = params.get( "trailer", "" ) == "True"
        self.ALARM = int( params.get( "alarm", "0" ) )

    def _set_alarm( self ):
        # only run if user/skinner preference
        if ( not self.ALARM ): return
        # set the alarms command
        command = "XBMC.RunScript(%s,limit=%d&partial=%s&albums=%s&unplayed=%s&totals=%s&trailer=%s&alarm=%d)" % ( os.path.join( os.getcwd(), __file__ ), self.LIMIT, str( not self.RECENT ), str( self.ALBUMS ), str( self.UNPLAYED ), str( self.TOTALS ), str( self.PLAY_TRAILER ), self.ALARM, )
        xbmc.executebuiltin( "AlarmClock(LatestAdded,%s,%d,true)" % ( command, self.ALARM, ) )

    def __init__( self ):
        # parse argv for any preferences
        self._parse_argv()
        # clear properties
        self._clear_properties()
        # set any alarm
        self._set_alarm()
        # format our records start and end
        xbmc.executehttpapi( "SetResponseFormat()" )
        xbmc.executehttpapi( "SetResponseFormat(OpenRecord,%s)" % ( "<record>", ) )
        xbmc.executehttpapi( "SetResponseFormat(CloseRecord,%s)" % ( "</record>", ) )
        # fetch media info
        self._fetch_totals()
        self._fetch_movie_info()
        self._fetch_tvshow_info()
        self._fetch_music_info()

    def _fetch_totals( self ):
        # only run if user/skinner preference
        if ( not self.TOTALS ): return
        import datetime
        # get our regions format
        date_format = xbmc.getRegion( "dateshort" ).replace( "MM", "%m" ).replace( "DD", "%d" ).replace( "YYYYY", "%Y" ).replace( "YYYY", "%Y" ).strip()
        # only need to make Totals not empty
        self.WINDOW.setProperty( "Database.Totals", "true" )
        # sql statement for movie totals
        sql_totals = "select count(1), count(playCount), movieview.* from movieview group by lastPlayed"
        totals_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_totals ), )
        records = re.findall( "<record>(.+?)</record>", totals_xml, re.DOTALL )
        # initialize our list
        movies_totals = [ 0 ] * 7
        # enumerate thru and total our numbers
        for record in records:
            fields = re.findall( "<field>(.*?)</field>", record, re.DOTALL )
            movies_totals[ 0 ] += int( fields[ 0 ] )
            movies_totals[ 1 ] += int( fields[ 1 ] )
            if ( fields[ 29 ] ):
                movies_totals[ 2 ] = fields[ 3 ] # title
                movies_totals[ 3 ] = fields[ 10 ] # year
                movies_totals[ 4 ] = fields[ 14 ] # runningtime
                movies_totals[ 5 ] = fields[ 17 ] # genre
                movies_totals[ 6 ] = "" # last watched
                date = fields[ 29 ].split( " " )[ 0 ].split( "-" )
                movies_totals[ 6 ] = datetime.date( int( date[ 0 ] ), int( date[ 1 ] ), int( date[ 2 ] ) ).strftime( date_format ) # last played
        # sql statement for music videos totals
        sql_totals = "select count(1), count(playCount) from musicvideoview"
        totals_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_totals ), )
        mvideos_totals = re.findall( "<field>(.+?)</field>", totals_xml, re.DOTALL )
        # sql statement for tv shows/episodes totals
        sql_totals = "select count(1), sum(totalCount), sum(watched), sum(watchedCount) from (select tvshow.*, path.strPath as strPath, counts.totalcount as totalCount, counts.watchedcount as watchedCount, counts.totalcount=counts.watchedcount as watched from tvshow join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow join path on path.idpath=tvshowlinkpath.idPath left outer join ( select tvshow.idShow as idShow,count(1) as totalcount, count(files.playCount) as watchedcount from tvshow join tvshowlinkepisode on tvshow.idShow = tvshowlinkepisode.idShow join episode on episode.idEpisode = tvshowlinkepisode.idEpisode join files on files.idFile = episode.idFile group by tvshow.idShow) counts on tvshow.idShow = counts.idShow) as tvshowview"
        totals_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_totals ), )
        tvshows_totals = re.findall( "<field>(.+?)</field>", totals_xml, re.DOTALL )
        # if no tvshows we reset values
        if ( tvshows_totals[ 0 ] == "0" ):
            tvshows_totals = ( 0, 0, 0, 0, )
        # sql statement for tv albums/songs totals
        sql_totals = "select count(1), count(distinct strAlbum), count(distinct strArtist) from songview"
        totals_xml = xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( sql_totals ), )
        music_totals = re.findall( "<field>(.+?)</field>", totals_xml, re.DOTALL )
        # set properties
        self.WINDOW.setProperty( "Movies.Count" , str( movies_totals[ 0 ] ) or "" )
        self.WINDOW.setProperty( "Movies.Watched" , str( movies_totals[ 1 ] ) or "" )
        self.WINDOW.setProperty( "Movies.UnWatched" , str( movies_totals[ 0 ] - movies_totals[ 1 ] ) or "" )
        self.WINDOW.setProperty( "Movies.LastWatchedTitle" , movies_totals[ 2 ] or "" )
        self.WINDOW.setProperty( "Movies.LastWatchedYear" , movies_totals[ 3 ] or "" )
        self.WINDOW.setProperty( "Movies.LastWatchedRuntime" , movies_totals[ 4 ] or "" )
        self.WINDOW.setProperty( "Movies.LastWatchedGenre" , movies_totals[ 5 ] or "" )
        self.WINDOW.setProperty( "Movies.LastWatchedDate" , movies_totals[ 6 ] or "" )
        
        self.WINDOW.setProperty( "MusicVideos.Count" , mvideos_totals[ 0 ] or "" )
        self.WINDOW.setProperty( "MusicVideos.Watched" , mvideos_totals[ 1 ] or "" )
        self.WINDOW.setProperty( "MusicVideos.UnWatched" , str( int( mvideos_totals[ 0 ] ) - int( mvideos_totals[ 1 ] ) ) or "" )
        
        self.WINDOW.setProperty( "TVShows.Count" , tvshows_totals[ 0 ] or "" )
        self.WINDOW.setProperty( "TVShows.Watched" , tvshows_totals[ 2 ] or "" )
        self.WINDOW.setProperty( "TVShows.UnWatched" , str( int( tvshows_totals[ 0 ] ) - int( tvshows_totals[ 2 ] ) ) or "" )
        self.WINDOW.setProperty( "Episodes.Count" , tvshows_totals[ 1 ] or "" )
        self.WINDOW.setProperty( "Episodes.Watched" , tvshows_totals[ 3 ] or "" )
        self.WINDOW.setProperty( "Episodes.UnWatched" , str( int( tvshows_totals[ 1 ] ) - int( tvshows_totals[ 3 ] ) ) or "" )
        
        self.WINDOW.setProperty( "Music.SongsCount" , music_totals[ 0 ] or "" )
        self.WINDOW.setProperty( "Music.AlbumsCount" , music_totals[ 1 ] or "" )
        self.WINDOW.setProperty( "Music.ArtistsCount" , music_totals[ 2 ] or "" )

    def _fetch_movie_info( self ):
        # set our unplayed query
        unplayed = ( "", "where playCount isnull ", )[ self.UNPLAYED ]
        # sql statement
        if ( self.RECENT ):
            # recently added
            sql_movies = "select * from movieview %sorder by idMovie desc limit %d" % ( unplayed, self.LIMIT, )
        else:
            # movies not finished
            sql_movies = "select movieview.*, bookmark.timeInSeconds from movieview join bookmark on (movieview.idFile = bookmark.idFile) %sorder by movieview.c00 limit %d" % ( unplayed, self.LIMIT, )
        # query the database
        movies_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_movies ), )
        # separate the records
        movies = re.findall( "<record>(.+?)</record>", movies_xml, re.DOTALL )
        # enumerate thru our records and set our properties
        for count, movie in enumerate( movies ):
            # separate individual fields
            fields = re.findall( "<field>(.*?)</field>", movie, re.DOTALL )
            # set properties

            self.WINDOW.setProperty( "LatestMovie.%d.Title" % ( count + 1, ), fields[ 2 ] )
            self.WINDOW.setProperty( "LatestMovie.%d.Rating" % ( count + 1, ), fields[ 7 ] )
            self.WINDOW.setProperty( "LatestMovie.%d.Year" % ( count + 1, ), fields[ 9 ] )
            self.WINDOW.setProperty( "LatestMovie.%d.RunningTime" % ( count + 1, ), fields[ 13 ] )
            # get cache names of path to use for thumbnail/fanart and play path
            thumb_cache, fanart_cache, play_path = self._get_media( fields[ 25 ], fields[ 24 ] )
            if os.path.isfile("%s.dds" % (xbmc.translatePath( "special://profile/Thumbnails/Video/%s/%s" % ( "Fanart", os.path.splitext(fanart_cache)[0],) ) )):
                fanart_cache = "%s.dds" % (os.path.splitext(fanart_cache)[0],)
            self.WINDOW.setProperty( "LatestMovie.%d.Path" % ( count + 1, ), ( play_path, fields[ 21 ], )[ fields[ 21 ] != "" and self.PLAY_TRAILER ] )
            self.WINDOW.setProperty( "LatestMovie.%d.Trailer" % ( count + 1, ), fields[ 21 ] )
            self.WINDOW.setProperty( "LatestMovie.%d.Fanart" % ( count + 1, ), "special://profile/Thumbnails/Video/%s/%s" % ( "Fanart", fanart_cache, ) )
            # initial thumb path
            thumb = "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[ 0 ], thumb_cache, )
            # if thumb does not exist use an auto generated thumb path
            if ( not os.path.isfile( xbmc.translatePath( thumb ) ) ):
                thumb = "special://profile/Thumbnails/Video/%s/auto-%s" % ( thumb_cache[ 0 ], thumb_cache, )
            self.WINDOW.setProperty( "LatestMovie.%d.Thumb" % ( count + 1, ), thumb )

    def _fetch_tvshow_info( self ):
        # set our unplayed query
        unplayed = ( "", "where playCount isnull ", )[ self.UNPLAYED ]
        # sql statement
        if ( self.RECENT ):
            # recently added
            sql_episodes = "select * from episodeview %sorder by idepisode desc limit %d" % ( unplayed, self.LIMIT, )
        else:
            # tv shows not finished
            sql_episodes = "select episodeview.*, bookmark.timeInSeconds from episodeview join bookmark on (episodeview.idFile = bookmark.idFile) %sorder by episodeview.strTitle limit %d" % ( unplayed, self.LIMIT, )
        # query the database
        episodes_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_episodes ), )
        # separate the records
        episodes = re.findall( "<record>(.+?)</record>", episodes_xml, re.DOTALL )
        # enumerate thru our records and set our properties
        for count, episode in enumerate( episodes ):
            # separate individual fields
            fields = re.findall( "<field>(.*?)</field>", episode, re.DOTALL )
            # set properties        
            self.WINDOW.setProperty( "LatestEpisode.%d.ShowTitle" % ( count + 1, ), fields[ 28 ] )
            self.WINDOW.setProperty( "LatestEpisode.%d.EpisodeTitle" % ( count + 1, ), fields[ 2 ] )
            self.WINDOW.setProperty( "LatestEpisode.%d.EpisodeNo" % ( count + 1, ), "s%02de%02d" % ( int( fields[ 14 ] ), int( fields[ 15 ] ), ) )
            self.WINDOW.setProperty( "LatestEpisode.%d.Rating" % ( count + 1, ), fields[ 5 ] )
            # get cache names of path to use for thumbnail/fanart and play path
            thumb_cache, fanart_cache, play_path = self._get_media( fields[ 25 ], fields[ 24 ] )
            if ( not os.path.isfile( xbmc.translatePath( "special://profile/Thumbnails/Video/%s/%s" % ( "Fanart", fanart_cache, ) ) ) ):
                fanart_cache = xbmc.getCacheThumbName(os.path.join(os.path.split(os.path.split(fields[ 25 ])[0])[0], ""))
            if os.path.isfile("%s.dds" % (xbmc.translatePath( "special://profile/Thumbnails/Video/%s/%s" % ( "Fanart", os.path.splitext(fanart_cache)[0],) ) )):
                fanart_cache = "%s.dds" % (os.path.splitext(fanart_cache)[0],)
            self.WINDOW.setProperty( "LatestEpisode.%d.Path" % ( count + 1, ), play_path )
            self.WINDOW.setProperty( "LatestEpisode.%d.Fanart" % ( count + 1, ), "special://profile/Thumbnails/Video/%s/%s" % ( "Fanart", fanart_cache, ) )
            # initial thumb path
            thumb = "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[ 0 ], thumb_cache, )
            # if thumb does not exist use an auto generated thumb path
            if ( not os.path.isfile( xbmc.translatePath( thumb ) ) ):
                thumb = "special://profile/Thumbnails/Video/%s/auto-%s" % ( thumb_cache[ 0 ], thumb_cache, )
            self.WINDOW.setProperty( "LatestEpisode.%d.Thumb" % ( count + 1, ), thumb )

    def _fetch_music_info( self ):
        # sql statement
        if ( self.ALBUMS ):
            sql_music = "select idAlbum from albumview order by idAlbum desc limit %d" % ( self.LIMIT, )
            # query the database for recently added albums
            music_xml = xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( sql_music ), )
            # separate the records
            albums = re.findall( "<record>(.+?)</record>", music_xml, re.DOTALL )
            # set our unplayed query
            unplayed = ( "(idAlbum = %s)", "(idAlbum = %s and lastplayed isnull)", )[ self.UNPLAYED ]
            # sql statement
            sql_music = "select songview.* from songview where %s limit 1" % ( unplayed, )
            # clear our xml data
            music_xml = ""
            # enumerate thru albums and fetch info
            for album in albums:
                # query the database and add result to our string
                music_xml += xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( sql_music % ( album.replace( "<field>", "" ).replace( "</field>", "" ), ) ), )
        else:
            # set our unplayed query
            unplayed = ( "", "where lastplayed isnull ", )[ self.UNPLAYED ]
            # sql statement
            sql_music = "select * from songview %sorder by idSong desc limit %d" % ( unplayed, self.LIMIT, )
            # query the database
            music_xml = xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( sql_music ), )
        # separate the records
        items = re.findall( "<record>(.+?)</record>", music_xml, re.DOTALL )
        # enumerate thru our records and set our properties
        for count, item in enumerate( items ):
            # separate individual fields
            fields = re.findall( "<field>(.*?)</field>", item, re.DOTALL )
            # set properties
            self.WINDOW.setProperty( "LatestSong.%d.Title" % ( count + 1, ), fields[ 3 ] )
            self.WINDOW.setProperty( "LatestSong.%d.Year" % ( count + 1, ), fields[ 6 ] )
            self.WINDOW.setProperty( "LatestSong.%d.Artist" % ( count + 1, ), fields[ 24 ] )
            self.WINDOW.setProperty( "LatestSong.%d.Album" % ( count + 1, ), fields[ 21 ] )
            path = fields[ 22 ]
            # don't add song for albums list TODO: figure out how toplay albums
            ##if ( not self.ALBUMS ):
            path += fields[ 8 ]
            self.WINDOW.setProperty( "LatestSong.%d.Path" % ( count + 1, ), path )
            # get cache name of path to use for fanart
            cache_name = xbmc.getCacheThumbName( fields[ 24 ] )
            self.WINDOW.setProperty( "LatestSong.%d.Fanart" % ( count + 1, ), "special://profile/Thumbnails/Music/%s/%s" % ( "Fanart", cache_name, ) )
            self.WINDOW.setProperty( "LatestSong.%d.Thumb" % ( count + 1, ), fields[ 27 ] )


if ( __name__ == "__main__" ):
    Main()
