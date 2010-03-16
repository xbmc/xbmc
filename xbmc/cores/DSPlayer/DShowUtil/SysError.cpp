/*
 *      Copyright (C) 2005-2010 Team XBMC
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

// SysError.cpp: implementation of the SysError class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif //_DEBUG


#include <DshowUtil/SysError.h>
#ifndef WIN32
#include <string.h>
#endif 

#ifdef _DEBUG
// Include CRTDBG.H after all other headers
#include <stdlib.h>
#include <crtdbg.h>
#define NEW_INLINE_WORKAROUND new ( _NORMAL_BLOCK ,\
                                    __FILE__ , __LINE__ )
#define new NEW_INLINE_WORKAROUND
#endif

using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SysError::SysError():
  runtime_error(errorDesc(GetLastError())),
  m_code(GetLastError())
{
}

SysError::SysError(const std::string& msg):
  runtime_error(msg),
  m_code(GetLastError())
{
}

SysError::SysError(int l):
  runtime_error(errorDesc(l)),
  m_code(l)
{
}
SysError::~SysError() throw()
{
}

string SysError::errorDesc(int code)
{
#ifdef WIN32
  LPVOID lpMsgBuf;
  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf, 0, NULL) == 0)
      return "";  // If the formatMessage failed, return empty string

  //copy the string 
  string res((const char*)lpMsgBuf);

  // Free the buffer.
  LocalFree( lpMsgBuf );
  return res;
#else
  char lpMsgBuf[255];
  // Cannot just return string(strerror_r), as the return value is 
  // an int at least under FreeBSD 5.2
  return (strerror_r((int)code,(char*)lpMsgBuf,255));
  //return string(lpMsgBuf);
#endif
}
