#include "CDDARipper.h"
#include "..\utils\log.h"
#include "..\util.h"
#include "..\GUIDialogProgress.h"
#include "..\GUIDialogOK.h"
#include "..\guiWindowManager.h"
#include "..\settings.h"
#include "EncoderLame.h"
#include "EncoderWav.h"
#include "..\filesystem\CDDADirectory.h"
#include "..\detectdvdtype.h"
#include "..\localizestrings.h"

CCDDARipper::CCDDARipper()
{
	m_pEncoder = NULL;
	m_pbtStream = NULL;
}

CCDDARipper::~CCDDARipper()
{
	if(m_pEncoder) delete m_pEncoder;
	if(m_pbtStream) delete []m_pbtStream;
}

bool CCDDARipper::Init(int iTrack, const char* strFile)
{
	DWORD dwStartSector;
	DWORD dwEndSector;

	if (CDEX_OK != CR_Init(""))
	{
		CLog::Log("Error: CR_Init failed");
		return false;
	}

	// get and set cdrom params
	CR_GetCDROMParameters(&m_cdParams);
	//read around 128k per chunk instead of 0x1a which is the default. This makes the cd reading less noisy.
	m_cdParams.nNumReadSectors = 0x37; 
	CR_SetCDROMParameters(&m_cdParams);

	CR_ReadToc();
	dwStartSector = CR_GetTocEntry(iTrack - 1).dwStartSector;
	dwEndSector = CR_GetTocEntry(iTrack).dwStartSector-1;
	CLog::Log("Track %d, Sectors %d", CR_GetTocEntry(iTrack - 1).btTrackNumber,	dwEndSector - dwStartSector);

	// Open ripper
	if (CR_OpenRipper(&m_lBufferSize,	dwStartSector, dwEndSector) != CDEX_OK)
	{
		CLog::Log("Error: Failed to open ripper");
		return false;
	}

	// allocate buffer and setup a new encoder
	m_pbtStream = new BYTE[m_lBufferSize];
	if (g_stSettings.m_iRipEncoder == CDDARIP_ENCODER_LAME) m_pEncoder = new CEncoderLame();
	else if (g_stSettings.m_iRipEncoder == CDDARIP_ENCODER_WAV) m_pEncoder = new CEncoderWav();
	else m_pEncoder = new CEncoderLame(); // default

	// init encoder
	if(!m_pEncoder->Init(strFile))
	{
		delete []m_pbtStream;
		m_pbtStream = NULL;
		delete m_pEncoder;
		m_pEncoder = NULL;
		return false;
	}
	return true;
}

bool CCDDARipper::DeInit()
{
	// Close the Ripper session
	CR_CloseRipper();
	// Close the encoder
	m_pEncoder->Close();

	if(m_pEncoder) delete m_pEncoder;
	m_pEncoder = NULL;
	if(m_pbtStream) delete []m_pbtStream;
	m_pbtStream = NULL;

	if (CDEX_OK != CR_DeInit())
	{
		CLog::Log("Error: CR_DeInit failed");
		return false;
	}
	return true;
}

int CCDDARipper::RipChunk(int& nPercent, int& nPeakValue, int& nJitterErrors, int&	nJitterPos)
{
	CDEX_ERR ripErr;
	BOOL bAbort = false;
	long lBytesRead = 0;

	// Initialize incoming paramters
	nPercent = 0;
	nJitterErrors = 0;
	nPeakValue = 0;
	nJitterPos = 0;

	ripErr = CR_RipChunk(m_pbtStream, &lBytesRead, bAbort);

	if (CDEX_RIPPING_DONE == ripErr) return CDDARIP_DONE; 

	// Check for jitter errors
	if (CDEX_JITTER_ERROR == ripErr )
	{
		DWORD dwStartSector, dwEndSector;

		// Get info where jitter error did occur
		CR_GetLastJitterErrorPosition(dwStartSector,dwEndSector);
		// and do something usefull with it ? :)
	}

	if (CDEX_ERROR == ripErr)	return CDDARIP_ERR;

	m_pEncoder->Encode(lBytesRead, m_pbtStream);

	// Get progress indication
	nPercent = CR_GetPercentCompleted();
	// Get relative jitter position
	nJitterPos = CR_GetJitterPosition();
	// Get the Peak Value
	nPeakValue = CR_GetPeakValue();
	// Get the number of jitter errors
	nJitterErrors = CR_GetNumberOfJitterErrors();

	return CDDARIP_OK;
}

