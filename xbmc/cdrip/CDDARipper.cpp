#include "../stdafx.h"
#include "CDDARipper.h"
#include "CDDAReader.h"
#include "..\util.h"
#include "EncoderLame.h"
#include "EncoderWav.h"
#include "EncoderVorbis.h"
#include "..\filesystem\CDDADirectory.h"
#include "..\detectdvdtype.h"

CCDDARipper::CCDDARipper()
{
	m_pEncoder = NULL;
}

CCDDARipper::~CCDDARipper()
{
	if(m_pEncoder) delete m_pEncoder;
}

bool CCDDARipper::Init(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag* infoTag)
{
	m_cdReader.Init(strTrackFile);

	switch(g_guiSettings.GetInt("CDDARipper.Encoder"))
	{
	case CDDARIP_ENCODER_WAV:
		m_pEncoder = new CEncoderWav();
		break;
	case CDDARIP_ENCODER_VORBIS:
		m_pEncoder = new CEncoderVorbis();
		break;
	default:
		m_pEncoder = new CEncoderLame();
		break;
	}

	// we have to set the tags before we init the Encoder
	if (infoTag)
	{
		CStdString strTrack;
		strTrack.Format("%i", atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str()) + 1);

		m_pEncoder->SetComment("Ripped with XBMC");
		m_pEncoder->SetArtist(infoTag->GetArtist().c_str());
		m_pEncoder->SetTitle(infoTag->GetTitle().c_str());
		m_pEncoder->SetAlbum(infoTag->GetAlbum().c_str());
		m_pEncoder->SetGenre(infoTag->GetGenre().c_str());
		m_pEncoder->SetTrack(strTrack.c_str());
		m_pEncoder->SetYear(infoTag->GetYear().c_str());
	}

	// init encoder
	if(!m_pEncoder->Init(strFile.c_str(), 2, 44100, 16))
	{
		m_cdReader.DeInit();
		delete m_pEncoder;
		m_pEncoder = NULL;
		return false;
	}
	return true;
}

bool CCDDARipper::DeInit()
{
	// Close the encoder
	m_pEncoder->Close();

	m_cdReader.DeInit();

	if(m_pEncoder) delete m_pEncoder;
	m_pEncoder = NULL;

	return true;
}

int CCDDARipper::RipChunk(int& nPercent)
{
	BYTE* pbtStream = NULL;
	long lBytesRead = 0;

	// get data
	int iResult = m_cdReader.GetData(&pbtStream, lBytesRead);
	
	// return if rip is done or on some kind of error
	if (iResult != CDDARIP_OK) return iResult;

	// encode data
	m_pEncoder->Encode(lBytesRead, pbtStream);

	// Get progress indication
	nPercent = m_cdReader.GetPercent();

	return CDDARIP_OK;
}

// rip a single track from cd to hd
// strFileName has to be a valid filename and the directory must exist
bool CCDDARipper::Rip(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag& infoTag)
{
	int	iPercent, iOldPercent = 0;
	bool bCancelled = false;
	const char* strFilename = strFile.c_str();

	CLog::Log(LOGINFO, "Start ripping track %s to %s", strTrackFile, strFile);

  // if we are ripping to a samba share, rip it to hd first and then copy it it the share
	CFileItem file(strFile, false);
  if (file.IsRemote()) strFilename = tempnam("Z:\\", "");
  if (!strFilename)
  {
    CLog::Log(LOGERROR, "CCDDARipper: Error opening file");
    return false;
   }
  
	// init ripper
	if (!Init(strTrackFile, strFilename, &infoTag))
	{
		CLog::Log(LOGERROR, "Error: CCDDARipper::Init failed");
		return false;
	}

	// setup the progress dialog
	CGUIDialogProgress*	pDlgProgress	= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
	CStdStringW strLine0, strLine1;
	int iTrack=atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str()) + 1;
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
	while(!bCancelled && CDDARIP_DONE != RipChunk(iPercent))
	{
		pDlgProgress->ProgressKeys();
		bCancelled = pDlgProgress->IsCanceled();
		if (!bCancelled && iPercent > (iOldPercent + 2)) // update each 2%, it's a bit faster then every 1%
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
	
  if (file.IsRemote() && !bCancelled)
  {
    // copy the ripped track to the share
    CFile file;
    if (!file.Cache(strFilename, strFile.c_str()))
    {
      CLog::Log(LOGINFO, "Error copying file from %s to %s", strFilename, strFile);
		  // show error
		  g_graphicsContext.Lock();
		  CGUIDialogOK*	pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		  pDlgOK->SetHeading("Error copying");
		  pDlgOK->SetLine(0, CStdString(strFilename) + " to");
		  pDlgOK->SetLine(1, strFile);
		  pDlgOK->SetLine(2, "");
		  pDlgOK->DoModal(m_gWindowManager.GetActiveWindow());
		  g_graphicsContext.Unlock();
		  CFile::Delete(strFilename);
		  return false;
    }
    // delete cached file
    CFile::Delete(strFilename);
  }
  
	if (bCancelled) CLog::Log(LOGWARNING, "User Cancelled CDDA Rip");
	else CLog::Log(LOGINFO, "Finished ripping %s", strTrackFile);
	return !bCancelled;
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
	int iTrack = 0;
	CStdString strFile;
	CStdString strDirectory = g_stSettings.m_strRipPath;
	if (!CUtil::HasSlashAtEnd(strDirectory)) CUtil::AddDirectorySeperator(strDirectory);
	CFileItem ripPath(strDirectory, true);
	bool bIsFATX = !ripPath.IsSmb();

	if (pItem->m_strPath.Find(".cdda") < 0) return false;
	if (strDirectory.size() < 3)
	{
		// no rip path has been set, show error
		CLog::Log(LOGERROR, "Error: CDDARipPath has not been set in XboxMediaCenter.xml");
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
		strDirectory += CUtil::MakeLegalFileName(pItem->m_musicInfoTag.GetAlbum().c_str(), false, bIsFATX);
		CUtil::AddDirectorySeperator(strDirectory);
	}

	// Create directory if it doesn't exist
	CUtil::CreateDirectoryEx(strDirectory);

	// if title is set we use it, and modify it if needed
	char* cExt = GetExtension(g_guiSettings.GetInt("CDDARipper.Encoder"));
	if (pItem->m_musicInfoTag.GetTitle().size() > 0)
	{
		CStdString track;
		// do we want to include the track number in the file name?
		if (g_guiSettings.GetBool("CDDARipper.UseTrackNumber"))
			track.Format("%02i %s", iTrack, pItem->m_musicInfoTag.GetTitle().c_str());
		else
			track = pItem->m_musicInfoTag.GetTitle();

		strFile = strDirectory + CUtil::MakeLegalFileName((track + cExt).c_str(), true, bIsFATX);
	}
	else
		strFile.Format("%s%s%02i%s", strDirectory.c_str(), "Track-", iTrack, cExt);

	return Rip(pItem->m_strPath, strFile.c_str(), pItem->m_musicInfoTag);
}

