/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "system.h"

#ifdef HAS_CDDA_RIPPER

#include "CDDARipper.h"
#include "CDDARipJob.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "filesystem/CDDADirectory.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "utils/LabelFormatter.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/windows/GUIControlSettings.h"
#include "FileItem.h"
#include "filesystem/SpecialProtocol.h"
#include "storage/MediaManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "settings/MediaSourceSettings.h"
#include "Application.h"
#include "music/MusicDatabase.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;

CCDDARipper& CCDDARipper::GetInstance()
{
  static CCDDARipper sRipper;
  return sRipper;
}

CCDDARipper::CCDDARipper()
  : CJobQueue(false, 1) //enforce fifo and non-parallel processing
{
}

CCDDARipper::~CCDDARipper()
{
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
  // don't rip non cdda items
  if (!URIUtils::HasExtension(pItem->GetPath(), ".cdda"))
  {
    CLog::Log(LOGDEBUG, "cddaripper: file is not a cdda track");
    return false;
  }

  // construct directory where the track is stored
  CStdString strDirectory;
  int legalType;
  if (!CreateAlbumDir(*pItem->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  CStdString strFile = URIUtils::AddFileToFolder(strDirectory, 
                      CUtil::MakeLegalFileName(GetTrackName(pItem), legalType));

  AddJob(new CCDDARipJob(pItem->GetPath(),strFile,
                         *pItem->GetMusicInfoTag(),
                         CSettings::Get().GetInt("audiocds.encoder")));

  return true;
}

bool CCDDARipper::RipCD()
{
  // return here if cd is not a CDDA disc
  MEDIA_DETECT::CCdInfo* pInfo = g_mediaManager.GetCdInfo();
  if (pInfo == NULL || !pInfo->IsAudio(1))
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
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->GetPath()));
    if (NULL != pLoader.get())
    {
      pLoader->Load(pItem->GetPath(), *pItem->GetMusicInfoTag()); // get tag from file
      if (!pItem->GetMusicInfoTag()->Loaded())
        break;  //  No CDDB info available
    }
  }

  // construct directory where the tracks are stored
  CStdString strDirectory;
  int legalType;
  if (!CreateAlbumDir(*vecItems[0]->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  // rip all tracks one by one
  for (int i = 0; i < vecItems.Size(); i++)
  {
    CFileItemPtr item = vecItems[i];

    // construct filename
    CStdString strFile = URIUtils::AddFileToFolder(strDirectory, CUtil::MakeLegalFileName(GetTrackName(item.get()), legalType));

    // don't rip non cdda items
    if (item->GetPath().find(".cdda") == std::string::npos)
      continue;

    bool eject = CSettings::Get().GetBool("audiocds.ejectonrip") && 
                 i == vecItems.Size()-1;
    AddJob(new CCDDARipJob(item->GetPath(),strFile,
                           *item->GetMusicInfoTag(),
                           CSettings::Get().GetInt("audiocds.encoder"), eject));
  }

  return true;
}

const char* CCDDARipper::GetExtension(int iEncoder)
{
  if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
  if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
  if (iEncoder == CDDARIP_ENCODER_FLAC) return ".flac";
  if (iEncoder == CDDARIP_ENCODER_FFMPEG_M4A) return ".m4a";
  if (iEncoder == CDDARIP_ENCODER_FFMPEG_WMA) return ".wma";
  return ".mp3";
}

bool CCDDARipper::CreateAlbumDir(const MUSIC_INFO::CMusicInfoTag& infoTag, CStdString& strDirectory, int& legalType)
{
  CSettingPath *recordingpathSetting = (CSettingPath*)CSettings::Get().GetSetting("audiocds.recordingpath");
  if (recordingpathSetting != NULL)
  {
    strDirectory = recordingpathSetting->GetValue();
    if (strDirectory.empty())
    {
      if (CGUIControlButtonSetting::GetPath(recordingpathSetting))
        strDirectory = recordingpathSetting->GetValue();
    }
  }
  URIUtils::AddSlashAtEnd(strDirectory);

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
#ifdef TARGET_WINDOWS
  if (ripPath.IsHD())
    legalType = LEGAL_WIN32_COMPAT;
#endif

  CStdString strAlbumDir = GetAlbumDirName(infoTag);

  if (!strAlbumDir.empty())
  {
    strDirectory = URIUtils::AddFileToFolder(strDirectory, strAlbumDir);
    URIUtils::AddSlashAtEnd(strDirectory);
  }

  strDirectory = CUtil::MakeLegalPath(strDirectory, legalType);

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
  strAlbumDir = CSettings::Get().GetString("audiocds.trackpathformat");
  size_t pos = strAlbumDir.find_last_of("/\\");
  if (pos == std::string::npos)
    return ""; // no directory
  
  strAlbumDir = strAlbumDir.substr(0, pos);

  // replace %A with album artist name
  if (strAlbumDir.find("%A") != std::string::npos)
  {
    CStdString strAlbumArtist = StringUtils::Join(infoTag.GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator);
    if (strAlbumArtist.empty())
      strAlbumArtist = StringUtils::Join(infoTag.GetArtist(), g_advancedSettings.m_musicItemSeparator);
    if (strAlbumArtist.empty())
      strAlbumArtist = "Unknown Artist";
    else
      StringUtils::Replace(strAlbumArtist, '/', '_');
    StringUtils::Replace(strAlbumDir, "%A", strAlbumArtist);
  }

  // replace %B with album title
  if (strAlbumDir.find("%B") != std::string::npos)
  {
    CStdString strAlbum = infoTag.GetAlbum();
    if (strAlbum.empty())
      strAlbum = StringUtils::Format("Unknown Album %s", CDateTime::GetCurrentDateTime().GetAsLocalizedDateTime().c_str());
    else
      StringUtils::Replace(strAlbum, '/', '_');
    StringUtils::Replace(strAlbumDir, "%B", strAlbum);
  }

  // replace %G with genre
  if (strAlbumDir.find("%G") != std::string::npos)
  {
    CStdString strGenre = StringUtils::Join(infoTag.GetGenre(), g_advancedSettings.m_musicItemSeparator);
    if (strGenre.empty())
      strGenre = "Unknown Genre";
    else
      StringUtils::Replace(strGenre, '/', '_');
    StringUtils::Replace(strAlbumDir, "%G", strGenre);
  }

  // replace %Y with year
  if (strAlbumDir.find("%Y") != std::string::npos)
  {
    CStdString strYear = infoTag.GetYearString();
    if (strYear.empty())
      strYear = "Unknown Year";
    else
      StringUtils::Replace(strYear, '/', '_');
    StringUtils::Replace(strAlbumDir, "%Y", strYear);
  }

  return strAlbumDir;
}

CStdString CCDDARipper::GetTrackName(CFileItem *item)
{
  // get track number from "cdda://local/01.cdda"
  int trackNumber = atoi(item->GetPath().substr(13, item->GetPath().size() - 13 - 5).c_str());

  // Format up our ripped file label
  CFileItem destItem(*item);
  destItem.SetLabel("");

  // get track file name format from audiocds.trackpathformat setting,
  // use only format part starting from the last '/'
  CStdString strFormat = CSettings::Get().GetString("audiocds.trackpathformat");
  size_t pos = strFormat.find_last_of("/\\");
  if (pos != std::string::npos)
    strFormat.erase(0, pos+1);

  CLabelFormatter formatter(strFormat, "");
  formatter.FormatLabel(&destItem);

  // grab the label to use it as our ripped filename
  CStdString track = destItem.GetLabel();
  if (track.empty())
    track = StringUtils::Format("%s%02i", "Track-", trackNumber);
  track += GetExtension(CSettings::Get().GetInt("audiocds.encoder"));

  return track;
}

void CCDDARipper::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    if(CJobQueue::QueueEmpty())
    {
      CStdString dir = URIUtils::GetDirectory(((CCDDARipJob*)job)->GetOutput());
      bool unimportant;
      int source = CUtil::GetMatchingSource(dir, *CMediaSourceSettings::Get().CMediaSourceSettings::GetSources("music"), unimportant);

      CMusicDatabase database;
      database.Open();
      if (source>=0 && database.InsideScannedPath(dir))
        g_application.StartMusicScan(dir);
      database.Close();
    }
    return CJobQueue::OnJobComplete(jobID, success, job);
  }

  CancelJobs();
}

#endif
