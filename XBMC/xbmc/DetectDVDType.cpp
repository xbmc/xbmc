#include "DetectDVDType.h"
#include "Filesystem/cdiosupport.h"
#include "xbox/undocumented.h"
#include "Settings.h"
#include "playlistplayer.h"

using namespace MEDIA_DETECT;

CCriticalSection CDetectDVDMedia::m_muReadingMedia;
CEvent					 CDetectDVDMedia::m_evAutorun;
int							 CDetectDVDMedia::m_DriveState = DRIVE_CLOSED_NO_MEDIA;
CCdInfo*				 CDetectDVDMedia::m_pCdInfo = NULL;

CDetectDVDMedia::CDetectDVDMedia()
{
	m_bAutorun = false;
	m_bStop = false;
	m_dwLastTrayState=0;
	m_bStartup = true;		//	Do not autorun on startup
}

CDetectDVDMedia::~CDetectDVDMedia()
{

}

void CDetectDVDMedia::OnStartup()
{
//	SetPriority( THREAD_PRIORITY_LOWEST );
}

void CDetectDVDMedia::Process() 
{
	while ( !m_bStop ) 
	{
		Sleep(500);
		UpdateDvdrom();
		m_bStartup = false;
		if ( m_bAutorun ) 
		{
			m_evAutorun.Set();
			m_bAutorun = false;
		}
	}
}

void CDetectDVDMedia::OnExit() 
{

}

//	Gets state of the DVD drive
VOID CDetectDVDMedia::UpdateDvdrom()
{
	//	Signal for WaitMediaReady()
	//	that we are busy detecting the
	//	newly inserted media.
	{
		CSingleLock waitLock(m_muReadingMedia);

		DWORD dwCurrentState;
		
			dwCurrentState = GetTrayState();
			switch(dwCurrentState)
			{
				case DRIVE_OPEN:
				{
					m_DriveState = DRIVE_OPEN;
					SetNewDVDShareUrl( "D:\\" ,false,"(open)");
					//	Send Message to GUI that disc been ejected
					CGUIMessage msg( GUI_MSG_DVDDRIVE_EJECTED_CD, 0, 0, 0, 0, NULL );
					waitLock.Leave();
					m_gWindowManager.SendThreadMessage( msg );
					return;
				}
				break;
				
				case DRIVE_NOT_READY:
        {
					// drive is not ready (closing, opening)
					SetNewDVDShareUrl( "D:\\" ,false,"(busy)");
          m_DriveState = DRIVE_NOT_READY;
					//	DVD-ROM in undefined state
					//	better delete old CD Information
					if ( m_pCdInfo != NULL ) 
					{
						delete m_pCdInfo;
						m_pCdInfo = NULL;
					}
					waitLock.Leave();
					CGUIMessage msg( GUI_MSG_DVDDRIVE_CHANGED_CD, 0, 0, 0, 0, NULL );
					m_gWindowManager.SendThreadMessage( msg );
					Sleep(6000);
					return;
        }
        break;
				
				case DRIVE_READY:
					// drive is ready
					//m_DriveState = DRIVE_READY;
					return;
				break;
				case DRIVE_CLOSED_NO_MEDIA:
					{
						// nothing in there...
						m_DriveState = DRIVE_CLOSED_NO_MEDIA;
						SetNewDVDShareUrl( "D:\\" ,false,"(empty)");
            //	Send Message to GUI that disc has changed
						CGUIMessage msg( GUI_MSG_DVDDRIVE_CHANGED_CD, 0, 0, 0, 0, NULL );
						waitLock.Leave();
						m_gWindowManager.SendThreadMessage( msg );
						return;
					}
					break;
				case DRIVE_CLOSED_MEDIA_PRESENT:
					{
						m_DriveState = DRIVE_CLOSED_MEDIA_PRESENT;
						// drive has been closed and is ready
						OutputDebugString("Drive closed media present, remounting...\n");
						m_helper.Remount("D:","Cdrom0");
						//	Detect ISO9660(mode1/mode2) or CDDA filesystem
						DetectMediaType();
						CGUIMessage msg( GUI_MSG_DVDDRIVE_CHANGED_CD, 0, 0, 0, 0, (void*) m_pCdInfo );
						waitLock.Leave();
						m_gWindowManager.SendThreadMessage( msg );
						//	Tell the application object that a new Cd is inserted
						//	So autorun can be started.
						if ( !m_bStartup )
							m_bAutorun = true;
						return;
					}
					break;
			}

		//	We have finished media detection
		//	Signal for WaitMediaReady()
	}


}

