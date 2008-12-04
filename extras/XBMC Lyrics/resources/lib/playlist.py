"""
This script will create a playlist by recursively searching MUSIC_PATH and adding songs.
"""
import os
import xbmc

# add all music paths here, no subdirectories needed
MUSIC_PATH = ( "E:/Music", "F:/Music", )
# add all music extensions wanted in lowercase
MUSIC_EXT = xbmc.getSupportedMedia( "music" )
# set to True to shuffle the playlist
SHUFFLE = True

def create_playlist( base_path=MUSIC_PATH, shuffle=SHUFFLE ):
    playlist = xbmc.PlayList( 0 )
    playlist.clear()
    for path in base_path:
        os.path.walk( path, add_music, playlist )
    if ( shuffle ): playlist.shuffle()
    return playlist

def add_music( playlist, path, files ):    
    for file in files:
        ext = os.path.splitext( file )[ 1 ].lower()
        if ( ext and ext in MUSIC_EXT ):
            playlist.add( os.path.join( path, file ) )

if ( __name__ == "__main__" ):
    playlist = create_playlist()
    xbmc.Player().play( playlist )
