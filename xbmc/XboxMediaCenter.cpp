// guiTest.cpp : Defines the entry point for the application.
//
// graphics:
//			- default icons voor apps,pictures,audio/video/shortcuts
//			- default icons voor drives aanpassen
//		  - new icon voor reboot knop in home
//
// need 2 test: 
//      - tijd van files (loopt 1 uur voor/achter??)
//      - dialog wat groter gemaakt
//
// bugs: 
//
// algemeen:
//			-	ftp server
//      - thumbnails : centreren in folder indien thumb <64x64
//      - file info box
//      - show if movie contains a subtitle
//      - show if mp3  contains id3 tag info
//      - show picture dimensions in thumbnail view
//
// guilib:
//		  - alpha text voor thumbnail
//      - screen calibration offsets
//  		- 4:3/16:9 aspect ratio
//  		- radio button: radio is vierkant ipv een rondje?
//			- skinnable key mapping for controls
//
// My Pictures:
//      - use analog thumbsticks voor zoom en move
//      - FF/RW l/r trigger   : speed/slow down slideshow time
//      - show selected image in file browser
//
// my music:
//		  - show track, filename, time in listview
//	    - show fileinfo:id3v1/id3v2/mpeg info 
//		  - sort : path,filename, length,track
//			- m3u playlists (save/load)
//			- play window:
//					- show current playtime, kbps, stereo/mono, samplerate (khz), total duration, id3 tag info			
//			- shoutcast
//		  - cddb
//
// my videos:
//		  - control panel like in xbmp
//		  - back 7 sec, forward 30 sec buttons
//		  - imdb
//
// settings:
//			- general:
//					- select skin
//					- ftpserver enabled
//				  - time synchronisation enabled
//          - restore to default settings
//			- Slideshow
//					- specify mypics transition time/slideshow time
//		  - screen:
//					- screen calibration offsets
//					- video params: 16:9 / 4:3  pal/ntsc  resolution
//
//

#include "stdafx.h"
#include "application.h"

CApplication g_application;
void main()
{
	g_application.Create();
  while (1)
  {
    g_application.Run();
  }
}

extern "C" 
{

	void mp_msg( int x, int lev,const char *format, ... )
	{
		va_list va;
		static char tmp[2048];
		va_start(va, format);
		_vsnprintf(tmp, 2048, format, va);
		va_end(va);
		tmp[2048-1] = 0;

		OutputDebugString(tmp);
	}
}
