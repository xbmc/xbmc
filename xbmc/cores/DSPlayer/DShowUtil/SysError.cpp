// SysError.cpp: implementation of the SysError class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif //_DEBUG


//#include <DshowUtil/abuse.h>
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

SysError::SysError():runtime_error(errorDesc(GetLastError())),m_code(GetLastError())
{
}

SysError::SysError(const std::string& msg):runtime_error(msg),m_code(GetLastError())
{
}

SysError::SysError(int l):runtime_error(errorDesc(l)),m_code(l)
{
}
SysError::~SysError()throw()
{

}

string SysError::errorDesc(int code)
{
#ifdef WIN32
  LPVOID lpMsgBuf;
  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL)==0)
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
