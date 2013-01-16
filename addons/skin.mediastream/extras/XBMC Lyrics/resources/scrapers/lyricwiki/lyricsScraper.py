"""
Scraper for http://www.lyricwiki.org

Nuka1195
"""

import sys
import os
from sgmllib import SGMLParser
import urllib

if ( __name__ != "__main__" ):
    import xbmc

__title__ = "LyricWiki.org"
__allow_exceptions__ = True


class _SongListParser( SGMLParser ):
    """ Parser Class: parses an html document for all song links """
    def reset( self ):
        SGMLParser.reset( self )
        self.song_list = []
        self.url = "None"

    def start_a( self, attrs ):
        for key, value in attrs:
            if ( key == "href" ):
                if ( not value[ 0 ] == "#" and not value[ : 10 ] =="/Category:" and
                    not value[ : 11 ] == "/LyricWiki:" and not value == "/Main_Page" and
                    not value[ : 15 ] == "/Special:Search" ):
                    self.url = value
            elif ( key == "title" ):
                if ( urllib.unquote( self.url[ 1 : ] ) == urllib.unquote( value.replace( " ", "_" ).replace( "&amp;", "&" ) ) ):
                    self.song_list += [ ( unicode( value[ value.find( ":" ) + 1 : ], "utf-8", "ignore" ), self.url, ) ]
            else:
                self.url = "None"

class _LyricsParser( SGMLParser ):
    """ Parser Class: parses an html document for song lyrics """
    def reset( self ):
        SGMLParser.reset( self )
        self.lyrics_found = False
        self.lyrics = unicode( "", "utf-8" )

    def start_div( self, attrs ):
        for key, value in attrs:
            if ( key == "class" and value == "lyricbox" ):
                self.lyrics_found = True
            else: self.lyrics_found = False
                
    def start_pre( self, attrs ):
        self.lyrics_found = True

    def start_br( self, attrs ):
        self.lyrics += "\n"

    def handle_data( self, text ):
        if ( self.lyrics_found ):
            try:
                self.lyrics += unicode( text, "utf-8", "ignore" )
            except:
                # bad data so skip it
                pass

