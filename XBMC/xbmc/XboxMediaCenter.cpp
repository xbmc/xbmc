// XboxMediaCenter
//
// graphics:
//			- pike:special icon for files (they now use the folder icon)
//
// bugs: 
//			- reboot button blinks 1/2 speed???
//		  - reboot doesnt work on some biosses

// general:
//			- keep groups together when sorting (hd, shares,...)
//			-	ftp server
//      - thumbnails need 2b centered in folder icon
//
// guilib:
//		  - thumbnail: render text with disolving alpha
//      - screen calibration offsets
//  		- 4:3/16:9 aspect ratio
//			- skinnable key mapping for controls
//
// My Pictures:
//      - use analog thumbsticks voor zoom en move
//      - FF/RW l/r trigger   : speed/slow down slideshow time
//      - show selected image in file browser
//
// my music:
//			- http://www.saunalahti.fi/~cse/html/tag.html
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
//          - restore to default settings
//			- Slideshow
//					- specify mypics transition time/slideshow time
//		  - screen:
//					- screen calibration offsets
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
