// XboxMediaCenter
//
// libraries: 
//			- CDRipX			: doesnt support section loading yet			
//			- xbfilezilla : doesnt support section loading yet
//			
// todo: 
//			- zoom/stretch
//					- 4:3 / 16:9 compensation for movies & pictures
//			- subtitles:
//					- if zoom/stretch : change subtitle font factor
//		  - video
//				- video starts pretty slooowww..
//				- fix movie calibration if other mode is chosen
//
// user submitted patches:
//				- bobbin007 : autodetect dvd/iso/cdda. Not finished yet
// general:
//			- different skins for 4:3 / 16:9
//			- python scripts GUI (darki)
//			- CDDB query : show progress dialogs
//			-	add an ftp server
//		  - thumbnail: render text with disolving alpha
//			- skinnable key mapping for controls
//			- center text in dialog boxes
//		  - goom
//			- tvguide
//			- screensaver
//			- webserver
//
// My Programs:
// My Files:
// My Pictures:
//      - use analog thumbsticks voor zoom 
//      - FF/RW l/r trigger   : speed/slow down slideshow time
//      - show selected image in file browser
//			- cancel for create thumbs
// my music:
//			- shoutcast
//			- overlay 
//					-FFT
//			    -status of FF/RWND
//			- ff/rwnd
//			- cancel search 
//			- cancel scan.
// my videos:
//		  - control panel like in xbmp
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
