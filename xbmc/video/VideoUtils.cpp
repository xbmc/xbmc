/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/ArtUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace KODI::VIDEO::UTILS
{

std::string FindTrailer(const CFileItem& item)
{
  std::string strFile2;
  std::string strFile = item.GetPath();
  if (item.IsStack())
  {
    std::string strPath;
    URIUtils::GetParentPath(item.GetPath(), strPath);
    XFILE::CStackDirectory dir;
    std::string strPath2;
    strPath2 = dir.GetStackedTitlePath(strFile);
    strFile = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strPath2));
    CFileItem sitem(dir.GetFirstStackedFile(item.GetPath()), false);
    std::string strTBNFile(URIUtils::ReplaceExtension(ART::GetTBNFile(sitem), "-trailer"));
    strFile2 = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strTBNFile));
  }
  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath, strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(item.GetPath()));
  }

  // no local trailer available for these
  if (NETWORK::IsInternetStream(item) || URIUtils::IsUPnP(strFile) || URIUtils::IsBluray(strFile) ||
      item.IsLiveTV() || item.IsPlugin() || item.IsDVD())
    return "";

  std::string strDir = URIUtils::GetDirectory(strFile);
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(
      strDir, items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
      XFILE::DIR_FLAG_READ_CACHE | XFILE::DIR_FLAG_NO_FILE_INFO | XFILE::DIR_FLAG_NO_FILE_DIRS);
  URIUtils::RemoveExtension(strFile);
  strFile += "-trailer";
  std::string strFile3 = URIUtils::AddFileToFolder(strDir, "movie-trailer");

  // Precompile our REs
  VECCREGEXP matchRegExps;
  CRegExp tmpRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strMatchRegExps =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_trailerMatchRegExps;

  for (const auto& strRegExp : strMatchRegExps)
  {
    if (tmpRegExp.RegComp(strRegExp))
      matchRegExps.push_back(tmpRegExp);
  }

  std::string strTrailer;
  for (int i = 0; i < items.Size(); i++)
  {
    std::string strCandidate = items[i]->GetPath();
    URIUtils::RemoveExtension(strCandidate);
    if (StringUtils::EqualsNoCase(strCandidate, strFile) ||
        StringUtils::EqualsNoCase(strCandidate, strFile2) ||
        StringUtils::EqualsNoCase(strCandidate, strFile3))
    {
      strTrailer = items[i]->GetPath();
      break;
    }
    else
    {
      for (auto& expr : matchRegExps)
      {
        if (expr.RegFind(strCandidate) != -1)
        {
          strTrailer = items[i]->GetPath();
          i = items.Size();
          break;
        }
      }
    }
  }

  return strTrailer;
}

std::string GetOpticalMediaPath(const CFileItem& item)
{
  auto exists = [&item](const std::string& file)
  {
    const std::string path = URIUtils::AddFileToFolder(item.GetPath(), file);
    return CFileUtils::Exists(path);
  };

  using namespace std::string_literals;
  const auto files = std::array{
      "VIDEO_TS.IFO"s,    "VIDEO_TS/VIDEO_TS.IFO"s,
#ifdef HAVE_LIBBLURAY
      "index.bdmv"s,      "INDEX.BDM"s,
      "BDMV/index.bdmv"s, "BDMV/INDEX.BDM"s,
#endif
  };

  const auto it = std::find_if(files.begin(), files.end(), exists);
  return it != files.end() ? URIUtils::AddFileToFolder(item.GetPath(), *it) : std::string{};
}

bool IsAutoPlayNextItem(const CFileItem& item)
{
  if (!item.HasVideoInfoTag())
    return false;

  return IsAutoPlayNextItem(item.GetVideoInfoTag()->m_type);
}

