
#include "stdafx.h"
// FileSmb.cpp: implementation of the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////

#include "FileSmb.h"
#include "../sectionloader.h"
#include "../settings.h"
#include "../utils/log.h"
using namespace XFILE;

void xb_smbc_log(const char* msg)
{
	CLog::Log("%s%s", "smb: ", msg);
}

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen, 
			char *un, int unlen,  char *pw, int pwlen)
{
	return;
}

CSMB::CSMB()
{
	InitializeCriticalSection(&m_critSection);
	binitialized = false;
}

CSMB::~CSMB()
{
	DeleteCriticalSection(&m_critSection);
}

void CSMB::Init()
{
	if(!binitialized)
	{
		// set ip and subnet
		set_xbox_interface(g_stSettings.m_strLocalIPAdres, g_stSettings.m_strLocalNetmask);
		// set log function
		set_log_callback(xb_smbc_log);

		// set workgroup for samba, after smbc_init it can be freed();
		xb_setSambaWorkgroup(g_stSettings.m_strSambaWorkgroup);

		// initialize samba and do some hacking into the settings
		if(!smbc_init(xb_smbc_auth, g_stSettings.m_iSambaDebugLevel))
		{
			// if a wins-server is set, we have to change name resolve order to
			if (strlen(g_stSettings.m_strSambaWinsServer) > 1)
			{
				lp_do_parameter(-1, "wins server", g_stSettings.m_strSambaWinsServer);
				lp_do_parameter(-1, "name resolve order", "bcast wins");
			}
			else lp_do_parameter(-1, "name resolve order", "bcast");

			binitialized = true;
		}
	}
}

void CSMB::Purge()
{
	Lock();
	smbc_purge();
	Unlock();
}

void CSMB::Lock()
{
	::EnterCriticalSection(&m_critSection);
}

void CSMB::Unlock()
{
	::LeaveCriticalSection(&m_critSection);
}

CSMB smb;

CFileSMB::CFileSMB()
{
	smb.Init();
	m_fd = -1;
}

CFileSMB::~CFileSMB()
{
	Close();
}

__int64 CFileSMB::GetPosition()
{
	if (m_fd == -1) return 0;
	smb.Lock();
	__int64 pos = smbc_lseek(m_fd, 0, SEEK_CUR);
	smb.Unlock();
	if( pos < 0 )
		return 0;
	return pos;
}

__int64 CFileSMB::GetLength()
{
	if (m_fd == -1) return 0;
	return m_fileSize;
}

bool CFileSMB::Open(const CURL& url, bool bBinary)
{
	m_bBinary = bBinary;

	// since the syntax of the new smb is a little different, the workgroup is now specified
	// as workgroup;username:pass@pc/share (strUserName contains workgroup;username)
	// instead of username:pass@workgroup/pc/share
	// this means that if no password and username is provided szFileName doesn't have the workgroup.
	// should be fixed.

	// we can't open files like smb://file.f or smb://server/file.f
	
	if (!url.GetFileName().Find('/'))
	{
		m_fd = -1;
		return false;
	}
	CStdString strFileName;
	url.GetURL(strFileName);

	// convert from string to UTF8
	char strUtfFileName[1024];
	int strLen = convert_string(CH_DOS, CH_UTF8, strFileName.c_str(), (size_t)strFileName.length(), strUtfFileName, 1024, false);
	strUtfFileName[strLen] = 0;

	Close();

	smb.Lock();

	// opening a file to another computer share will create a new session
	// when opening smb://server xbms will try to find folder.jpg in all shares
	// listed, which will create lot's of open sessions.
	
	m_fd = smbc_open(strUtfFileName, O_RDONLY, 0);

	if(m_fd == -1)
	{
		// file failed to open. If we tried to open a file in a share
		// we should close all sessions available. It is slow, so don't
		// just do it if opening a file fails.
		char* cPos = strchr(strFileName, '/');
		if(cPos)
		{
			if (strlen(cPos) > 0 && !strchr(cPos+1, '/'))
			{
				//we have a file in a share, close sessions
				smbc_purge();
			}
		}
		smb.Unlock();
		// write error to logfile
		int nt_error = map_nt_error_from_unix(errno);
		CLog::Log("FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'",
				strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
		return false;
	}
	UINT64 ret = smbc_lseek(m_fd, 0, SEEK_END);

	if( ret < 0 )
	{
		smbc_close(m_fd);
		m_fd = -1;
		smb.Unlock();
		return false;
	}

	m_fileSize = ret;
	ret = smbc_lseek(m_fd, 0, SEEK_SET);
	if( ret < 0 )
	{
		smbc_close(m_fd);
		m_fd = -1;
		smb.Unlock();
		return false;
	}
	// We've successfully opened the file!
	smb.Unlock();
	return true;
}

bool CFileSMB::Exists(const CURL& url)
{
	if (url.GetFileName().Find('/')<0)
	{
		m_fd = -1;
		return false;
	}
	CStdString strFileName;
	url.GetURL(strFileName);

	// convert from string to UTF8
	char strUtfFileName[1024];
	int strLen = convert_string(CH_DOS, CH_UTF8, strFileName.c_str(), (size_t)strFileName.length(), strUtfFileName, 1024, false);
	strUtfFileName[strLen] = 0;

	struct __stat64 info;

	smb.Lock();
	int i = smbc_stat(strUtfFileName, &info);
	smb.Unlock();
	
	if (i<0) {
		m_fd = -1;
		return false;
	}
	return true;
}

int CFileSMB::Stat(const CURL& url, struct __stat64* buffer)
{
	if (!url.GetFileName().Find('/'))
	{
		m_fd = -1;
		return false;
	}
	CStdString strFileName;
	url.GetURL(strFileName);

	// convert from string to UTF8
	char strUtfFileName[1024];
	int strLen = convert_string(CH_DOS, CH_UTF8, strFileName.c_str(), (size_t)strFileName.length(), strUtfFileName, 1024, false);
	strUtfFileName[strLen] = 0;

	smb.Lock();
	int i = smbc_stat(strUtfFileName, buffer);
	smb.Unlock();
	return i;
}

unsigned int CFileSMB::Read(void *lpBuf, __int64 uiBufSize)
{
	if (m_fd == -1) return 0;
	smb.Lock();
	int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
	smb.Unlock();
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
	__int64 iFilePos=GetPosition();

	smb.Lock();	
	int iBytesRead = smbc_read(m_fd, (unsigned char*)szLine, iLineLength);
	smb.Unlock();
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

__int64 CFileSMB::Seek(__int64 iFilePosition, int iWhence)
{
	if (m_fd == -1) return -1;

	smb.Lock();	
	INT64 pos = smbc_lseek(m_fd, iFilePosition, iWhence);
	smb.Unlock();

	if( pos < 0 )	return -1;

	return (__int64)pos;
}

void CFileSMB::Close()
{
	if (m_fd != -1)
	{
		smb.Lock();
		smbc_close(m_fd);
		// we could close all sessions that are open now, but it will slow things down if we
		// have more open / close to do on the same server
		//smbc_purge();
		smb.Unlock();
	}
	m_fd = -1;
}

int CFileSMB::Write(const void* lpBuf, __int64 uiBufSize)
{
	if (m_fd == -1) return -1;

	DWORD dwNumberOfBytesWritten=0;

	// lpBuf can be safely casted to void* since xmbc_write will only read from it.
	smb.Lock();
	dwNumberOfBytesWritten = smbc_write(m_fd, (void*)lpBuf, (DWORD)uiBufSize);
	smb.Unlock();

	return (int)dwNumberOfBytesWritten;
}
