/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CDDARipper.h"

#include "CDDARipJob.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/CDDADirectory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "music/MusicLibraryQueue.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/windows/GUIControlSettings.h"
#include "storage/MediaManager.h"
#include "utils/LabelFormatter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace ADDON;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace KODI::MESSAGING;
using namespace KODI::CDRIP;

CCDDARipper& CCDDARipper::GetInstance()
{
  static CCDDARipper sRipper;
  return sRipper;
}

CCDDARipper::CCDDARipper() : CJobQueue(false, 1) //enforce fifo and non-parallel processing
{
}

CCDDARipper::~CCDDARipper() = default;

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
  // don't rip non cdda items
  if (!URIUtils::HasExtension(pItem->GetPath(), ".cdda"))
  {
    CLog::Log(LOGDEBUG, "CCDDARipper::{} - File '{}' is not a cdda track", __func__,
              pItem->GetPath());
    return false;
  }

  // construct directory where the track is stored
  std::string strDirectory;
  int legalType;
  if (!CreateAlbumDir(*pItem->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  std::string strFile = URIUtils::AddFileToFolder(
      strDirectory, CUtil::MakeLegalFileName(GetTrackName(pItem), legalType));

  AddJob(new CCDDARipJob(pItem->GetPath(), strFile, *pItem->GetMusicInfoTag(),
                         CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                             CSettings::SETTING_AUDIOCDS_ENCODER)));

  return true;
}

bool CCDDARipper::RipCD()
{
  // return here if cd is not a CDDA disc
  MEDIA_DETECT::CCdInfo* pInfo = CServiceBroker::GetMediaManager().GetCdInfo();
  if (pInfo == nullptr || !pInfo->IsAudio(1))
  {
    CLog::Log(LOGDEBUG, "CCDDARipper::{} - CD is not an audio cd", __func__);
    return false;
  }

  // get cd cdda contents
  CFileItemList vecItems;
  XFILE::CCDDADirectory directory;
  directory.GetDirectory(CURL("cdda://local/"), vecItems);

  // get cddb info
  for (int i = 0; i < vecItems.Size(); ++i)
  {
    CFileItemPtr pItem = vecItems[i];
    CMusicInfoTagLoaderFactory factory;
    std::unique_ptr<IMusicInfoTagLoader> pLoader(factory.CreateLoader(*pItem));
    if (nullptr != pLoader)
    {
      pLoader->Load(pItem->GetPath(), *pItem->GetMusicInfoTag()); // get tag from file
      if (!pItem->GetMusicInfoTag()->Loaded())
        break; //  No CDDB info available
    }
  }

  // construct directory where the tracks are stored
  std::string strDirectory;
  int legalType;
  if (!CreateAlbumDir(*vecItems[0]->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  // rip all tracks one by one
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  for (int i = 0; i < vecItems.Size(); i++)
  {
    CFileItemPtr item = vecItems[i];

    // construct filename
    std::string strFile = URIUtils::AddFileToFolder(
        strDirectory, CUtil::MakeLegalFileName(GetTrackName(item.get()), legalType));

    // don't rip non cdda items
    if (item->GetPath().find(".cdda") == std::string::npos)
      continue;

    bool eject =
        settings->GetBool(CSettings::SETTING_AUDIOCDS_EJECTONRIP) && i == vecItems.Size() - 1;
    AddJob(new CCDDARipJob(item->GetPath(), strFile, *item->GetMusicInfoTag(),
                           settings->GetInt(CSettings::SETTING_AUDIOCDS_ENCODER), eject));
  }

  return true;
}

bool CCDDARipper::CreateAlbumDir(const MUSIC_INFO::CMusicInfoTag& infoTag,
                                 std::string& strDirectory,
                                 int& legalType)
{
  std::shared_ptr<CSettingPath> recordingpathSetting = std::static_pointer_cast<CSettingPath>(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
          CSettings::SETTING_AUDIOCDS_RECORDINGPATH));
  if (recordingpathSetting != nullptr)
  {
    strDirectory = recordingpathSetting->GetValue();
    if (strDirectory.empty())
    {
      if (CGUIControlButtonSetting::GetPath(recordingpathSetting, &g_localizeStrings))
        strDirectory = recordingpathSetting->GetValue();
    }
  }
  URIUtils::AddSlashAtEnd(strDirectory);

  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "CCDDARipper::{} - Required path has not been set", __func__);
    HELPERS::ShowOKDialogText(CVariant{257}, CVariant{608});
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

  std::string strAlbumDir = GetAlbumDirName(infoTag);

  if (!strAlbumDir.empty())
  {
    strDirectory = URIUtils::AddFileToFolder(strDirectory, strAlbumDir);
    URIUtils::AddSlashAtEnd(strDirectory);
  }

  strDirectory = CUtil::MakeLegalPath(std::move(strDirectory), legalType);

  // Create directory if it doesn't exist
  if (!CUtil::CreateDirectoryEx(strDirectory))
  {
    CLog::Log(LOGERROR, "CCDDARipper::{} - Unable to create directory '{}'", __func__,
              strDirectory);
    return false;
  }

  return true;
}

