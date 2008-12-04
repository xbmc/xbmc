"""
Scraper for embedded lyrics

Nuka1195
"""

if ( __name__ != "__main__" ):
    import xbmc
import re

__title__ = "Embedded Lyrics"
__allow_exceptions__ = False


class LyricsFetcher:
    """ required: Fetcher Class for embedded lyrics """
    def __init__( self ):
        self.pattern = "USLT.*?[A-Za-z]{3}\x00([^\x00]*?)\x00"

    def get_lyrics( self, artist, song ):
        """ *required: Returns song lyrics or a blank string if none found """
        if ( __name__ == "__main__" ):
            file_path = song
        else:
            file_path = xbmc.Player().getPlayingFile()
        lyrics = self._fetch_lyrics( file_path )
        # if no lyrics found return blank string as there is no song list for embedded lyrics
        if ( lyrics is None ):
            return ""
        else:
            return self._clean_text( lyrics )
    
    def get_lyrics_from_list( self, item ):
        """ *required: Returns song lyrics from user selection - item[1] (not used for embedded) """
        return None

    def _fetch_lyrics( self, file_path ):
        """ Fetch lyrics if available """
        try:
            file_object = open( file_path , "rb" )
            file_data = file_object.read()
            file_object.close()
            lyrics = re.findall( self.pattern, file_data )
            return lyrics[ 0 ]
        except:
            return None

    def _clean_text( self, text ):
        """ Convert line terminators and html entities """
        text = text.replace( "\t", "" )
        text = text.replace( "\r\n", "\n" )
        text = text.replace( "\r", "\n" )
        if ( text.endswith( "TEXT" ) or text.endswith( "TIT2" ) ):
            text = text[ : -4 ]
        return text


if ( __name__ == "__main__" ):
    path = "C:\\Documents and Settings\\All Users\\Documents\\My Music\\"
    # used to test get_lyrics() 
    artist = None
    song = [ "The Verve - Bitter Sweet Symphony.mp3", "S.O.S.mp3", "S.O.S_wma.mp3", "Steve Miller Band - Abracadabra.mp3" ]
    lyrics = LyricsFetcher().get_lyrics( artist, path + song[ 0 ] )
    
    # print the results
    print lyrics.encode( "utf-8", "ignore" )
