
#include "stdafx.h"
// FileShoutcast.cpp: implementation of the CFileShoutcast class.
//
//////////////////////////////////////////////////////////////////////

#include "FileShoutcast.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include "../settings.h"
#include "../lib/libshout/types.h"
#include "../lib/libshout/rip_manager.h"
#include "../lib/libshout/util.h"
#include "../lib/libshout/filelib.h"
#include "../GUIDialogProgress.h"
#include "../application.h"
#include "guiwindowmanager.h"
#include "ringbuffer.h"
//#include <dialog.h>
#include "ShoutcastRipFile.h"

const int SHOUTCASTTIMEOUT=100;
static CRingBuffer m_ringbuf;

static FileState m_fileState;
static CShoutcastRipFile m_ripFile;


static RIP_MANAGER_INFO m_ripInfo;
static ERROR_INFO m_errorInfo;



void rip_callback(int message, void *data)
{
	switch(message)
	{
		RIP_MANAGER_INFO *info;
		case RM_UPDATE:
			info = (RIP_MANAGER_INFO*)data;
			memcpy(&m_ripInfo,info,sizeof(m_ripInfo));
			if (info->status==RM_STATUS_BUFFERING)
			{
				m_fileState.bBuffering=true;
			}
			else if ( info->status==RM_STATUS_RIPPING)
			{
				m_ripFile.SetRipManagerInfo( &m_ripInfo );
				m_fileState.bBuffering=false;
			}
			else if (info->status==RM_STATUS_RECONNECTING)
			{
			}
			break;
		case RM_ERROR:
			ERROR_INFO *errInfo;
			errInfo = (ERROR_INFO*)data;
			memcpy(&m_errorInfo,errInfo,sizeof(m_errorInfo));
			m_fileState.bRipError=true;
			OutputDebugString("error\n");
			break;
		case RM_DONE:
			OutputDebugString("done\n");
			m_fileState.bRipDone=true;
			break;
		case RM_NEW_TRACK:
			char *trackName;
			trackName = (char*) data;
			m_ripFile.SetTrackname( trackName );
			break;
		case RM_STARTED:
			m_fileState.bRipStarted=true;
			OutputDebugString("Started\n");
			break;
	}

}

error_code filelib_write(char *buf, u_long size)
{
	while (m_ringbuf.GetMaxWriteSize() < (int)size) Sleep(100);
	m_ringbuf.WriteBinary(buf,size);
	m_ripFile.Write( buf, size ); //will only write, if it has to
	return SR_SUCCESS;
}
CFileShoutcast* m_pShoutCastRipper=NULL;

CFileShoutcast::CFileShoutcast()
{
	m_fileState.bBuffering=true;
	m_fileState.bRipDone=false;
	m_fileState.bRipStarted=false;
	m_fileState.bRipError = false;
	m_ringbuf.Create(1024*1024*5);
	m_pShoutCastRipper=this;
}

CFileShoutcast::~CFileShoutcast()
{
	m_pShoutCastRipper=NULL;
	m_ripFile.Reset();
	m_ringbuf.Destroy(); 
	rip_manager_stop();
}

bool CFileShoutcast::CanSeek()
{
	return false;
}

bool CFileShoutcast::CanRecord() 
{
	if ( !m_fileState.bRipStarted )
		return false;
	return m_ripFile.CanRecord();
}

bool CFileShoutcast::Record() 
{
	return m_ripFile.Record();
}

void CFileShoutcast::StopRecording() 
{
	m_ripFile.StopRecording();
}


__int64 CFileShoutcast::GetPosition()
{
	return 0;
}

__int64 CFileShoutcast::GetLength()
{
	return 0;
}


