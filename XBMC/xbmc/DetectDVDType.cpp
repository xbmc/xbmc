#include "DetectDVDType.h"
#include "Filesystem/cdiosupport.h"
#include "xbox/undocumented.h"
#include "Settings.h"

using namespace XISO9660;

CMutex CDetectDVDMedia::m_muReadingMedia;
int CDetectDVDMedia::m_DriveState = DRIVE_CLOSED_NO_MEDIA;
CCdInfo* CDetectDVDMedia::m_pCdInfo = NULL;

CDetectDVDMedia::CDetectDVDMedia()
{
	m_bStop = false;
	m_dwLastTrayState=0;
}

CDetectDVDMedia::~CDetectDVDMedia()
{

}

void CDetectDVDMedia::OnStartup()
{
	SetPriority( THREAD_PRIORITY_LOWEST );
}

void CDetectDVDMedia::Process() 
{
	while ( !m_bStop ) {
		UpdateDvdrom();
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
	m_muReadingMedia.Wait();

	DWORD dwCurrentState;
	do
	{
		dwCurrentState = GetTrayState();
		switch(dwCurrentState)
		{
			case DRIVE_OPEN:
				{
				m_DriveState = DRIVE_OPEN;
				if ( m_pCdInfo != NULL ) {
					delete m_pCdInfo;
					m_pCdInfo = NULL;
				}
				//	Send Message to GUI that disc been ejected
				CGUIMessage msg( GUI_MSG_DVDDRIVE_EJECTED_CD, 0, 0, 0, 0, NULL );
				m_gWindowManager.SendMessage( msg );
				}
				break;
			case DRIVE_NOT_READY:
				// drive is not ready (closing, opening)
				m_DriveState = DRIVE_NOT_READY;
				Sleep(6000);
				break;
			case DRIVE_READY:
				// drive is ready
				//m_DriveState = DRIVE_READY;
				break;
			case DRIVE_CLOSED_NO_MEDIA:
				{
				// nothing in there...
				m_DriveState = DRIVE_CLOSED_NO_MEDIA;
				//	Send Message to GUI that disc has changed
				CGUIMessage msg( GUI_MSG_DVDDRIVE_CHANGED_CD, 0, 0, 0, 0, NULL );
				m_gWindowManager.SendMessage( msg );
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
					m_gWindowManager.SendMessage( msg );
				}
				break;
		}

	} while ( dwCurrentState != DRIVE_CLOSED_NO_MEDIA && dwCurrentState != DRIVE_READY && dwCurrentState != DRIVE_OPEN );

	//	We have finished media detection
	//	Signal for WaitMediaReady()
	m_muReadingMedia.Release();
}

//	Generates the drive url, (like iso9660://)
//	from the CCdInfo class
void CDetectDVDMedia::DetectMediaType()
{
	OutputDebugString( "Detecting DVD-ROM media filesystem...\n" );

	CStdString strNewUrl;
	CCdIoSupport cdio;
	//	Delete old CD-Information
	if ( m_pCdInfo != NULL ) {
		delete m_pCdInfo;
		m_pCdInfo = NULL;
	}

	//	Detect new CD-Information
	m_pCdInfo = cdio.GetCdInfo();
	if ( m_pCdInfo == NULL ) {
		OutputDebugString( "Detection of DVD-ROM media failed.\n" );
		return;
	}

	//	Detect ISO9660(mode1/mode2) or CDDA filesystem
	if ( m_pCdInfo->IsIso9660( 1 ) || m_pCdInfo->IsIso9660Interactive( 1 ) )
		strNewUrl = "iso9660://";
	else if ( m_pCdInfo->IsAudio( 1 ) )
		strNewUrl = "cdda://local/";
	else
		strNewUrl = "D:\\";

	char buf[256];
	sprintf( buf, "Using protocol %s\n", strNewUrl.c_str() );
	OutputDebugString( buf );

	SetNewDVDShareUrl( strNewUrl );
}

void CDetectDVDMedia::SetNewDVDShareUrl( CStdString strNewUrl ) 
{
	//	Set new URL for every share group

	//	My Music
	for (int i=0; i < (int)g_settings.m_vecMyMusicShares.size(); ++i)
	{
		if ( g_settings.m_vecMyMusicShares[i].m_iDriveType == SHARE_TYPE_DVD )
			g_settings.m_vecMyMusicShares[i].strPath = strNewUrl;
	}

	//	My Pictures
	for (i=0; i < (int)g_settings.m_vecMyPictureShares.size(); ++i)
	{
		if ( g_settings.m_vecMyPictureShares[i].m_iDriveType == SHARE_TYPE_DVD )
			g_settings.m_vecMyPictureShares[i].strPath = strNewUrl;
	}

	//	My Files
	for (i=0; i < (int)g_settings.m_vecMyFilesShares.size(); ++i)
	{
		if ( g_settings.m_vecMyFilesShares[i].m_iDriveType == SHARE_TYPE_DVD )
			g_settings.m_vecMyFilesShares[i].strPath = strNewUrl;
	}

	//	My Videos
	for (i=0; i < (int)g_settings.m_vecMyVideoShares.size(); ++i)
	{
		if ( g_settings.m_vecMyVideoShares[i].m_iDriveType == SHARE_TYPE_DVD )
			g_settings.m_vecMyVideoShares[i].strPath = strNewUrl;
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
	m_muReadingMedia.Wait();
	m_muReadingMedia.Release();
}

//	Static function
//	Whether a disc is in drive
bool CDetectDVDMedia::IsDiscInDrive()
{
	m_muReadingMedia.Wait();
	if ( m_DriveState != DRIVE_CLOSED_MEDIA_PRESENT ) {
		m_muReadingMedia.Release();
		return false;
	}

	m_muReadingMedia.Release();
	return true;
}

//	Static function
//	Returns a CCdInfo class, which contains
//	Media information of the current
//	inserted CD.
//	Can be NULL
CCdInfo* CDetectDVDMedia::GetCdInfo()
{
	m_muReadingMedia.Wait();
	CCdInfo* pCdInfo = m_pCdInfo;
	m_muReadingMedia.Release();
	return pCdInfo;
}
