/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#ifdef HAS_CDDA_RIPPER

#include "CDDARipper.h"
#include "CDDAReader.h"
#include "StringUtils.h"
#include "Util.h"
#include "EncoderLame.h"
#include "EncoderWav.h"
#include "EncoderVorbis.h"
#include "FileSystem/CDDADirectory.h"
#include "MusicInfoTagLoaderFactory.h"
#include "utils/LabelFormatter.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogKeyboard.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "FileSystem/SpecialProtocol.h"
#include "MediaManager.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;

CCDDARipper::CCDDARipper()
{
  m_pEncoder = NULL;
}

CCDDARipper::~CCDDARipper()
{
  delete m_pEncoder;
}

bool CCDDARipper::Init(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag* infoTag)
{
  m_cdReader.Init(strTrackFile);

  switch (g_guiSettings.GetInt("cddaripper.encoder"))
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
    strTrack.Format("%i", atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str()));

    m_pEncoder->SetComment("Ripped with XBMC");
    m_pEncoder->SetArtist(infoTag->GetArtist().c_str());
    m_pEncoder->SetTitle(infoTag->GetTitle().c_str());
    m_pEncoder->SetAlbum(infoTag->GetAlbum().c_str());
    m_pEncoder->SetGenre(infoTag->GetGenre().c_str());
    m_pEncoder->SetTrack(strTrack.c_str());
    m_pEncoder->SetYear(infoTag->GetYearString().c_str());
  }

  // init encoder
  CStdString strFile2=CUtil::MakeLegalPath(strFile);
  if (!m_pEncoder->Init(strFile2.c_str(), 2, 44100, 16))
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

  delete m_pEncoder;
  m_pEncoder = NULL;

  return true;
}

int CCDDARipper::RipChunk(int& nPercent)
{
  BYTE* pbtStream = NULL;
  long lBytesRead = 0;
  nPercent = 0;

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
  int iPercent, iOldPercent = 0;
  bool bCancelled = false;
  CStdString strFilename(strFile);

  CLog::Log(LOGINFO, "Start ripping track %s to %s", strTrackFile.c_str(), strFile.c_str());

  // if we are ripping to a samba share, rip it to hd first and then copy it it the share
  CFileItem file(strFile, false);
  if (file.IsRemote()) 
  {
    char tmp[MAX_PATH];
#ifndef _LINUX
    GetTempFileName(_P("special://temp/"), "riptrack", 0, tmp);
#else
    int fd;
    strncpy(tmp, _P("special://temp/riptrackXXXXXX"), MAX_PATH);
    if ((fd = mkstemp(tmp)) == -1)
      strFilename = "";
    close(fd);
#endif
    strFilename = tmp;
  }
  
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
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CStdString strLine0, strLine1;
  int iTrack = atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str());
  strLine0.Format("%s %i", g_localizeStrings.Get(606).c_str(), iTrack); // Track Number: %i
  strLine1.Format("%s %s", g_localizeStrings.Get(607).c_str(), strFile); // To: %s
  pDlgProgress->SetHeading(605); // Ripping
  pDlgProgress->SetLine(0, strLine0);
  pDlgProgress->SetLine(1, strLine1);
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal();
  pDlgProgress->ShowProgressBar(true);

  // show progress dialog
  g_graphicsContext.Lock();
  pDlgProgress->Progress();
  g_graphicsContext.Unlock();

  // start ripping
  while (!bCancelled && CDDARIP_DONE != RipChunk(iPercent))
  {
    pDlgProgress->ProgressKeys();
    bCancelled = pDlgProgress->IsCanceled();
    if (!bCancelled && iPercent > iOldPercent) // update each 2%, it's a bit faster then every 1%
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
    if (!CFile::Cache(strFilename, strFile.c_str()))
    {
      CLog::Log(LOGINFO, "Error copying file from %s to %s", strFilename.c_str(), strFile.c_str());
      // show error
      g_graphicsContext.Lock();
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      pDlgOK->SetHeading("Error copying");
      pDlgOK->SetLine(0, CStdString(strFilename) + " to");
      pDlgOK->SetLine(1, strFile);
      pDlgOK->SetLine(2, "");
      pDlgOK->DoModal();
      g_graphicsContext.Unlock();
      CFile::Delete(strFilename);
      return false;
    }
    // delete cached file
    CFile::Delete(strFilename);
  }

  if (bCancelled) {
    CLog::Log(LOGWARNING, "User Cancelled CDDA Rip");
    CFile::Delete(strFilename);
  }
  else CLog::Log(LOGINFO, "Finished ripping %s", strTrackFile.c_str());
  return !bCancelled;
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
  CStdString strDirectory = g_guiSettings.GetString("cddaripper.path");
  CUtil::AddSlashAtEnd(strDirectory);
  CFileItem ripPath(strDirectory, true);

  int LegalType = LEGAL_NONE;
  if (ripPath.IsSmb())
    LegalType=LEGAL_WIN32_COMPAT;
#ifdef _WIN32  
  if (ripPath.IsHD())
    LegalType=LEGAL_WIN32_COMPAT;
#endif
  
  if (pItem->m_strPath.Find(".cdda") < 0) return false;
  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "Error: CDDARipPath has not been set");
    g_graphicsContext.Lock();
    CGUIDialogOK::ShowAndGetInput(257, 608, 609, 0);
    g_graphicsContext.Unlock();
    return false;
  }

  // if album name is set, then we use this as the directory to place the new file in.
  if (!pItem->GetMusicInfoTag()->GetAlbum().empty())
  {
    strDirectory += CUtil::MakeLegalFileName(pItem->GetMusicInfoTag()->GetAlbum().c_str(), LegalType);
    CUtil::AddSlashAtEnd(strDirectory);
  }

  // Create directory if it doesn't exist
  CUtil::CreateDirectoryEx(strDirectory);

  CStdString strFile;
  CUtil::AddFileToFolder(strDirectory, GetTrackName(pItem, LegalType), strFile);

  return Rip(pItem->m_strPath, strFile.c_str(), *pItem->GetMusicInfoTag());
}

