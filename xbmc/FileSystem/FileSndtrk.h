// FileSndtrk.h: interface for the CFileSndtrk class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FileSndtrk_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
#define AFX_FileSndtrk_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

using namespace XFILE;

namespace XFILE
{
	class CFileSndtrk : public IFile  
	{
	public:
		CFileSndtrk();
		virtual ~CFileSndtrk();
		virtual __int64			  GetPosition();
		virtual __int64			  GetLength();
		virtual bool					Open(const CURL& url, bool bBinary=true);
		virtual bool					Exists(const CURL& url) { return true;};
		virtual int						Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual int						Write(const void* lpBuf, __int64 uiBufSize);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
		virtual void					Close();

		bool									OpenForWrite(const char* strFileName);
		unsigned int					Write(void *lpBuf, __int64 uiBufSize);
	protected:
		CAutoPtrHandle				m_hFile;
		__int64								m_i64FileLength;
		__int64								m_i64FilePos;
	};

};
#endif // !defined(AFX_FileSndtrk_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
