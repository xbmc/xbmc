// FileRar.h: interface for the CFileRar class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_FILERAR_H__C6E9401A_3715_11D9_8185_0050FC718317__INCLUDED_)
#define AFX_FILERAR_H__C6E9401A_3715_11D9_8185_0050FC718317__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "File.h"
#include "RarManager.h"
using namespace XFILE;

namespace XFILE
{	
	class CFileRar : public IFile  
	{
	public:
		CFileRar();
		virtual ~CFileRar();
		virtual __int64			  GetPosition();
		virtual __int64			  GetLength();
		virtual bool					Open(const CURL& url, bool bBinary=true);
		virtual bool					Exists(const CURL& url);
		virtual int						Stat(const CURL& url, struct __stat64* buffer);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual int						Write(const void* lpBuf, __int64 uiBufSize);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
		virtual void					Close();
		virtual void          Flush();

		virtual bool					OpenForWrite(const CURL& url, bool bBinary=true);
		unsigned int					Write(void *lpBuf, __int64 uiBufSize);
		
		virtual bool          Delete(const char* strFileName);
		virtual bool          Rename(const char* strFileName, const char* strNewFileName);
	protected:
		CFile			m_File;
		CStdString	m_strCacheDir;
		CStdString	m_strRarPath;
		CStdString m_strPassword;
		CStdString m_strPathInRar;
		BYTE m_bRarOptions;
		BYTE m_bFileOptions;
		void InitFromUrl(const CURL& url);
	};

};
#endif // !defined(AFX_FILERAR_H__C6E9401A_3715_11D9_8185_0050FC718317__INCLUDED_)