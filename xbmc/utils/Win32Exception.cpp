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

#include "Win32Exception.h"
#ifndef _LINUX
#include "eh.h"
#endif
#include "log.h"

#ifdef _LINUX

void win32_exception::writelog(const char *prefix)  const
{
  if( prefix )
    CLog::Log(LOGERROR, "%s : %s (code:0x%08x) at %p",
              prefix, what(), (unsigned int) code(), where());
  else
    CLog::Log(LOGERROR, "%s (code:0x%08x) at %p",
              what(), (unsigned int) code(), where());
}


#else

void win32_exception::install_handler()
{
    _set_se_translator(win32_exception::translate);
}

void win32_exception::translate(unsigned code, EXCEPTION_POINTERS* info)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        throw access_violation(info);
        break;
    default:
        throw win32_exception(info);
    }
}

win32_exception::win32_exception(EXCEPTION_POINTERS* info)
: mWhat("Win32 exception"), mWhere(info->ExceptionRecord->ExceptionAddress), mCode(info->ExceptionRecord->ExceptionCode), mExceptionPointers(info)
{
    // Windows guarantees that *(info->ExceptionRecord) is valid
    switch (info->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        mWhat = "Access violation";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        mWhat = "Division by zero";
        break;
    }
}

void win32_exception::writelog(const char *prefix)  const
{
  if( prefix )
    CLog::Log(LOGERROR, "%s : %s (code:0x%08x) at 0x%08x", prefix, (unsigned int) what(), code(), where());
  else
    CLog::Log(LOGERROR, "%s (code:0x%08x) at 0x%08x", what(), code(), where());
}

access_violation::access_violation(EXCEPTION_POINTERS* info)
: win32_exception(info), mAccessType(Invalid), mBadAddress(0)
{
    switch(info->ExceptionRecord->ExceptionInformation[0])
    {
    case 0:
      mAccessType = Read;
      break;
    case 1:
      mAccessType = Write;
      break;
    case 8:
      mAccessType = DEP;
      break;
    }
    mBadAddress = reinterpret_cast<win32_exception ::Address>(info->ExceptionRecord->ExceptionInformation[1]);
}

void access_violation::writelog(const char *prefix) const
{
  if( prefix )
    if( mAccessType == Write)
      CLog::Log(LOGERROR, "%s : %s at 0x%08x: Writing location 0x%08x", prefix, what(), where(), address());
    else if( mAccessType == Read)
      CLog::Log(LOGERROR, "%s : %s at 0x%08x: Reading location 0x%08x", prefix, what(), where(), address());
    else if( mAccessType == DEP)
      CLog::Log(LOGERROR, "%s : %s at 0x%08x: DEP violation, location 0x%08x", prefix, what(), where(), address());
    else
      CLog::Log(LOGERROR, "%s : %s at 0x%08x: unknown access type, location 0x%08x", prefix, what(), where(), address());
  else
    if( mAccessType == Write)
      CLog::Log(LOGERROR, "%s at 0x%08x: Writing location 0x%08x", what(), where(), address());
    else if( mAccessType == Read)
      CLog::Log(LOGERROR, "%s at 0x%08x: Reading location 0x%08x", what(), where(), address());
    else if( mAccessType == DEP)
      CLog::Log(LOGERROR, "%s at 0x%08x: DEP violation, location 0x%08x", what(), where(), address());
    else
      CLog::Log(LOGERROR, "%s at 0x%08x: unknown access type, location 0x%08x", what(), where(), address());

}

#endif
