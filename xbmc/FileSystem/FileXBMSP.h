// FileXBMSP.h: interface for the CFileXBMSP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEXMBMSP_H___INCLUDED_)
#define AFX_FILEXMBMSP_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
using namespace XFILE;

extern "C" {
#include "../lib/libxbms/ccincludes.h"
#include "../lib/libxbms/ccxclient.h"
}

namespace XFILE
{

class CFileXBMSP : public IFile  
{
public:
	CFileXBMSP();
	virtual ~CFileXBMSP();
	virtual offset_t			GetPosition();
	virtual offset_t			GetLength();
	virtual bool				Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
	virtual unsigned int		Read(void* lpBuf, offset_t uiBufSize);
	virtual bool				ReadString(char *szLine, int iLineLength);
	virtual offset_t			Seek(offset_t iFilePosition, int iWhence=SEEK_SET);
	virtual void				Close();
protected:
	UINT64						m_fileSize;
	UINT64						m_filePos;
	SOCKET						m_socket;
private:
	CcXstreamServerConnection	m_connection;
	unsigned long				m_handle;
	bool						m_bOpened;
	
};
};

#endif // !defined(AFX_FILEXMBMSP_H___INCLUDED_)
