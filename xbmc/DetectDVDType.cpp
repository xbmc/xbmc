
#include "stdafx.h"
#include "DetectDVDType.h"
#include "Filesystem/cdiosupport.h"
#include "Filesystem/iso9660.h"
#include "xbox/undocumented.h"
#include "Settings.h"
#include "playlistplayer.h"
#include "utils/log.h"
#include "localizeStrings.h"

using namespace MEDIA_DETECT;

CCriticalSection CDetectDVDMedia::m_muReadingMedia;
CEvent					 CDetectDVDMedia::m_evAutorun;
int							 CDetectDVDMedia::m_DriveState = DRIVE_CLOSED_NO_MEDIA;
CCdInfo*				 CDetectDVDMedia::m_pCdInfo = NULL;
time_t	CDetectDVDMedia::m_LastPoll = 0;
CDetectDVDMedia* CDetectDVDMedia::m_pInstance = NULL;

CDetectDVDMedia::CDetectDVDMedia()
{
	m_bAutorun = false;
	m_bStop = false;
	m_dwLastTrayState=0;
	m_bStartup = true;		//	Do not autorun on startup
	m_pInstance = this;
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
	if (g_stSettings.m_bUsePCDVDROM)
	{
		m_DriveState = DRIVE_CLOSED_MEDIA_PRESENT;
	}

	while (( !m_bStop ) && (!g_stSettings.m_bUsePCDVDROM)) 
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
          m_isoReader.Reset();
					m_DriveState = DRIVE_OPEN;
					SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(502));
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
          m_isoReader.Reset();
					SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(503));
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
            m_isoReader.Reset();
						m_DriveState = DRIVE_CLOSED_NO_MEDIA;
						SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(504));
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
	CLog::Log("Detecting DVD-ROM media filesystem...");

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
	if (m_pCdInfo==NULL) 
	{
		CLog::Log("Detection of DVD-ROM media failed.");
		return;
	}
	CLog::Log("Tracks overall:%i; Audio tracks:%i; Data tracks:%i",
	m_pCdInfo->GetTrackCount(),
	m_pCdInfo->GetAudioTrackCount(),
	m_pCdInfo->GetDataTrackCount() );

	//	Detect ISO9660(mode1/mode2), CDDA filesystem or UDF
	if (m_pCdInfo->IsISOHFS(1) || m_pCdInfo->IsIso9660(1) || m_pCdInfo->IsIso9660Interactive(1))
  {
		strNewUrl = "iso9660://";
    m_isoReader.Scan();
  }
	else
	{
		if (m_pCdInfo->IsUDF(1) || m_pCdInfo->IsUDFX(1))
			strNewUrl = "D:\\";
		else if (m_pCdInfo->IsAudio(1))
		{
			strNewUrl = "cdda://local/";
			bCDDA=true;
		}
		else
			strNewUrl = "D:\\";
	}

	if (m_pCdInfo->IsISOUDF(1))
	{
		if (g_stSettings.m_bDetectAsIso)
		{
			strNewUrl = "iso9660://";
			m_isoReader.Scan();
		}
		else
		{
			strNewUrl = "D:\\";
		}
	}

	CLog::Log("Using protocol %s", strNewUrl.c_str());

	if (m_pCdInfo->IsValidFs())
	{
		if (!m_pCdInfo->IsAudio(1))
			CLog::Log("Disc label: %s", m_pCdInfo->GetDiscLabel().c_str());
	}
	else
	{
		CLog::Log("Filesystem is not supported");
	}

  CStdString strLabel="";
	if (bCDDA)
	{
		strLabel="Audio-CD";
	}
	else
	{
		strLabel=m_pCdInfo->GetDiscLabel();
		strLabel.TrimRight(" ");
	}

  SetNewDVDShareUrl( strNewUrl ,bCDDA, strLabel);
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
			SetShareName(g_settings.m_vecMyMusicShares[i].strName, strDescription);
		}
	}

	//	My Pictures
	for (i=0; i < (int)g_settings.m_vecMyPictureShares.size(); ++i)
	{
		if ( g_settings.m_vecMyPictureShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyPictureShares[i].strPath = strNewUrl;
			SetShareName(g_settings.m_vecMyPictureShares[i].strName, strDescription);
		}
	}

	//	My Files
	for (i=0; i < (int)g_settings.m_vecMyFilesShares.size(); ++i)
	{
		if ( g_settings.m_vecMyFilesShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyFilesShares[i].strPath = strNewUrl;
			SetShareName(g_settings.m_vecMyFilesShares[i].strName, strDescription);
		}
	}

	//	My Videos
	for (i=0; i < (int)g_settings.m_vecMyVideoShares.size(); ++i)
	{
		if ( g_settings.m_vecMyVideoShares[i].m_iDriveType == SHARE_TYPE_DVD )
		{
			g_settings.m_vecMyVideoShares[i].strPath = strNewUrl;
			SetShareName(g_settings.m_vecMyVideoShares[i].strName, strDescription);
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

	if (g_stSettings.m_bUsePCDVDROM)
	{
		// allow the application to poll once every five seconds
		if ((clock()-m_LastPoll)>5000)
		{
			CLog::Log("Polling PC-DVDROM...");

			CIoSupport helper;
			if (helper.Remount("D:","Cdrom0")==S_OK)
			{
				if (m_pInstance)
				{
					m_pInstance->DetectMediaType();
				}
			}
			m_LastPoll = clock();
		}
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

// add status to sharename -> share (status)
void CDetectDVDMedia::SetShareName(CStdString& strShareName, CStdString& strStatus)
{
	// find last '(' if it's there and remove it. Needed so user can define
	// his own dvd share name in xboxmediacenter.xml
	CStdString strTemp = strShareName;
	int iPos = strTemp.ReverseFind('(') - 1;
	if (iPos > 0) strTemp.erase(iPos, strTemp.size());
	strShareName.Format("%s (%s)", strTemp.c_str(), strStatus.c_str());
}