bool CCDDARipper::RipCD()
{
  int iTrack = 0;
  bool bResult = true;
  CStdString strFile;
  CStdString strDirectory = g_guiSettings.GetString("cddaripper.path");
  CUtil::AddSlashAtEnd(strDirectory);
  CFileItem ripPath(strDirectory, true);
  bool bIsFATX = !ripPath.IsSmb();

  // return here if cd is not a CDDA disc
  MEDIA_DETECT::CCdInfo* pInfo = g_mediaManager.GetCdInfo();
  if (pInfo == NULL && !pInfo->IsAudio(1))
  {
    CLog::Log(LOGDEBUG, "cddaripper: CD is not an audio cd");
    return false;
  }

  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "Error: CDDARipPath has not been set");
    g_graphicsContext.Lock();
    CGUIDialogOK::ShowAndGetInput(257, 608, 609, 0);
    g_graphicsContext.Unlock();
    return false;
  }

  // get cd cdda contents
  CFileItemList vecItems;
  DIRECTORY::CCDDADirectory directory;
  directory.GetDirectory("cdda://local/", vecItems);

  //  Get cddb info
  for (int i = 0; i < vecItems.Size(); ++i)
  {
    CFileItemPtr pItem = vecItems[i];
    CMusicInfoTagLoaderFactory factory;
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
    if (NULL != pLoader.get())
    {
      pLoader->Load(pItem->m_strPath, *pItem->GetMusicInfoTag()); // get tag from file
      if (!pItem->GetMusicInfoTag()->Loaded())
        break;  //  No CDDB info available
    }
  }

  // if album name from first item is set,
  // then we use this as the directory to place the new file in.
  CStdString strAlbumDir;
  if (!vecItems[0]->GetMusicInfoTag()->GetAlbum().empty())
  {
    int LegalType=LEGAL_NONE;
    if (ripPath.IsSmb())
      LegalType=LEGAL_WIN32_COMPAT;
#ifdef _WIN32
    if (ripPath.IsHD())
      LegalType=LEGAL_WIN32_COMPAT;
#endif
    strAlbumDir=CUtil::MakeLegalFileName(vecItems[0]->GetMusicInfoTag()->GetAlbum().c_str(), LegalType);
  }

    // No legal fatx directory name or no album in tag
  if (strAlbumDir.IsEmpty())
  {
    // create a directory based on current date
    SYSTEMTIME datetime;
    CStdString strDate;
    GetLocalTime(&datetime);
    int iNumber = 1;
    while (1)
    {
      strDate.Format("%04i-%02i-%02i-%i", datetime.wYear, datetime.wMonth, datetime.wDay, iNumber);
      if (!CGUIDialogKeyboard::ShowAndGetInput(strDate, g_localizeStrings.Get(748), false))
        return false;
      if (!CFile::Exists(strDirectory + strDate))
      {
        strAlbumDir=strDate;
        break;
      }
      iNumber++;
    }
  }

  // construct directory where the tracks are stored
  strDirectory += strAlbumDir;
  CUtil::AddSlashAtEnd(strDirectory);

  // Create directory if it doesn't exist
  if (!CUtil::CreateDirectoryEx(strDirectory))
  {
    CLog::Log(LOGERROR, "Unable to create directory '%s'", strDirectory.c_str());
    return false;
  }

  // rip all tracks one by one, if one fails we quit and return false
  for (int i = 0; i < vecItems.Size() && bResult == true; i++)
  {
    CFileItemPtr item = vecItems[i];
    CStdString track(GetTrackName(item.get(), bIsFATX));

    // construct filename
    CUtil::AddFileToFolder(strDirectory, track, strFile);

    unsigned int tick = CTimeUtils::GetTimeMS();

    // don't rip non cdda items
    if (item->m_strPath.Find(".cdda") < 0)
      continue;

    // return false if Rip returned false (this means an error or the user cancelled
    if (!Rip(item->m_strPath, strFile.c_str(), *item->GetMusicInfoTag())) return false;

    tick = CTimeUtils::GetTimeMS() - tick;
    CStdString strTmp;
    StringUtils::SecondsToTimeString(tick / 1000, strTmp);
    CLog::Log(LOGINFO, "Ripping Track %d took %s", iTrack, strTmp.c_str());
  }
  CLog::Log(LOGINFO, "Ripped CD succesfull");
  return true;
}

const char* CCDDARipper::GetExtension(int iEncoder)
{
  if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
  if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
  return ".mp3";
}

CStdString CCDDARipper::GetTrackName(CFileItem *item, int LegalType)
{
  // get track number from "cdda://local/01.cdda"
  int trackNumber = atoi(item->m_strPath.substr(13, item->m_strPath.size() - 13 - 5).c_str());

  // Format up our ripped file label
  CFileItem destItem(*item);
  destItem.SetLabel("");
  CLabelFormatter formatter(g_guiSettings.GetString("cddaripper.trackformat"), "");
  formatter.FormatLabel(&destItem);

  // grab the label to use it as our ripped filename
  CStdString track = destItem.GetLabel();
  if (track.IsEmpty())
    track.Format("%s%02i", "Track-", trackNumber);
  track += GetExtension(g_guiSettings.GetInt("cddaripper.encoder"));

  // make sure the filename is legal
  track = CUtil::MakeLegalFileName(track, LegalType);
  return track;
}

#endif