class LyricsFetcher:
    """ required: Fetcher Class for www.lyricwiki.org """
    def __init__( self ):
        self.base_url = "http://lyricwiki.org"
        self._set_exceptions()
        
    def get_lyrics( self, artist, song ):
        """ *required: Returns song lyrics or a list of choices from artist & song """
        url = self.base_url + "/%s:%s"
        artist = self._format_param( artist )
        song = self._format_param( song, False )
        lyrics = self._fetch_lyrics( url % ( artist, song, ) )
        # if no lyrics found try just artist for a list of songs
        if ( not lyrics ):
            song_list = self._get_song_list( artist )
            return song_list
        else: return self._clean_text( lyrics )
    
    def get_lyrics_from_list( self, item ):
        """ *required: Returns song lyrics from user selection - item[1]"""
        lyrics = self._fetch_lyrics( self.base_url + item[ 1 ] )
        return self._clean_text( lyrics )
        
    def _set_exceptions( self, exception=None ):
        """ Sets exceptions for formatting artist """
        try:
            if ( __name__ == "__main__" ):
                ex_path = os.path.join( os.getcwd(), "exceptions.txt" )
            else:
                name = __name__.replace( "resources.scrapers.", "" ).replace( ".lyricsScraper", "" )
                ex_path = xbmc.translatePath( os.path.join( "P:\\script_data", os.getcwd(), "scrapers", name, "exceptions.txt" ) )
            ex_file = open( ex_path, "r" )
            self.exceptions = eval( ex_file.read() )
            ex_file.close()
        except:
            self.exceptions = {}
        if ( exception is not None ):
            self.exceptions[ exception[ 0 ] ] = exception[ 1 ]
            self._save_exception_file( ex_path, self.exceptions )

    def _save_exception_file( self, ex_path, exceptions ):
        """ Saves the exception file as a repr(dict) """
        try:
            if ( not os.path.isdir( os.path.split( ex_path )[ 0 ] ) ):
                os.makedirs( os.path.split( ex_path )[ 0 ] )
            ex_file = open( ex_path, "w" )
            ex_file.write( repr( exceptions ) )
            ex_file.close()
        except: pass
        
    def _fetch_lyrics( self, url ):
        """ Fetch lyrics if available """
        try:
            # Open url or local file (if debug)
            if ( not debug ):
                usock = urllib.urlopen( url )
            else:
                usock = open( os.path.join( os.getcwd(), "lyrics_source.txt" ), "r" )
            htmlSource = usock.read()
            usock.close()
            # Save htmlSource to a file for testing scraper (if debugWrite)
            if ( debugWrite ):
                file_object = open( os.path.join( os.getcwd(), "lyrics_source.txt" ), "w" )
                file_object.write( htmlSource )
                file_object.close()
            # Parse htmlSource for lyrics
            parser = _LyricsParser()
            parser.feed( htmlSource )
            parser.close()
            return parser.lyrics
        except:
            return None
        
    def _get_song_list( self, artist ):
        """ If no lyrics found, fetch a list of choices """
        try:
            url = self.base_url + "/%s"
            # Open url or local file (if debug)
            if ( not debug ):
                usock = urllib.urlopen( url % ( artist, ) )
            else:
                usock = open( os.path.join( os.getcwd(), "songs_source.txt" ), "r" )
            htmlSource = usock.read()
            usock.close()
            # Save htmlSource to a file for testing scraper (if debugWrite)
            if ( debugWrite ):
                file_object = open( os.path.join( os.getcwd(), "songs_source.txt" ), "w" )
                file_object.write( htmlSource )
                file_object.close()
            # Parse htmlSource for song links
            parser = _SongListParser()
            parser.feed( htmlSource )
            parser.close()
            # Create sorted return list
            song_list = self._remove_dupes( parser.song_list )
            return song_list
        except:
            return None
            
    def _remove_dupes( self, song_list ):
        """ Returns a sorted list with duplicates removed """
        # this is apparently the fastest method
        dupes = {}
        for x in song_list:
            dupes[ x ] = x
        new_song_list = dupes.values()
        new_song_list.sort()
        return new_song_list

    def _format_param( self, param, exception=True ):
        """ Converts param to the form expected by www.lyricwiki.org """
        caps = True
        result = ""
        # enumerate thru string to properly capitalize words (why doesn't title() do this properly?)
        for letter in param.upper().strip():
            if ( letter == " " ):
                letter = "_"
                caps = True
            elif ( letter in "(_-." ):
                caps = True
            elif ( unicode( letter.isalpha() ) and caps ):
                caps = False
            else:
                letter = letter.lower()
                caps = False
            result += letter
        result = result.replace( "/", "_" )
        # properly quote string for url
        result = urllib.quote( result )
        # replace any exceptions
        if ( exception and result in self.exceptions ):
            result = self.exceptions[ result ]
        return result
    
    def _clean_text( self, text ):
        """ Convert line terminators and html entities """
        try:
            text = text.replace( "\t", "" )
            text = text.replace( "<br> ", "\n" )
            text = text.replace( "<br>", "\n" )
            text = text.replace( "<br /> ", "\n" )
            text = text.replace( "<br />", "\n" )
            text = text.replace( "<div>", "\n" )
            text = text.replace( "> ", "\n" )
            text = text.replace( ">", "\n" )
            text = text.replace( "&amp;", "&" )
            text = text.replace( "&gt;", ">" )
            text = text.replace( "&lt;", "<" )
            text = text.replace( "&quot;", '"' )
        except: 
            pass
        return text

# used for testing only
debug = False
debugWrite = False

if ( __name__ == "__main__" ):
    # used to test get_lyrics() 
    artist = [ "Iron & Wine", "The Charlie Daniels Band", "ABBA", "Jem","Stealers Wheel","Paul McCartney & Wings","ABBA","AC/DC", "Tom Jones", "Kim Mitchell", "Ted Nugent", "Blue Öyster Cult", "The 5th Dimension", "Big & Rich", "Don Felder" ]
    song = [ "On Your Wings", "(What This World Needs Is) A Few More Rednecks", "S.O.S","24","Stuck in the middle with you","Band on the run", "Dancing Queen", "T.N.T.", "She's A Lady", "Go for Soda", "Free-for-all", "(Don't Fear) The Reaper", "Age of Aquarius", "Save a Horse (Ride a Cowboy)", "Heavy Metal (Takin' a Ride)" ]
    for cnt in range( 4,5 ):
        lyrics = LyricsFetcher().get_lyrics( artist[ cnt ], song[ cnt ] )
    
    # used to test get_lyrics_from_list() 
    #url = ("Big & Rich:Save a Horse (Ride a Cowboy)", "/Big_%26_Rich:Save_a_Horse_%28Ride_a_Cowboy%29")
    #url = (u"Stuck In The Middle With You", "/Stealers_Wheel:Stuck_In_The_Middle_With_You")
    #lyrics = LyricsFetcher().get_lyrics_from_list( url )
    
    # print the results
    if ( isinstance( lyrics, list ) ):
        for song in lyrics:
            print song
    else:
        print lyrics.encode( "utf-8", "ignore" )
