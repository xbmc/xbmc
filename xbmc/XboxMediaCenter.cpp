// XboxMediaCenter
//
// libraries:
//			- CDRipX			: doesnt support section loading yet			
//			- xbfilezilla : doesnt support section loading yet
//
// test:
//			- my files copy : shortens filenames to 42 chars.
// bugs: 
//			- reboot button blinks 1/2 speed???
//		  - reboot doesnt work on some biosses
//			- in debug mode CSMBDirectory gives an stack corruption
//
// general:
//			- MPLAYER
//					- other formats like ogg,matroska,...
//			- python scripts GUI
//			- CDDB query : show dialogs
//			- keep groups together when sorting (hd, shares,...)
//			-	ftp server
//		  - thumbnail: render text with disolving alpha
//      - screen calibration offsets
//  		- 4:3/16:9 aspect ratio
//			- skinnable key mapping for controls
//			- center text in dialog boxes
//			- autodetect dvd/iso/cdda
//		  - goom
//			- tvguide
//
// My Files:
//
// My Pictures:
//      - use analog thumbsticks voor zoom en move
//      - FF/RW l/r trigger   : speed/slow down slideshow time
//      - show selected image in file browser
//
// my music:
//			- shoutcast
//			- when playing : show album info/playtime/... in leftbottom corner
//			- ff/rwnd
//			- status of FF/RWND/pause
//			- cancel search 
//			- cancel scan.
//
// my videos:
//		  - control panel like in xbmp
//		  - back 7 sec, forward 30 sec buttons
//		  - imdb
//			- when playing : show video in leftbottom corner
//			- bookmarks
//			- OSD
//			- subtitles (positioning also)
//			- file stacking
//
// settings:
//			- general:
//          - restore to default settings
//					- CDDB on/off?
//			- audio:
//          - output 2 all speakers
//			- Slideshow
//					- specify mypics transition time/slideshow time
//		  - screen:
//					- screen calibration offsets
//					- filter on/off
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
