/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include <xtl.h>

/*
  It is assumed that this file is the first file included in stdafx.h
  Via the next define, the declaration of CCriticalSectionWrapper is moved
  to the top of stdafx.h
  This makes it possible to use CCriticalSectionWrapper in other header files that
  are included in stdafx.h
*/

#define CCRITICALSECTIONWRAPPERINCLUDED

class CCriticalSectionWrapper
{
public:
	CCriticalSectionWrapper();
	~CCriticalSectionWrapper();
	
	void Lock();
	void Unlock();

protected:
	CRITICAL_SECTION m_criticalSection;
	BOOL m_bInitialized;
};

// some defines to elliminate duplicate names in XBMP
#define CThread CXBFileZilla_CThread
#define gethostbyname CXBFileZilla_gethostbyname

#include "xbwindows.h"
#include "xbtimer.h"
#include "xbfilezillaimp.h"
//
//  Time Flags for GetTimeFormat.
//
#define TIME_NOMINUTESORSECONDS   0x00000001  // do not use minutes or seconds
#define TIME_NOSECONDS            0x00000002  // do not use seconds
#define TIME_NOTIMEMARKER         0x00000004  // do not use time marker
#define TIME_FORCE24HOURFORMAT    0x00000008  // always use 24 hour format


//
//  Date Flags for GetDateFormat.
//
#define DATE_SHORTDATE            0x00000001  // use short date picture
#define DATE_LONGDATE             0x00000002  // use long date picture
#define DATE_USE_ALT_CALENDAR     0x00000004  // use alternate calendar (if any)

#if(WINVER >= 0x0500)
#define DATE_YEARMONTH            0x00000008  // use year month picture
#define DATE_LTRREADING           0x00000010  // add marks for left to right reading order layout
#define DATE_RTLREADING           0x00000020  // add marks for right to left reading order layout
#endif /* WINVER >= 0x0500 */


int GetDateFormat(
  LCID Locale,               // locale for which date is to be formatted
  DWORD dwFlags,             // flags specifying function options
  CONST SYSTEMTIME *lpDate,  // date to be formatted
  LPCTSTR lpFormat,          // date format string
  LPTSTR lpDateStr,          // buffer for storing formatted string
  int cchDate                // size of buffer
);
 

int GetTimeFormat(
  LCID Locale,       // locale for which time is to be formatted
  DWORD dwFlags,             // flags specifying function options
  CONST SYSTEMTIME *lpTime,  // time to be formatted
  LPCTSTR lpFormat,          // time format string
  LPTSTR lpTimeStr,          // buffer for storing formatted string
  int cchTime                // size, in bytes or characters, of the buffer
);

#endif
