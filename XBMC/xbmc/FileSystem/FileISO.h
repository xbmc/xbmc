// FileISO.h: interface for the CFileISO class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_)
#define AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "iso9660.h"
#include "../xbox/iosupport.h"


using namespace XFILE;
namespace XFILE
{

	class CFileISO : public IFile  
	{
	public:
		CFileISO();
		virtual ~CFileISO();
		virtual offset_t			GetPosition();
		virtual offset_t			GetLength();
		virtual bool					Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
		virtual unsigned int	Read(void* lpBuf, offset_t uiBufSize);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual offset_t			Seek(offset_t iFilePosition, int iWhence=SEEK_SET);
		virtual void					Close();
	protected:
		iso9660*			m_pIsoReader;
		offset_t		  m_i64FilePos;
		static int		m_iReferences;
	};
};

#endif // !defined(AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_)