std::string CCDDARipper::GetAlbumDirName(const MUSIC_INFO::CMusicInfoTag& infoTag)
{
  std::string strAlbumDir;

  // use audiocds.trackpathformat setting to format
  // directory name where CD tracks will be stored,
  // use only format part ending at the last '/'
  strAlbumDir = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_AUDIOCDS_TRACKPATHFORMAT);
  size_t pos = strAlbumDir.find_last_of("/\\");
  if (pos == std::string::npos)
    return ""; // no directory

  strAlbumDir.resize(pos);

  // replace %A with album artist name
  if (strAlbumDir.find("%A") != std::string::npos)
  {
    std::string strAlbumArtist = infoTag.GetAlbumArtistString();
    if (strAlbumArtist.empty())
      strAlbumArtist = infoTag.GetArtistString();
    if (strAlbumArtist.empty())
      strAlbumArtist = "Unknown Artist";
    else
      StringUtils::Replace(strAlbumArtist, '/', '_');
    StringUtils::Replace(strAlbumDir, "%A", strAlbumArtist);
  }

  // replace %B with album title
  if (strAlbumDir.find("%B") != std::string::npos)
  {
    std::string strAlbum = infoTag.GetAlbum();
    if (strAlbum.empty())
      strAlbum = StringUtils::Format("Unknown Album {}",
                                     CDateTime::GetCurrentDateTime().GetAsLocalizedDateTime());
    else
      StringUtils::Replace(strAlbum, '/', '_');
    StringUtils::Replace(strAlbumDir, "%B", strAlbum);
  }

  // replace %G with genre
  if (strAlbumDir.find("%G") != std::string::npos)
  {
    std::string strGenre = StringUtils::Join(
        infoTag.GetGenre(),
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
    if (strGenre.empty())
      strGenre = "Unknown Genre";
    else
      StringUtils::Replace(strGenre, '/', '_');
    StringUtils::Replace(strAlbumDir, "%G", strGenre);
  }

  // replace %Y with year
  if (strAlbumDir.find("%Y") != std::string::npos)
  {
    std::string strYear = infoTag.GetYearString();
    if (strYear.empty())
      strYear = "Unknown Year";
    else
      StringUtils::Replace(strYear, '/', '_');
    StringUtils::Replace(strAlbumDir, "%Y", strYear);
  }

  return strAlbumDir;
}

std::string CCDDARipper::GetTrackName(CFileItem* item)
{
  // get track number from "cdda://local/01.cdda"
  int trackNumber = atoi(item->GetPath().substr(13, item->GetPath().size() - 13 - 5).c_str());

  // Format up our ripped file label
  CFileItem destItem(*item);
  destItem.SetLabel("");

  // get track file name format from audiocds.trackpathformat setting,
  // use only format part starting from the last '/'
  std::string strFormat = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_AUDIOCDS_TRACKPATHFORMAT);
  size_t pos = strFormat.find_last_of("/\\");
  if (pos != std::string::npos)
    strFormat.erase(0, pos + 1);

  CLabelFormatter formatter(strFormat, "");
  formatter.FormatLabel(&destItem);

  // grab the label to use it as our ripped filename
  std::string track = destItem.GetLabel();
  if (track.empty())
    track = StringUtils::Format("{}{:02}", "Track-", trackNumber);

  const std::string encoder = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_AUDIOCDS_ENCODER);
  const AddonInfoPtr addonInfo =
      CServiceBroker::GetAddonMgr().GetAddonInfo(encoder, AddonType::AUDIOENCODER);
  if (addonInfo)
    track += addonInfo->Type(AddonType::AUDIOENCODER)->GetValue("@extension").asString();

  return track;
}

void CCDDARipper::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    if (CJobQueue::QueueEmpty())
    {
      std::string dir = URIUtils::GetDirectory(static_cast<CCDDARipJob*>(job)->GetOutput());
      bool unimportant;
      int source = CUtil::GetMatchingSource(
          dir, *CMediaSourceSettings::GetInstance().CMediaSourceSettings::GetSources("music"),
          unimportant);

      CMusicDatabase database;
      database.Open();
      if (source >= 0 && database.InsideScannedPath(dir))
        CMusicLibraryQueue::GetInstance().ScanLibrary(
            dir, MUSIC_INFO::CMusicInfoScanner::SCAN_NORMAL, false);

      database.Close();
    }
    return CJobQueue::OnJobComplete(jobID, success, job);
  }

  CancelJobs();
}