bool CCDDARipper::RipCD()
{
	int iTrack = 0;
	bool bResult = true;
	CStdString strFile;
	CStdString strDirectory = g_stSettings.m_strRipPath;
	if (!CUtil::HasSlashAtEnd(strDirectory)) CUtil::AddDirectorySeperator(strDirectory);
	CFileItem ripPath(strDirectory, true);
	bool bIsFATX = !ripPath.IsSmb();

	// return here if cd is not a CDDA disc
	MEDIA_DETECT::CCdInfo* pInfo = MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
	if (pInfo == NULL && !pInfo->IsAudio(1)) return false;

	if (strDirectory.size() < 3)
	{
		// no rip path has been set, show error
		CLog::Log(LOGERROR, "Error: CDDARipPath has not been set in XboxMediaCenter.xml");
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
	{
		strDirectory += CUtil::MakeLegalFileName(vecItems[0]->m_musicInfoTag.GetAlbum().c_str(), false, bIsFATX);
		CUtil::AddDirectorySeperator(strDirectory);
  }		
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
				strDirectory += strDate;
				CUtil::AddDirectorySeperator(strDirectory);
				break;
			}
			iNumber++;
		}
	}

	// Create directory if it doesn't exist
	if (!CUtil::CreateDirectoryEx(strDirectory))
	{
	  CLog::Log(LOGERROR, "Unable to create directory '%s'", strDirectory.c_str());
	  return false;
	}

	// rip all tracks one by one, if one fails we quit and return false
	for (unsigned int i = 0; i < vecItems.size() && bResult == true; i++)
	{
		char* cExt = GetExtension(g_guiSettings.GetInt("CDDARipper.Encoder"));
		// get track number from "cdda://local/0.cdda"
		iTrack = atoi(vecItems[i]->m_strPath.substr(13, vecItems[i]->m_strPath.size() - 13 - 5).c_str()) + 1;
		// if title is set we use it, and modify it if needed
		if (vecItems[i]->m_musicInfoTag.GetTitle().size() > 0)
		{
			CStdString track;
			// do we want to include the track number in the file name?
			if (g_guiSettings.GetBool("CDDARipper.UseTrackNumber"))
				track.Format("%02i %s", iTrack, vecItems[i]->m_musicInfoTag.GetTitle().c_str());
			else
				track = vecItems[i]->m_musicInfoTag.GetTitle();

			strFile = strDirectory + CUtil::MakeLegalFileName((track + cExt).c_str(), true, bIsFATX);
		}
		else
			strFile.Format("%s%s%02i%s", strDirectory.c_str(), "Track-", iTrack, cExt);

		DWORD dwTick=timeGetTime();
		
		// return false if Rip returned false (this means an error or the user cancelled
		if (!Rip(vecItems[i]->m_strPath, strFile.c_str(), vecItems[i]->m_musicInfoTag)) return false;

		dwTick = timeGetTime() - dwTick;
		CStdString strTmp;
		CUtil::SecondsToHMSString(dwTick/1000, strTmp);
		CLog::Log(LOGINFO,"Ripping Track %d took %s", iTrack, strTmp.c_str());
	}
	CLog::Log(LOGINFO, "Ripped CD succesfull");
	return true;
}

char* CCDDARipper::GetExtension(int iEncoder)
{
	if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
	if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
	return ".mp3";
}