#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdio.h>
#include "StdString.h"

#define LOG_LEVEL_NONE         -1 // nothing at all is logged
#define LOG_LEVEL_NORMAL        0 // shows notice, error, severe and fatal
#define LOG_LEVEL_DEBUG         1 // shows all
#define LOG_LEVEL_DEBUG_FREEMEM 2 // shows all + shows freemem on screen
#define LOG_LEVEL_DEBUG_SAMBA   3 // shows all + freemem on screen + samba debugging
#define LOG_LEVEL_MAX           LOG_LEVEL_DEBUG_SAMBA

// ones we use in the code
#define LOGDEBUG   0
#define LOGINFO    1
#define LOGNOTICE  2
#define LOGWARNING 3
#define LOGERROR   4
#define LOGSEVERE  5
#define LOGFATAL   6
#define LOGNONE    7

#ifdef __GNUC__
#define ATTRIB_LOG_FORMAT __attribute__((format(printf,2,3)))
#else
#define ATTRIB_LOG_FORMAT
#endif

class CLog
{
  static FILE*       m_file;
  static int         m_repeatCount;
  static int         m_repeatLogLevel;
  static CStdString* m_repeatLine;
public:
  CLog();
  virtual ~CLog(void);
  static void Close();
  static void Log(int loglevel, const char *format, ... ) ATTRIB_LOG_FORMAT;
  static void DebugLog(const char *format, ...);
  static void MemDump(char *pData, int length);
  static void DebugLogMemory();
  static bool Init(const char* path);
#ifdef _WIN32
  static bool InitW(const char* path);
#endif
};

// GL Error checking macro
// this function is useful for tracking down GL errors, which otherwise
// just result in undefined behavior and can be difficult to track down.
//
// Just call it 'VerifyGLState()' after a sequence of GL calls
//
// if GL_DEBUGGING and HAS_GL are defined, the function checks
// for GL errors and prints the current state of the various matrices;
// if not it's just an empty inline stub, and thus won't affect performance
// and will be optimized out.

void _VerifyGLState(const char* szfile, const char* szfunction, int lineno);
#if defined(GL_DEBUGGING) && defined(HAS_GL)
#define VerifyGLState() _VerifyGLState(__FILE__, __FUNCTION__, __LINE__)
#else
#define VerifyGLState()
#endif

void LogGraphicsInfo();


