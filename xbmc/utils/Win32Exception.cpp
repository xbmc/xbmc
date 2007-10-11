#include "stdafx.h"
#include "Win32Exception.h"
#ifndef _LINUX
#include "eh.h"
#endif
#include "log.h"

#ifdef _LINUX

void win32_exception::writelog(const char *prefix)  const
{
  if( prefix )
    CLog::Log(LOGERROR, "%s : %s (code:0x%08x) at 0x%08x", prefix, what(), (unsigned int) code(), (unsigned int) where());
  else
    CLog::Log(LOGERROR, "%s (code:0x%08x) at 0x%08x", what(), (unsigned int) code(), (unsigned int) where());
}


#else

void win32_exception::install_handler()
{
    _set_se_translator(win32_exception::translate);
}

void win32_exception::translate(unsigned code, EXCEPTION_POINTERS* info)
{
    // Windows guarantees that *(info->ExceptionRecord) is valid
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        throw access_violation(*(info->ExceptionRecord));
        break;
    default:
        throw win32_exception(*(info->ExceptionRecord));
    }
}

win32_exception::win32_exception(const EXCEPTION_RECORD& info)
: mWhat("Win32 exception"), mWhere(info.ExceptionAddress), mCode(info.ExceptionCode)
{
    switch (info.ExceptionCode) {
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

access_violation::access_violation(const EXCEPTION_RECORD& info)
: win32_exception(info), mIsWrite(false), mBadAddress(0)
{
    mIsWrite = info.ExceptionInformation[0] == 1;
    mBadAddress = reinterpret_cast<win32_exception ::Address>(info.ExceptionInformation[1]);
}

void access_violation::writelog(const char *prefix) const
{
  if( prefix )
    if( mIsWrite )
      CLog::Log(LOGERROR, "%s : %s at 0x%08x: Writing location 0x%08x", prefix, what(), where(), address());
    else
      CLog::Log(LOGERROR, "%s : %s at 0x%08x: Reading location 0x%08x", prefix, what(), where(), address());
  else
    if( mIsWrite )
      CLog::Log(LOGERROR, "%s at 0x%08x: Writing location 0x%08x", what(), where(), address());
    else
      CLog::Log(LOGERROR, "%s at 0x%08x: Reading location 0x%08x", what(), where(), address());

}

#endif
