// XboxMediaCenter
//
//
// libraries: 
//			- CDRipX			: doesnt support section loading yet			
//			- xbfilezilla : doesnt support section loading yet
//			
// solved, need 2 test:
//			- [XC]D-Ice : my pictures: dpad down moves pic down (not in zoommode)
//
// bugs: 
//			- reboot button blinks 1/2 speed???
//		  - reboot doesnt work on some biosses
//			- in debug mode CSMBDirectory gives a stack corruption
//
//			- zoom/stretch
//					- zoom doesnt work if movie height > movie width
//					- easy way 2 switch between zoom/stretch/normal
//					- 4:3 / 16:9 compensation for movies
//
//			- subtitles:
//					- subtitles are resized in stretch mode 2?
//					- if zooming causes the movie to use most of the screen then put subtitles in video
//					- try to keep subtitles on bottom of screen
//
//		  - video
//				- video starts pretty slooowww..
//				- fix video preview window
//				- reset video window from 720x576 to 640x480 if movie <= 640x480?
//
// bugs reported by users
//			  - [XC]D-Ice : my pictures: show bmp while listening mp3-> 10..15 secs b4 picture shows up? its k when notplaying audio
//				
// feature requests by users:
//				- pike|afk  : version number for mplayer.dll
//			  - ???				: autoupdate for mplayer.dll
//
//
// general:
//			- different skins for 4:3 / 16:9
//			- python scripts GUI (darki)
//			- CDDB query : show progress dialogs
//			- keep groups together when sorting (hd, shares, dirs, files, ...)
//			-	add an ftp server
//		  - thumbnail: render text with disolving alpha
//			- skinnable key mapping for controls
//			- center text in dialog boxes
//			- autodetect dvd/iso/cdda
//		  - goom
//			- tvguide
//			- screensaver
//			- webserver
//
// My Programs:
//
// My Files:
//
// My Pictures:
//      - use analog thumbsticks voor zoom en move
//      - FF/RW l/r trigger   : speed/slow down slideshow time
//      - show selected image in file browser
//			- cancel for create thumbs
//
// my music:
//			- shoutcast
//			- overlay 
//					-show playtime using large font
//					-FFT
//			    - status of FF/RWND
//			- ff/rwnd
//			- cancel search 
//			- cancel scan.
//
// my videos:
//		  - control panel like in xbmp
//		  - back 7 sec, forward 30 sec buttons
//			- when playing : show video info in leftbottom corner 
//			- bookmarks
//			- OSD
//			- file stacking
//
// Settings:
//			- general:
//					- CDDB on/off?
//          - restore to default settings
//			- audio:
//          - output 2 all speakers
//			- Slideshow
//					- specify mypics transition time/slideshow time
//		  - screen:
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
