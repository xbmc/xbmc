// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
#define AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ifile.h"
using namespace XFILE;

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

#include "stdstring.h"
using namespace std;

namespace XFILE
{
	class IFileCallback
	{
	public:
		virtual bool OnFileCallback(void* pContext, int ipercent)=0;
	};
  class CFile   
  {
  public:
	  CFile();
	  virtual ~CFile();

	  unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		int						Write(const void* lpBuf, __int64 uiBufSize);
	  bool					Open(const char* strFileName, bool bBinary=true);
	  bool          Cache(const char* strFileName, const char* szDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
	  bool					Exists(const char* strFileName);
		int           Stat(const char* strFileName, struct __stat64* buffer);
	  bool					ReadString(char *szLine, int iLineLength);
	  __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
	  void					Close();
	  void					Flush();
	  
    bool          OpenForWrite(const char* strFileName, bool bBinary=true);
    static bool   Delete(const char* strFileName);
    static bool   Rename(const char* strFileName, const char* strNewFileName);
	  
	  __int64			  GetPosition();
	  __int64			  GetLength();
  	
  private:
	  IFile*				m_pFile;
  };
};
#endif // !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
