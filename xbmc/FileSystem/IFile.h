// IFile.h: interface for the IFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
#define AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

typedef __int64 offset_t;

#include "stdstring.h"
using namespace std;

namespace XFILE
{

  class IFile  
  {
  public:
	  IFile();
	  virtual ~IFile();
	  virtual bool					Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName, int iport,bool bBinary=true)=0;
	  virtual unsigned int	Read(void* lpBuf, offset_t uiBufSize)=0;
	  virtual bool					ReadString(char *szLine, int iLineLength)=0;
	  virtual offset_t			Seek(offset_t iFilePosition, int iWhence=SEEK_SET)=0;
	  virtual void					Close()=0;
	  virtual offset_t			GetPosition()=0;
	  virtual offset_t			GetLength()=0;
	  virtual bool          CanSeek() {return true;};
  };
};

#endif // !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