// rip a single track from cd to hd
// strFileName has to be a valid filename and the directory must exist
bool CCDDARipper::Rip(int iTrack, const char* strFile, MUSIC_INFO::CMusicInfoTag& infoTag)
{
	int	iPercent, iOldPercent = 0;
	int	iPeakValue = 0;
	int	iJitterErrors = 0;
	int	iJitterPos = 0;
	bool bCancled = false;

	CLog::Log("Start ripping track %i to %s", iTrack, strFile);

		// init ripper
	if (!Init(iTrack, strFile))
	{
		CLog::Log("Error: CCDDARipper::Init failed");
		return false;
	}

	SYSTEMTIME datetime;
	infoTag.GetReleaseDate(datetime);
	CStdString strDate, strTrack;
	if (datetime.wYear > 0) strDate.Format("%d", datetime.wYear);
	strTrack.Format("%i", iTrack);

	// add tags to our media file
	m_pEncoder->AddTag(ENC_COMMENT, "Ripped with XBMC");
	m_pEncoder->AddTag(ENC_ARTIST, infoTag.GetArtist().c_str());
	m_pEncoder->AddTag(ENC_TITLE, infoTag.GetTitle().c_str());
	m_pEncoder->AddTag(ENC_ALBUM, infoTag.GetAlbum().c_str());
	m_pEncoder->AddTag(ENC_GENRE, infoTag.GetGenre().c_str());
	m_pEncoder->AddTag(ENC_TRACK, strTrack.c_str());
	m_pEncoder->AddTag(ENC_YEAR, strDate.c_str());

	// setup the progress dialog
	CGUIDialogProgress*	pDlgProgress	= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
	CStdStringW strLine0, strLine1;
	strLine0.Format(L"%ls %i", g_localizeStrings.Get(606).c_str(), iTrack); // Track Number: %i
	strLine1.Format(L"%ls %hs", g_localizeStrings.Get(607).c_str(), strFile);// To: %s
	pDlgProgress->SetHeading(605); // Ripping
	pDlgProgress->SetLine(0, strLine0);
	pDlgProgress->SetLine(1, strLine1);
	pDlgProgress->SetLine(2, L"");
	pDlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
	pDlgProgress->ShowProgressBar(true);

	// show progress dialog
	g_graphicsContext.Lock();
	pDlgProgress->Progress();
	g_graphicsContext.Unlock();

	// start ripping
	while(!bCancled && CDDARIP_DONE != RipChunk(iPercent, iPeakValue, iJitterErrors, iJitterPos))
	{
		pDlgProgress->ProgressKeys();
		bCancled = pDlgProgress->IsCanceled();
		if (!bCancled && iPercent > (iOldPercent + 2)) // update each 2%, it's a bit faster then every 1%
		{
			// update dialog
			iOldPercent = iPercent;
			pDlgProgress->SetPercentage(iPercent);
			pDlgProgress->Progress();
		}
	}

	// close dialog and deinit ripper
	pDlgProgress->Close();
	DeInit();

	if (bCancled) CLog::Log("User Cancelled CDDA Rip");
	else CLog::Log("Finished ripping track %i", iTrack);
	return !bCancled;
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
	int iTrack = 0;
	CStdString strFile;
	CStdString strDirectory = g_stSettings.m_strRipPath;
	if (!CUtil::HasSlashAtEnd(strDirectory)) strDirectory += "\\";

	if (pItem->m_strPath.Find(".cdda") < 0) return false;
	if (strDirectory.size() < 3)
	{
		// no rip path has been set, show error
		CLog::Log("Error: CDDARipPath has not been set in XboxMediaCenter.xml");
		g_graphicsContext.Lock();
		CGUIDialogOK*	pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		pDlgOK->SetHeading(257); // Error
		pDlgOK->SetLine(0, 608); // Could not rip CD or Track
		pDlgOK->SetLine(1, 609); // CDDARipPath is not set in XboxMediaCenter.xml
		pDlgOK->SetLine(2, "");
		pDlgOK->DoModal(m_gWindowManager.GetActiveWindow());
		g_graphicsContext.Unlock();
		return false;
	}

	// get track number from "cdda://local/0.cdda"
	iTrack = atoi(pItem->m_strPath.substr(13, pItem->m_strPath.size() - 13 - 5).c_str()) + 1;

	// if album name is set, then we use this as the directory to place the new file in.
	if (pItem->m_musicInfoTag.GetAlbum().size() > 0)
	{
		strDirectory += CUtil::MakeLegalFATXFileName(pItem->m_musicInfoTag.GetAlbum().c_str(), false) + '\\';
	}

	// Create directory if it doesn't exist
	CUtil::CreateDirectoryEx(strDirectory);

	// if title is set we use it, and modify it if needed
	char* cExt = GetExtension(g_stSettings.m_iRipEncoder);
	if (pItem->m_musicInfoTag.GetTitle().size() > 0)
	{
		CStdString track;
		// do we want to include the track number in the file name?
		if (g_stSettings.m_bRipWithTrackNumber)
			track.Format("%02i %s", iTrack, pItem->m_musicInfoTag.GetTitle().c_str());
		else
			track = pItem->m_musicInfoTag.GetTitle();

		strFile = strDirectory + CUtil::MakeLegalFATXFileName((track + cExt).c_str(), true);
	}
	else
		strFile.Format("%s%s%02i%s", strDirectory.c_str(), "Track-", iTrack, cExt);

	return Rip(iTrack, strFile.c_str(), pItem->m_musicInfoTag);
}