//	Generates the drive url, (like iso9660://)
//	from the CCdInfo class
void CDetectDVDMedia::DetectMediaType()
{
	bool bCDDA(false);
	OutputDebugString( "Detecting DVD-ROM media filesystem...\n" );

	CStdString strNewUrl;
	CCdIoSupport cdio;
	//	Delete old CD-Information
	if ( m_pCdInfo != NULL ) 
	{
		delete m_pCdInfo;
		m_pCdInfo = NULL;
	}

	//	Detect new CD-Information
	m_pCdInfo = cdio.GetCdInfo();
	if ( m_pCdInfo == NULL ) 
	{
		OutputDebugString( "Detection of DVD-ROM media failed.\n" );
		return;
	}
	char szLog[128];
	sprintf(szLog,"tracks:%i audio tracks:%i data tracks:%i\n",
	m_pCdInfo->GetTrackCount(),
	m_pCdInfo->GetAudioTrackCount(),
	m_pCdInfo->GetDataTrackCount() );
	OutputDebugString(szLog);

	if (m_pCdInfo->IsUDF(1)||m_pCdInfo->IsUDFX(1))
		strNewUrl = "D:\\";
	else
	{
		//	Detect ISO9660(mode1/mode2) or CDDA filesystem
		if ( m_pCdInfo->IsIso9660( 1 ) || m_pCdInfo->IsIso9660Interactive( 1 ) )
			strNewUrl = "iso9660://";
		else if ( m_pCdInfo->IsAudio( 1 ) )
		{
			strNewUrl = "cdda://local/";
			bCDDA=true;
		}
		else
			strNewUrl = "D:\\";
	}

	char buf[256];
	sprintf( buf, "Using protocol %s\n", strNewUrl.c_str() );
	OutputDebugString( buf );

  sprintf( buf, "disc label:%s\n", m_pCdInfo->GetDiscLabel().c_str() );
	OutputDebugString( buf );

  SetNewDVDShareUrl( strNewUrl ,bCDDA, m_pCdInfo->GetDiscLabel());
}

void CDetectDVDMedia::SetNewDVDShareUrl( const CStdString& strNewUrl, bool bCDDA, const CStdString& strDiscLabel ) 
{
	CStdString strDescription="DVD";
	if (bCDDA) strDescription="CD";
  
  if (strDiscLabel!="") strDescription=strDiscLabel;

	//	Set new URL for every share group
	//	My Music
	for (int i=0; i < (int)g_settings.m_vecMyMusicShares.size(); ++i)
	{
		if ( g_settings.m_vecMyMusicShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyMusicShares[i].strPath = strNewUrl;
			g_settings.m_vecMyMusicShares[i].strName = strDescription;
		}
	}

	//	My Pictures
	for (i=0; i < (int)g_settings.m_vecMyPictureShares.size(); ++i)
	{
		if ( g_settings.m_vecMyPictureShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyPictureShares[i].strPath = strNewUrl;
			g_settings.m_vecMyPictureShares[i].strName = strDescription;
		}
	}

	//	My Files
	for (i=0; i < (int)g_settings.m_vecMyFilesShares.size(); ++i)
	{
		if ( g_settings.m_vecMyFilesShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyFilesShares[i].strPath = strNewUrl;
			g_settings.m_vecMyFilesShares[i].strName = strDescription;
		}
	}

	//	My Videos
	for (i=0; i < (int)g_settings.m_vecMyVideoShares.size(); ++i)
	{
		if ( g_settings.m_vecMyVideoShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyVideoShares[i].strPath = strNewUrl;
			g_settings.m_vecMyVideoShares[i].strName = strDescription;
		}
	}
}

DWORD CDetectDVDMedia::GetTrayState()
{
	HalReadSMCTrayState(&m_dwTrayState,&m_dwTrayCount);

	if(m_dwTrayState == TRAY_CLOSED_MEDIA_PRESENT) 
	{
		if (m_dwLastTrayState != TRAY_CLOSED_MEDIA_PRESENT)
		{
			m_dwLastTrayState = m_dwTrayState;
			return DRIVE_CLOSED_MEDIA_PRESENT;
		}
		else
		{
			return DRIVE_READY;
		}
	}
	else if(m_dwTrayState == TRAY_CLOSED_NO_MEDIA)
	{
		if (m_dwLastTrayState != TRAY_CLOSED_NO_MEDIA)
		{
			m_dwLastTrayState = m_dwTrayState;
			return DRIVE_CLOSED_NO_MEDIA;
		}
		else
		{
			return DRIVE_READY;
		}
	}
	else if(m_dwTrayState == TRAY_OPEN)
	{
		if (m_dwLastTrayState != TRAY_OPEN)
		{
			m_dwLastTrayState = m_dwTrayState;
			return DRIVE_OPEN;
		}
		else
		{
			return DRIVE_READY;
		}
	}
	else
	{
		m_dwLastTrayState = m_dwTrayState;
	}

	return DRIVE_NOT_READY;
}

//	Static function
//	Wait for drive, to finish media detection.
void CDetectDVDMedia::WaitMediaReady()
{
	CSingleLock waitLock(m_muReadingMedia);
}

//	Static function
//	Whether a disc is in drive
bool CDetectDVDMedia::IsDiscInDrive()
{
	CSingleLock waitLock(m_muReadingMedia);
	bool bResult = true;
	if ( m_DriveState != DRIVE_CLOSED_MEDIA_PRESENT ) {
		bResult = false;
	}
	return bResult;
}

//	Static function
//	Returns a CCdInfo class, which contains
//	Media information of the current
//	inserted CD.
//	Can be NULL
CCdInfo* CDetectDVDMedia::GetCdInfo()
{
	CSingleLock waitLock(m_muReadingMedia);
	CCdInfo* pCdInfo = m_pCdInfo;
	return pCdInfo;
}
