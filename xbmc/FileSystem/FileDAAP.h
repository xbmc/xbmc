// FileDAAP.h: interface for the CFileDAAP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEDAAP_H___INCLUDED_)
#define AFX_FILEDAAP_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
using namespace XFILE;

extern "C" {
	#include "../lib/libXDAAP/client.h"
	#include "../lib/libXDAAP/private.h"
}

#include "../application.h"

namespace XFILE
{
	class CFileDAAP : public IFile  
	{
public:
		CFileDAAP();
		virtual ~CFileDAAP();
		virtual __int64			GetPosition();
		virtual __int64			GetLength();
		virtual bool			Open(const CURL& url, bool bBinary=true);
		virtual bool			Exists(const CURL& url);
		virtual int				Stat(const CURL& url, struct __stat64* buffer);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual bool			ReadString(char *szLine, int iLineLength);
		virtual __int64			Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
		virtual void			Close();

protected:
		UINT64					m_fileSize;
		UINT64					m_filePos;
		DAAP_SClient			*m_thisClient;
		DAAP_SClientHost		*m_thisHost;
		DAAP_ClientHost_Song	m_song;

private:
		unsigned long			m_handle;
		bool					m_bOpened;
		void					DestroyDAAP();
	};
};

#endif // !defined(AFX_FILEDAAP_H___INCLUDED_)