bool CCDDARipper::RipCD()
{
	int iTrack = 0;
	bool bResult = true;
	CStdString strFile;
	CStdString strDirectory = g_stSettings.m_strRipPath;
	if (!CUtil::HasSlashAtEnd(strDirectory)) strDirectory += "\\";

	// return here if cd is not a CDDA disc
	MEDIA_DETECT::CCdInfo* pInfo = MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
	if (pInfo == NULL && !pInfo->IsAudio(1)) return false;

	if (strDirectory.size() < 3)
	{
		// no rip path has been set, show error
		CLog::Log("Error: CDDARipPath has not been set in XboxMediaCenter.xml");
		g_graphicsContext.Lock();
		CGUIDialogOK*	pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		pDlgOK->SetHeading(257); // Error
		pDlgOK->SetLine(0, 608); // Could not rip CD or Track
		pDlgOK->SetLine(1, 609); // CDDARipPath is not set in XboxMediaCenter.xml
		pDlgOK->SetLine(2, "");
		pDlgOK->DoModal(m_gWindowManager.GetActiveWindow());
		g_graphicsContext.Unlock();
		return false;
	}

	// get cd cdda contents
	VECFILEITEMS vecItems;
	DIRECTORY::CCDDADirectory directory;
	directory.GetDirectory("cdda://local/", vecItems);

	// if album name from first item is set,
	// then we use this as the directory to place the new file in.
	if (vecItems[0]->m_musicInfoTag.GetAlbum().size() > 0)
		strDirectory += CUtil::MakeLegalFATXFileName(vecItems[0]->m_musicInfoTag.GetAlbum().c_str(), false) + '\\';
	else
	{
		// create a directory based on current date
		SYSTEMTIME datetime;
		CStdString strDate;
		GetLocalTime(&datetime);
		int iNumber = 1;
		while (1)
		{
			strDate.Format("%04i-%02i-%02i-%i", datetime.wYear, datetime.wMonth, datetime.wDay, iNumber);
			if (!CUtil::FileExists(strDirectory + strDate))
			{
				strDirectory += strDate + "\\";
				break;
			}
			iNumber++;
		}
	}

	// Create directory if it doesn't exist
	CUtil::CreateDirectoryEx(strDirectory);

	// rip all tracks one by one, if one fails we quit and return false
	for (unsigned int i = 0; i < vecItems.size() && bResult == true; i++)
	{
		char* cExt = GetExtension(g_stSettings.m_iRipEncoder);
		// get track number from "cdda://local/0.cdda"
		iTrack = atoi(vecItems[i]->m_strPath.substr(13, vecItems[i]->m_strPath.size() - 13 - 5).c_str()) + 1;
		// if title is set we use it, and modify it if needed
		if (vecItems[i]->m_musicInfoTag.GetTitle().size() > 0)
		{
			CStdString track;
			// do we want to include the track number in the file name?
			if (g_stSettings.m_bRipWithTrackNumber)
				track.Format("%02i %s", iTrack, vecItems[i]->m_musicInfoTag.GetTitle().c_str());
			else
				track = vecItems[i]->m_musicInfoTag.GetTitle();

			strFile = strDirectory + CUtil::MakeLegalFATXFileName((track + cExt).c_str(), true);
		}
		else
			strFile.Format("%s%s%02i%s", strDirectory.c_str(), "Track-", iTrack, cExt);

		// return false if Rip returned false (this means an error or the user cancelled
		if (!Rip(iTrack, strFile.c_str(), vecItems[i]->m_musicInfoTag)) return false;
	}
	CLog::Log("Ripped CD succesfull");
	return true;
}

char* CCDDARipper::GetExtension(int iEncoder)
{
	if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
	if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
	return ".mp3";
}