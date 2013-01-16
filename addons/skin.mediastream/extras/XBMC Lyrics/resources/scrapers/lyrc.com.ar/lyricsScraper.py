"""
Scraper for http://lyrc.com.ar/

"""

import os
import urllib
import re

__title__ = "lyrc.com.ar"
__allow_exceptions__ = False

class LyricsFetcher:
    def __init__( self ):
        self.base_url = "http://lyrc.com.ar/en/tema1en.php"
        
    def get_lyrics(self, artist, song):
        params = urllib.urlencode({'artist': artist, 'songname': song, 'procesado2': '1', 'Submit': ' S E A R C H '})
        f = urllib.urlopen(self.base_url, params)
        Page = f.read()
        if Page.find("Nothing found :") != -1:
            return None
        elif Page.find("Suggestions :") != -1:
            links_query = re.compile('<br><a href=\"(.*?)\"><font color=\'white\'>(.*?)</font></a>', re.IGNORECASE)
            urls = re.findall(links_query, Page)
            links = []
            for x in urls:
                links.append( ( x[1], x[0], ) )
            return links
        else:
            lyrics = self._parse_lyrics( Page )
            return lyrics
    
    def get_lyrics_from_list(self, link):
        self.url = "http://lyrc.com.ar/en/" + link[ 1 ]
        data = urllib.urlopen(self.url)
        Page = data.read()
        lyrics = self._parse_lyrics( Page )
        return lyrics
    
    def _parse_lyrics(self, lyrics):
        try:
            query = re.compile('</script></td></tr></table>(.*)<p><hr size=1', re.IGNORECASE | re.DOTALL)
            full_lyrics = re.findall(query, lyrics)
            final = full_lyrics[0].replace("<br />\r\n","\n ")
        except:
            try:
                query = re.compile('</script></td></tr></table>(.*)<br><br><a href="#', re.IGNORECASE | re.DOTALL)
                full_lyrics = re.findall(query, lyrics)
                final = full_lyrics[0].replace("<br />\r\n","\n ")
            except:
                final = None
        return str(final)

if ( __name__ == '__main__' ):
    # --------------------------------------------------------------------#
    # Used to test get_lyrics() 
    #artist = "Edith Piaf", "Alain Souchon"#"Kim Mitchell"
    #song = "La Foule", "Toto 30 ans"#"Go for Soda"
    #lyrics = LyricsFetcher().get_lyrics( artist, song )
    # --------------------------------------------------------------------#
    
    # --------------------------------------------------------------------#
    # Used to test get_lyrics_from_list() 
    url = ('Edith piaf - La foule', 'tema1en.php?hash=5aa5821ce76e11e425e7dd4c3ee6253c')#('Big and rich - Save a horse, ride a cowboy', 'tema1en.php?hash=776d8fa28c4621dc943dc3a8caa81a32')
    #url = ('Big and rich - Save a horse, ride a cowboy', 'tema1en.php?hash=776d8fa28c4621dc943dc3a8caa81a32')
    lyrics = LyricsFetcher().get_lyrics_from_list( url )
    # --------------------------------------------------------------------#
    
    # print the results
    if ( isinstance( lyrics, list ) ):
        for song in lyrics:
            print song
    else:
        print lyrics
