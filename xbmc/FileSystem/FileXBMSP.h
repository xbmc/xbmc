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
	virtual __int64			GetPosition();
	virtual __int64			GetLength();
	virtual bool				Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
	virtual unsigned int		Read(void* lpBuf, __int64 uiBufSize);
	virtual bool				ReadString(char *szLine, int iLineLength);
	virtual __int64			Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
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