bool IsAutoPlayNextItem(const std::string& content)
{
  int settingValue = CSettings::SETTING_AUTOPLAYNEXT_UNCATEGORIZED;
  if (content == MediaTypeMovie || content == MediaTypeMovies ||
      content == MediaTypeVideoCollections)
    settingValue = CSettings::SETTING_AUTOPLAYNEXT_MOVIES;
  else if (content == MediaTypeEpisode || content == MediaTypeSeasons ||
           content == MediaTypeEpisodes)
    settingValue = CSettings::SETTING_AUTOPLAYNEXT_EPISODES;
  else if (content == MediaTypeMusicVideo || content == MediaTypeMusicVideos)
    settingValue = CSettings::SETTING_AUTOPLAYNEXT_MUSICVIDEOS;
  else if (content == MediaTypeTvShow || content == MediaTypeTvShows)
    settingValue = CSettings::SETTING_AUTOPLAYNEXT_TVSHOWS;

  const auto setting = std::dynamic_pointer_cast<CSettingList>(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
          CSettings::SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM));

  return setting && CSettingUtils::FindIntInList(setting, settingValue);
}

ResumeInformation GetItemResumeInformation(const CFileItem& item)
{
  // do not resume nfo files
  if (item.IsNFO())
    return {};

  // do not resume playlists, except strm files
  if (!item.IsType(".strm") && PLAYLIST::IsPlayList(item))
    return {};

  // do not resume Live TV and 'deleted' items (e.g. trashed pvr recordings)
  if (item.IsLiveTV() || item.IsDeleted())
    return {};

  int64_t startOffset{0};
  int partNumber{0};
  if (item.GetCurrentResumeTimeAndPartNumber(startOffset, partNumber) && startOffset > 0)
  {
    ResumeInformation resumeInfo;
    resumeInfo.startOffset = CUtil::ConvertSecsToMilliSecs(startOffset);
    resumeInfo.isResumable = true;
    return resumeInfo;
  }

  if (item.m_bIsFolder && item.IsResumable())
  {
    ResumeInformation resumeInfo;
    resumeInfo.isResumable = true;
    return resumeInfo;
  }

  return {};
}

ResumeInformation GetStackPartResumeInformation(const CFileItem& item, unsigned int partNumber)
{
  ResumeInformation resumeInfo;

  if (item.IsStack())
  {
    const std::string& path = item.GetDynPath();
    if (URIUtils::IsDiscImageStack(path))
    {
      // disc image stack
      CFileItemList parts;
      XFILE::CDirectory::GetDirectory(path, parts, "", XFILE::DIR_FLAG_DEFAULTS);

      resumeInfo = GetItemResumeInformation(*parts[partNumber - 1]);
      resumeInfo.partNumber = partNumber;
    }
    else
    {
      // video file stack
      CVideoDatabase db;
      if (!db.Open())
      {
        CLog::LogF(LOGERROR, "Cannot open VideoDatabase");
        return {};
      }

      std::vector<uint64_t> times;
      if (db.GetStackTimes(path, times))
      {
        resumeInfo.startOffset = times[partNumber - 1];
        resumeInfo.isResumable = (resumeInfo.startOffset > 0);
      }
      resumeInfo.partNumber = partNumber;
    }
  }
  return resumeInfo;
}

std::shared_ptr<CFileItem> LoadVideoFilesFolderInfo(const CFileItem& folder)
{
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open VideoDatabase");
    return {};
  }

  CFileItemList items;
  XFILE::CDirectory::GetDirectory(folder.GetDynPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);

  db.GetPlayCounts(items.GetPath(), items);

  std::shared_ptr<CFileItem> loadedItem{std::make_shared<CFileItem>(folder)};
  loadedItem->SetProperty("total", 0);
  loadedItem->SetProperty("watched", 0);
  loadedItem->SetProperty("unwatched", 0);
  loadedItem->SetProperty("inprogress", 0);

  for (const auto& item : items)
  {
    if (item->HasVideoInfoTag())
    {
      loadedItem->IncrementProperty("total", 1);
      if (item->GetVideoInfoTag()->GetPlayCount() == 0)
      {
        loadedItem->IncrementProperty("unwatched", 1);
      }
      else
      {
        loadedItem->IncrementProperty("watched", 1);
      }
      if (item->GetVideoInfoTag()->GetResumePoint().IsPartWay())
      {
        loadedItem->IncrementProperty("inprogress", 1);
      }
    }
  }
  return loadedItem;
}
} // namespace KODI::VIDEO::UTILS
