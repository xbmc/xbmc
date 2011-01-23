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
#include "EncoderFFmpeg.h"
#include "EncoderFlac.h"
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

bool CCDDARipper::Init(const CStdString& strTrackFile, const CStdString& strFile, const MUSIC_INFO::CMusicInfoTag& infoTag)
{
  m_cdReader.Init(strTrackFile);

  switch (g_guiSettings.GetInt("audiocds.encoder"))
  {
  case CDDARIP_ENCODER_WAV:
    m_pEncoder = new CEncoderWav();
    break;
  case CDDARIP_ENCODER_VORBIS:
    m_pEncoder = new CEncoderVorbis();
    break;
  case CDDARIP_ENCODER_FLAC:
    m_pEncoder = new CEncoderFlac();
    break;
  default:
    m_pEncoder = new CEncoderLame();
    break;
  }

  // we have to set the tags before we init the Encoder
  CStdString strTrack;
  strTrack.Format("%i", atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str()));

  m_pEncoder->SetComment("Ripped with XBMC");
  m_pEncoder->SetArtist(infoTag.GetArtist().c_str());
  m_pEncoder->SetTitle(infoTag.GetTitle().c_str());
  m_pEncoder->SetAlbum(infoTag.GetAlbum().c_str());
  m_pEncoder->SetAlbumArtist(infoTag.GetAlbumArtist().c_str());
  m_pEncoder->SetGenre(infoTag.GetGenre().c_str());
  m_pEncoder->SetTrack(strTrack.c_str());
  m_pEncoder->SetYear(infoTag.GetYearString().c_str());

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
bool CCDDARipper::Rip(const CStdString& strTrackFile, const CStdString& strFile, const MUSIC_INFO::CMusicInfoTag& infoTag)
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
  if (!Init(strTrackFile, strFilename, infoTag))
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

  if (bCancelled)
  {
    CLog::Log(LOGWARNING, "User Cancelled CDDA Rip");
    CFile::Delete(strFilename);
  }
  else CLog::Log(LOGINFO, "Finished ripping %s", strTrackFile.c_str());
  return !bCancelled;
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
  // don't rip non cdda items
  CStdString strExt;
  CUtil::GetExtension(pItem->m_strPath, strExt);
  if (strExt.CompareNoCase(".cdda") != 0) 
  {
    CLog::Log(LOGDEBUG, "cddaripper: file is not a cdda track");
    return false;
  }

  // construct directory where the track is stored
  CStdString strDirectory;
  int legalType;
  if (!CreateAlbumDir(*pItem->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  CStdString strFile = CUtil::AddFileToFolder(strDirectory, CUtil::MakeLegalFileName(GetTrackName(pItem), legalType));

  return Rip(pItem->m_strPath, strFile.c_str(), *pItem->GetMusicInfoTag());
}

bool CCDDARipper::RipCD()
{
  // return here if cd is not a CDDA disc
  MEDIA_DETECT::CCdInfo* pInfo = g_mediaManager.GetCdInfo();
  if (pInfo == NULL && !pInfo->IsAudio(1))
  {
    CLog::Log(LOGDEBUG, "cddaripper: CD is not an audio cd");
    return false;
  }

  // get cd cdda contents
  CFileItemList vecItems;
  XFILE::CCDDADirectory directory;
  directory.GetDirectory("cdda://local/", vecItems);

  // get cddb info
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

  // construct directory where the tracks are stored
  CStdString strDirectory;
  int legalType;
  if (!CreateAlbumDir(*vecItems[0]->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  // rip all tracks one by one, if one fails we quit and return false
  for (int i = 0; i < vecItems.Size(); i++)
  {
    CFileItemPtr item = vecItems[i];

    // construct filename
    CStdString strFile = CUtil::AddFileToFolder(strDirectory, CUtil::MakeLegalFileName(GetTrackName(item.get()), legalType));

    unsigned int tick = CTimeUtils::GetTimeMS();

    // don't rip non cdda items
    if (item->m_strPath.Find(".cdda") < 0)
      continue;

    // return false if Rip returned false (this means an error or the user cancelled
    if (!Rip(item->m_strPath, strFile.c_str(), *item->GetMusicInfoTag()))
      return false;

    tick = CTimeUtils::GetTimeMS() - tick;
    CLog::Log(LOGINFO, "Ripping Track %d took %s", i, StringUtils::SecondsToTimeString(tick / 1000).c_str());
  }

  CLog::Log(LOGINFO, "Ripped CD succesfull");
  return true;
}

const char* CCDDARipper::GetExtension(int iEncoder)
{
  if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
  if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
  if (iEncoder == CDDARIP_ENCODER_FLAC) return ".flac";
  return ".mp3";
}

bool CCDDARipper::CreateAlbumDir(const MUSIC_INFO::CMusicInfoTag& infoTag, CStdString& strDirectory, int& legalType)
{
  strDirectory = g_guiSettings.GetString("audiocds.recordingpath");
  CUtil::AddSlashAtEnd(strDirectory);

  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "Error: CDDARipPath has not been set");
    g_graphicsContext.Lock();
    CGUIDialogOK::ShowAndGetInput(257, 608, 609, 0);
    g_graphicsContext.Unlock();
    return false;
  }

  legalType = LEGAL_NONE;
  CFileItem ripPath(strDirectory, true);
  if (ripPath.IsSmb())
    legalType = LEGAL_WIN32_COMPAT;
#ifdef _WIN32
  if (ripPath.IsHD())
    legalType = LEGAL_WIN32_COMPAT;
#endif

  CStdString strAlbumDir = GetAlbumDirName(infoTag);

  if (!strAlbumDir.IsEmpty())
  {
    CUtil::AddFileToFolder(strDirectory, strAlbumDir, strDirectory);
    CUtil::AddSlashAtEnd(strDirectory);
  }

  CUtil::MakeLegalPath(strDirectory, legalType);

  // Create directory if it doesn't exist
  if (!CUtil::CreateDirectoryEx(strDirectory))
  {
    CLog::Log(LOGERROR, "Unable to create directory '%s'", strDirectory.c_str());
    return false;
  }

  return true;
}

CStdString CCDDARipper::GetAlbumDirName(const MUSIC_INFO::CMusicInfoTag& infoTag)
{
  CStdString strAlbumDir;

  // use audiocds.trackpathformat setting to format
  // directory name where CD tracks will be stored,
  // use only format part ending at the last '/'
  strAlbumDir = g_guiSettings.GetString("audiocds.trackpathformat");
  int pos = max(strAlbumDir.ReverseFind('/'), strAlbumDir.ReverseFind('\\'));
  if (pos < 0)
    return ""; // no directory
  
  strAlbumDir = strAlbumDir.Left(pos);

  // replace %A with album artist name
  if (strAlbumDir.Find("%A") != -1)
  {
    CStdString strAlbumArtist = infoTag.GetAlbumArtist();
    if (strAlbumArtist.IsEmpty())
      strAlbumArtist = infoTag.GetArtist();
    if (strAlbumArtist.IsEmpty())
      strAlbumArtist = "Unknown Artist";
    else
      strAlbumArtist.Replace('/', '_');
    strAlbumDir.Replace("%A", strAlbumArtist);
  }

  // replace %B with album title
  if (strAlbumDir.Find("%B") != -1)
  {
    CStdString strAlbum = infoTag.GetAlbum();
    if (strAlbum.IsEmpty()) 
      strAlbum.Format("Unknown Album %s", CDateTime::GetCurrentDateTime().GetAsLocalizedDateTime().c_str());
    else
      strAlbum.Replace('/', '_');
    strAlbumDir.Replace("%B", strAlbum);
  }

  // replace %G with genre
  if (strAlbumDir.Find("%G") != -1)
  {
    CStdString strGenre = infoTag.GetGenre();
    if (strGenre.IsEmpty())
      strGenre = "Unknown Genre";
    else
      strGenre.Replace('/', '_');
    strAlbumDir.Replace("%G", strGenre);
  }

  // replace %Y with year
  if (strAlbumDir.Find("%Y") != -1)
  {
    CStdString strYear = infoTag.GetYearString();
    if (strYear.IsEmpty())
      strYear = "Unknown Year";
    else
      strYear.Replace('/', '_');
    strAlbumDir.Replace("%Y", strYear);
  }

  return strAlbumDir;
}

CStdString CCDDARipper::GetTrackName(CFileItem *item)
{
  // get track number from "cdda://local/01.cdda"
  int trackNumber = atoi(item->m_strPath.substr(13, item->m_strPath.size() - 13 - 5).c_str());

  // Format up our ripped file label
  CFileItem destItem(*item);
  destItem.SetLabel("");

  // get track file name format from audiocds.trackpathformat setting,
  // use only format part starting from the last '/'
  CStdString strFormat = g_guiSettings.GetString("audiocds.trackpathformat");
  int pos = max(strFormat.ReverseFind('/'), strFormat.ReverseFind('\\'));
  if (pos != -1)
  {
    strFormat = strFormat.Right(strFormat.GetLength() - pos - 1);
  }

  CLabelFormatter formatter(strFormat, "");
  formatter.FormatLabel(&destItem);

  // grab the label to use it as our ripped filename
  CStdString track = destItem.GetLabel();
  if (track.IsEmpty())
    track.Format("%s%02i", "Track-", trackNumber);
  track += GetExtension(g_guiSettings.GetInt("audiocds.encoder"));

  return track;
}

#endif
