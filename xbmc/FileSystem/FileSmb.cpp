// FileSmb.cpp: implementation of the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////

#include "FileSmb.h"
#include "../sectionloader.h"
#include "../settings.h"
using namespace XFILE;

static CRITICAL_SECTION	m_critSection;

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen, 
			char *un, int unlen,  char *pw, int pwlen)
{
	return;
}

CSMB::CSMB()
{
	binitialized = false;
}

void CSMB::Init()
{
	if(!binitialized)
	{
		// set ip and subnet
		set_xbox_interface(g_stSettings.m_strLocalIPAdres, g_stSettings.m_strLocalNetmask);

		if(!smbc_init(xb_smbc_auth, 1/*Debug Level*/))
		{
			// set wins nameserver
			lp_do_parameter((-1), "wins server", g_stSettings.m_strNameServer);
			binitialized = true;
		}
	}
}

CSMB smb;

CFileSMB::CFileSMB()
{
	InitializeCriticalSection(&m_critSection);
	smb.Init();
	m_fd = -1;
}

CFileSMB::~CFileSMB()
{
	Close();
	DeleteCriticalSection(&m_critSection);
}

offset_t CFileSMB::GetPosition()
{
	if (m_fd == -1) return 0;
	::EnterCriticalSection(&m_critSection );
	int pos = smbc_lseek(m_fd, 0, SEEK_CUR);
	::LeaveCriticalSection(&m_critSection );
	if( pos < 0 )
		return 0;
	return (offset_t)pos;
}

offset_t CFileSMB::GetLength()
{
	if (m_fd == -1) return 0;
	return m_fileSize;
}

bool CFileSMB::Open(const char* strUserName, const char* strPassword,const char *strHostName, const char *strFileName,int iport, bool bBinary)
{
	char szFileName[1024];
	m_bBinary = bBinary;

	// since the syntax of the new smb is a little different, the workgroup is now specified
	// as workgroup;username:pass@pc/share (strUserName contains workgroup;username)
	// instead of username:pass@workgroup/pc/share
	// this means that if no password and username is provided szFileName doesn't have the workgroup.
	// should be fixed.

	if (strPassword && strUserName)
		sprintf(szFileName,"smb://%s:%s@%s/%s", strUserName, strPassword, strHostName, strFileName);
	else
		sprintf(szFileName,"smb://%s/%s", strHostName, strFileName);

	Close();
	::EnterCriticalSection(&m_critSection );

	m_fd = smbc_open(szFileName, O_RDONLY, 0);

	if(m_fd == -1)
	{
		::LeaveCriticalSection(&m_critSection );	
		return false;
	}
	INT64 ret = smbc_lseek(m_fd, 0, SEEK_END);

	if( ret < 0 )
	{
		smbc_close(m_fd);
		m_fd = -1;
		::LeaveCriticalSection(&m_critSection );	
		return false;
	}

	m_fileSize = ret;
	ret = smbc_lseek(m_fd, 0, SEEK_SET);
	if( ret < 0 )
	{
		smbc_close(m_fd);
		m_fd = -1;
		::LeaveCriticalSection(&m_critSection );	
		return false;
	}
	// We've successfully opened the file!
	::LeaveCriticalSection(&m_critSection );	
	return true;
}

unsigned int CFileSMB::Read(void *lpBuf, offset_t uiBufSize)
{
	if (m_fd == -1) return 0;
	::EnterCriticalSection(&m_critSection );	
	int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
	::LeaveCriticalSection(&m_critSection );	
	if( bytesRead <= 0 )
	{
		char szTmp[128];
		sprintf(szTmp,"SMB returned %i errno:%i\n",bytesRead,errno);
		OutputDebugString(szTmp);
		return 0;
	}
	return (unsigned int)bytesRead;
}

bool CFileSMB::ReadString(char *szLine, int iLineLength)
{
	if (m_fd == -1) return false;
	offset_t iFilePos=GetPosition();

	::EnterCriticalSection(&m_critSection );	
	int iBytesRead = smbc_read(m_fd, (unsigned char*)szLine, iLineLength);
	::LeaveCriticalSection(&m_critSection );	
	if (iBytesRead <= 0)
	{
		return false;
	}

	szLine[iBytesRead]=0;

	for (int i=0; i < iBytesRead; i++)
	{
		if ('\n' == szLine[i])
		{
			if ('\r' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
		else if ('\r'==szLine[i])
		{
			if ('\n' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
	}
	if (iBytesRead>0) 
	{
		return true;
	}
	return false;

}

offset_t CFileSMB::Seek(offset_t iFilePosition, int iWhence)
{
	if (m_fd == -1) return 0;

	::EnterCriticalSection(&m_critSection );	
	INT64 pos = smbc_lseek(m_fd, (int)iFilePosition, iWhence);
	::LeaveCriticalSection(&m_critSection );	

	if( pos < 0 )	return 0;

	return (offset_t)pos;
}

void CFileSMB::Close()
{
	if (m_fd != -1)
	{
		::EnterCriticalSection(&m_critSection );	
		smbc_close(m_fd);
		::LeaveCriticalSection(&m_critSection );	
	}
	m_fd = -1;
}

int CFileSMB::Write(const void* lpBuf, offset_t uiBufSize)
{
	if (m_fd == -1) return -1;

	DWORD dwNumberOfBytesWritten=0;

	// lpBuf can be safely casted to void* since xmbc_write will only read from it.
	::EnterCriticalSection(&m_critSection );	
	dwNumberOfBytesWritten = smbc_write(m_fd, (void*)lpBuf, (DWORD)uiBufSize);
	::LeaveCriticalSection(&m_critSection );

	return (int)dwNumberOfBytesWritten;
}