bool CFileShoutcast::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary)
{
	m_dwLastTime =timeGetTime();
	int ret;
	RIP_MANAGER_OPTIONS 		m_opt;
	m_opt.relay_port = 8000;
	m_opt.max_port = 18000;
	m_opt.flags = OPT_AUTO_RECONNECT | 
								OPT_SEPERATE_DIRS | 
								OPT_SEARCH_PORTS |
								OPT_ADD_ID3;

	CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

	strcpy(m_opt.output_directory, "./");
	m_opt.proxyurl[0] = (char)NULL;
	char szURL[1024];
	if ( strlen(strUserName)>0 && strlen(strPassword)>0 )
	{
		sprintf(szURL,"http://%s:%s@%s:%i/%s",strUserName,strPassword, strHostName,iport,strFileName);
	}
	else
	{
		sprintf(szURL,"http://%s:%i/%s", strHostName,iport,strFileName);
	}
	strncpy(m_opt.url, szURL, MAX_URL_LEN);
	sprintf(m_opt.useragent, "x%s",strFileName);
	if (dlgProgress)
  {
    dlgProgress->SetHeading(260);
	  dlgProgress->SetLine(0,259);
	  dlgProgress->SetLine(1,szURL);
	  dlgProgress->SetLine(2,"");
	  dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
	  dlgProgress->Progress();
  }
	
	if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS)
	{
		if (dlgProgress) dlgProgress->Close();
		return false;
	}
	int iShoutcastTimeout = 10 * SHOUTCASTTIMEOUT; //i.e: 10 * 10 = 100 * 100ms = 10s
	int iCount = 0;
	while (!m_fileState.bRipDone && !m_fileState.bRipStarted && !m_fileState.bRipError) 
	{
		if (iCount <= iShoutcastTimeout) //Normally, this isn't the problem, 
																		 //because if RIP_MANAGER fails, this would be here
																		 //with m_fileState.bRipError
		{
			Sleep(100);
		}
		else
		{
			if (dlgProgress)
      {
        dlgProgress->SetLine(1, 257); 
			  dlgProgress->SetLine(2,"Connection timed out...");
			  Sleep(1500);
			  dlgProgress->Close();
      }
			return false;
		}
		iCount++;
	}

	//CHANGED CODE: Don't reset timer anymore.

	while (!m_fileState.bRipDone && !m_fileState.bRipError && m_fileState.bBuffering) 
	{
		if (iCount <= iShoutcastTimeout) //Here is the real problem: Sometimes the buffer fills just to
																		 //slowly, thus the quality of the stream will be bad, and should be 
																		 //aborted...
		{
			Sleep(100);
			char szTmp[1024];
			//g_dialog.SetCaption(0, "Shoutcast" );
			sprintf(szTmp,"Buffering %i bytes", m_ringbuf.GetMaxReadSize());
			if (dlgProgress)
      {
        dlgProgress->SetLine(2,szTmp );
			  dlgProgress->Progress();
      }
			
			sprintf(szTmp,"%s",m_ripInfo.filename);
			for (int i=0; i < (int)strlen(szTmp); i++)
				szTmp[i]=tolower((unsigned char)szTmp[i]);
			szTmp[50]=0;
			if (dlgProgress) 
      {
        dlgProgress->SetLine(1,szTmp );
			  dlgProgress->Progress();
      }
		}
		else //it's not really a connection timeout, but it's here, 
				 //where things get boring, if connection is slow.
				 //trust me, i did a lot of testing... Doesn't happen often, 
				 //but if it does it sucks to wait here forever.
				 //CHANGED: Other message here
		{
			if (dlgProgress)
      {
        dlgProgress->SetLine(1, 257); 
			  dlgProgress->SetLine(2,"Connection to server to slow...");
			  dlgProgress->Close();
      }
			return false;
		}
		iCount++;
	}
	if ( m_fileState.bRipError )
	{
    if (dlgProgress)
    {
		  dlgProgress->SetLine(1,255);
		  dlgProgress->SetLine(2,m_errorInfo.error_str);
		  dlgProgress->Progress();

		  Sleep(1500);
		  dlgProgress->Close();
    }
		return false;
	}
	if (dlgProgress)
  {
    dlgProgress->SetLine(2,261);
	  dlgProgress->Progress();
	  dlgProgress->Close();
  }
	return true;
}

unsigned int CFileShoutcast::Read(void* lpBuf, __int64 uiBufSize)
{
	if (m_fileState.bRipDone) 
	{
		OutputDebugString("Read done\n");
		return 0;
	}
	while (m_ringbuf.GetMaxReadSize() <= 0) Sleep(100);
	int iRead=m_ringbuf.GetMaxReadSize();
	if (iRead > uiBufSize) iRead=(int)uiBufSize;
	m_ringbuf.ReadBinary((char*)lpBuf,iRead);

	if (timeGetTime() - m_dwLastTime > 500)
	{		
		m_dwLastTime =timeGetTime();
		ID3_Tag tag;
		GetID3TagInfo(tag);
		g_application.m_guiMusicOverlay.SetID3Tag(tag);
	}
	return iRead;
}

void CFileShoutcast::outputTimeoutMessage(const char* message)
{
	//g_dialog.SetCaption(0, "Shoutcast"  );
	//g_dialog.SetMessage(0,  message );
	//g_dialog.Render();
	Sleep(1500);
}

bool CFileShoutcast::ReadString(char *szLine, int iLineLength)
{
	return false;
}

__int64 CFileShoutcast::Seek(__int64 iFilePosition, int iWhence)
{
	return 0;
}

void CFileShoutcast::Close()
{
	OutputDebugString("Shoutcast Stopping\n");
	if ( m_ripFile.IsRecording() )
		m_ripFile.StopRecording();
	m_ringbuf.Clear();
	rip_manager_stop();
	m_ripFile.Reset();
	OutputDebugString("Shoutcast Stopped\n");
}

bool CFileShoutcast::IsRecording()
{
	return m_ripFile.IsRecording();
}



bool CFileShoutcast::GetID3TagInfo(ID3_Tag& tag)
{
	m_ripFile.GetID3Tag(tag);
	return true;
}