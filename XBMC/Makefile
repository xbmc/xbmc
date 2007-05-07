
DIRS=guilib xbmc xbmc/FileSystem xbmc/FileSystem/MusicDatabaseDirectory xbmc/FileSystem/VideoDatabaseDirectory xbmc/cores xbmc/cores/paplayer xbmc/cores/DllLoader xbmc/xbox xbmc/linux xbmc/visualizations xbmc/utils guilib/common guilib/tinyXML xbmc/lib/sqlLite xbmc/lib/libscrobbler xbmc/lib/UnrarXLib

all: XboxMediaCenter

XboxMediaCenter: lib
	g++-4.1 -o XboxMediaCenter xbmc/*.o xbmc/settings/*.o guilib/*.o guilib/tinyXML/*.o guilib/common/*.o xbmc/FileSystem/*.o xbmc/FileSystem/VideoDatabaseDirectory/*.o xbmc/FileSystem/MusicDatabaseDirectory/*.o xbmc/visualizations/*.o xbmc/cores/*.o xbmc/cores/paplayer/*.o xbmc/linux/*.o xbmc/lib/sqlLite/*.o xbmc/lib/libscrobbler/*.o xbmc/xbox/*.o xbmc/cores/DllLoader/*.o xbmc/utils/*.o xbmc/lib/UnrarXLib/*.o -lSDL_image -lSDL_gfx -lSDL_mixer -lSDL -llzo -lfreetype -lcdio -lsqlite3 -lfribidi

include Makefile.include
