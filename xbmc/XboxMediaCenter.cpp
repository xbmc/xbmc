// XboxMediaCenter
//
// libraries: 
//			- CDRipX			: doesnt support section loading yet			
//			- xbfilezilla : doesnt support section loading yet
//			
// bugs: 
//			- reboot button blinks 1/2 speed???
//		  - reboot doesnt work on some biosses
//			- in debug mode CSMBDirectory gives a stack corruption
//
// general:
//			- different skins for 4:3 / 16:9
//			- UI screen calibration    : just use (x,y) offset, no rescaling
//			- movie screen calibration : scale movie
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
//			- speedup artists/albums?
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
// Settings:
//			- general:
//					- CDDB on/off?
//          - restore to default settings
//			- audio:
//          - output 2 all speakers
//			- Slideshow
//					- specify mypics transition time/slideshow time
//		  - screen:
//					- UI screen calibration offsets
//					- movies screen calibration 
//					- soften on/off
//					- stretch / zoom
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
