
#ifndef _FFMPEG_H_
#define _FFMPEG_H_

#define EMULATE_INTTYPES
#define HAVE_MMX
#include "avformat.h"

static bool ffmpeg_printed_newline = false;
static void dvdplayer_log(void* ptr, int level, const char* format, va_list va)
{
	char tmp[2048];
	int charsprinted = _vsnprintf(tmp, 2048, format, va);

	if(!ffmpeg_printed_newline)
	{
	  OutputDebugString("ffmpeg msg: ");
	  ffmpeg_printed_newline = true;
	}
	if (tmp[charsprinted-1] == '\n')
	{
	  ffmpeg_printed_newline = false;
	}
	
	tmp[charsprinted] = 0;
	OutputDebugString(tmp);
}

#endif // _FFMPEG_H_
