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

//typedef __int64 __int64;

#include "stdstring.h"
#include "..\URL.h"
using namespace std;

namespace XFILE
{

  class IFile  
  {
  public:
	  IFile();
	  virtual ~IFile();

	  virtual bool					Open(const CURL& url, bool bBinary=true)=0;
	  virtual bool          OpenForWrite(const CURL& url, bool bBinary = true) { return false; };
	  virtual bool					Exists(const CURL& url)=0;
	  virtual int           Stat(const CURL& url, struct __stat64* buffer)=0;

	  virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize)=0;
		virtual int						Write(const void* lpBuf, __int64 uiBufSize) {return -1;};
	  virtual bool					ReadString(char *szLine, int iLineLength)=0;
	  virtual __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET)=0;
	  virtual void					Close()=0;
	  virtual __int64			  GetPosition()=0;
	  virtual __int64			  GetLength()=0;
	  virtual bool          CanSeek() { return true; }
	  virtual char          GetDirectorySeperator() { return '\\'; }
	  virtual void          Flush() { }
	  
	  virtual bool          Delete(const char* strFileName) { return false; }
    virtual bool          Rename(const char* strFileName, const char* strNewFileName) { return false; }
  };
};

#endif // !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
