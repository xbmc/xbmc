// FileShoutcast.cpp: implementation of the CFileShoutcast class.
//
//////////////////////////////////////////////////////////////////////

#include "FileShoutcast.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include "../UI/configuration.h"
#include "libshout/types.h"
#include "libshout/rip_manager.h"
#include "libshout/util.h"
#include "libshout/filelib.h"
#include "ringbuffer.h"
#include <dialog.h>
#include "ShoutcastRipFile.h"

static CRingBuffer m_ringbuf;

static FileState m_fileState;
static CShoutcastRipFile m_ripFile;


extern CDialog g_dialog;

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


CFileShoutcast::CFileShoutcast()
{
	m_fileState.bBuffering=true;
	m_fileState.bRipDone=false;
	m_fileState.bRipStarted=false;
	m_fileState.bRipError = false;
	m_ringbuf.Create(1024*1024*5);
}

CFileShoutcast::~CFileShoutcast()
{
	m_ripFile.Reset();
	m_ringbuf.Clear();
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


offset_t CFileShoutcast::GetPosition()
{
	return 0;
}

offset_t CFileShoutcast::GetLength()
{
	return 0;
}


bool CFileShoutcast::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary)
{

	int ret;
	RIP_MANAGER_OPTIONS 		m_opt;
	m_opt.relay_port = 8000;
	m_opt.max_port = 18000;
	m_opt.flags = OPT_AUTO_RECONNECT | 
								OPT_SEPERATE_DIRS | 
								OPT_SEARCH_PORTS |
								OPT_ADD_ID3;


	strcpy(m_opt.output_directory, "./");
	m_opt.proxyurl[0] = (char)NULL;
	char szURL[1024];
	sprintf(szURL,"http://%s:%i%s", strHostName,iport,strFileName);
	strncpy(m_opt.url, szURL, MAX_URL_LEN);
	sprintf(m_opt.useragent, "x%s",strFileName);

	g_dialog.DoModalLess();
	g_dialog.SetCaption(0, "Shoutcast" );
	g_dialog.SetMessage(0, "Opening" );
	g_dialog.SetMessage(1, szURL );
	g_dialog.Render();
	

	if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS)
	{
		return false;
	}
	int iShoutcastTimeout = 10 * g_playerSettings.iShoutcastTimeout; //i.e: 10 * 10 = 100 * 100ms = 10s
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
			outputTimeoutMessage("Connection timed out...");
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
			g_dialog.SetCaption(0, "Shoutcast" );
			sprintf(szTmp,"Buffering %i bytes", m_ringbuf.GetMaxReadSize());
			g_dialog.SetMessage(0, szTmp );
			
			sprintf(szTmp,"%s",m_ripInfo.filename);
			for (int i=0; i < (int)strlen(szTmp); i++)
				szTmp[i]=tolower((unsigned char)szTmp[i]);
			szTmp[50]=0;
			g_dialog.SetMessage(1, szTmp);
			g_dialog.Render();
		}
		else //it's not really a connection timeout, but it's here, 
				 //where things get boring, if connection is slow.
				 //trust me, i did a lot of testing... Doesn't happen often, 
				 //but if it does it sucks to wait here forever.
				 //CHANGED: Other message here
		{
			outputTimeoutMessage("Connection to server to slow...");
			return false;
		}
		iCount++;
	}
	if ( m_fileState.bRipError )
	{
		//show it for 1.5 seconds
		g_dialog.SetCaption(0, "Error" );
		g_dialog.SetMessage(0, m_errorInfo.error_str );
		g_dialog.Render();
		Sleep(1500);
		return false;
	}
	return true;
}

unsigned int CFileShoutcast::Read(void* lpBuf, offset_t uiBufSize)
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
	return iRead;
}

void CFileShoutcast::outputTimeoutMessage(const char* message)
{
	g_dialog.SetCaption(0, "Shoutcast"  );
	g_dialog.SetMessage(0,  message );
	g_dialog.Render();
	Sleep(1500);
}

bool CFileShoutcast::ReadString(char *szLine, int iLineLength)
{
	return false;
}

offset_t CFileShoutcast::Seek(offset_t iFilePosition, int iWhence)
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