// FileHD.h: interface for the CFileHD class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
#define AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "../autoptrhandle.h"
using namespace AUTOPTR;
using namespace XFILE;

namespace XFILE
{
	class CFileHD : public IFile  
	{
	public:
		CFileHD();
		virtual ~CFileHD();
		virtual __int64			  GetPosition();
		virtual __int64			  GetLength();
		virtual bool					Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual int						Write(const void* lpBuf, __int64 uiBufSize);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
		virtual void					Close();

		bool						    OpenForWrite(const char* strFileName);
		unsigned int				Write(void *lpBuf, __int64 uiBufSize);
	protected:
		CAutoPtrHandle	m_hFile;
		__int64				m_i64FileLength;
		__int64				m_i64FilePos;
	};

};
#endif // !defined(AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
