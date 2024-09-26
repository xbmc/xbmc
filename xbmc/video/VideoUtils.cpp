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
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "pvr/filesystem/PVRGUIDirectory.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/ArtUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

using namespace KODI;

namespace
{
KODI::VIDEO::UTILS::ResumeInformation GetFolderItemResumeInformation(const CFileItem& item)
{
  if (!item.m_bIsFolder)
    return {};

  CFileItem folderItem(item);
  if (!folderItem.HasProperty("inprogressepisodes") && // season/show/recordings
      !folderItem.HasProperty("inprogress")) // movie set
  {
    if (URIUtils::IsPVRRecordingFileOrFolder(folderItem.GetPath()))
    {
      PVR::CPVRGUIDirectory::GetRecordingsDirectoryInfo(folderItem);
    }
    else
    {
      CVideoDatabase db;
      if (db.Open())
      {
        XFILE::VIDEODATABASEDIRECTORY::CQueryParams params;
        XFILE::VIDEODATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(item.GetPath(), params);

        if (params.GetTvShowId() >= 0)
        {
          if (params.GetSeason() >= 0)
          {
            const int idSeason = db.GetSeasonId(static_cast<int>(params.GetTvShowId()),
                                                static_cast<int>(params.GetSeason()));
            if (idSeason >= 0)
            {
              CVideoInfoTag details;
              db.GetSeasonInfo(idSeason, details, &folderItem);
            }
          }
          else
          {
            CVideoInfoTag details;
            db.GetTvShowInfo(item.GetPath(), details, static_cast<int>(params.GetTvShowId()),
                             &folderItem);
          }
        }
        else if (params.GetSetId() >= 0)
        {
          CVideoInfoTag details;
          db.GetSetInfo(static_cast<int>(params.GetSetId()), details, &folderItem);
        }
      }
    }
  }

  if (folderItem.IsResumable())
  {
    KODI::VIDEO::UTILS::ResumeInformation resumeInfo;
    resumeInfo.isResumable = true;
    return resumeInfo;
  }

  return {};
}

KODI::VIDEO::UTILS::ResumeInformation GetNonFolderItemResumeInformation(const CFileItem& item)
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

  KODI::VIDEO::UTILS::ResumeInformation resumeInfo;

  if (item.GetCurrentResumeTimeAndPartNumber(resumeInfo.startOffset, resumeInfo.partNumber))
  {
    if (resumeInfo.startOffset > 0)
    {
      resumeInfo.startOffset = CUtil::ConvertSecsToMilliSecs(resumeInfo.startOffset);
      resumeInfo.isResumable = true;
    }
  }
  else
  {
    // Obtain the resume bookmark from video db...

    CVideoDatabase db;
    if (!db.Open())
    {
      CLog::LogF(LOGERROR, "Cannot open VideoDatabase");
      return {};
    }

    std::string path = item.GetPath();
    if (VIDEO::IsVideoDb(item) || item.IsDVD())
    {
      if (item.HasVideoInfoTag())
      {
        path = item.GetVideoInfoTag()->m_strFileNameAndPath;
      }
      else if (VIDEO::IsVideoDb(item))
      {
        // Obtain path+filename from video db
        XFILE::VIDEODATABASEDIRECTORY::CQueryParams params;
        XFILE::VIDEODATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(item.GetPath(), params);

        long id = -1;
        VideoDbContentType content_type;
        if ((id = params.GetMovieId()) >= 0)
          content_type = VideoDbContentType::MOVIES;
        else if ((id = params.GetEpisodeId()) >= 0)
          content_type = VideoDbContentType::EPISODES;
        else if ((id = params.GetMVideoId()) >= 0)
          content_type = VideoDbContentType::MUSICVIDEOS;
        else
        {
          CLog::LogF(LOGERROR, "Cannot obtain video content type");
          db.Close();
          return {};
        }

        db.GetFilePathById(static_cast<int>(id), path, content_type);
      }
      else
      {
        // DVD
        CLog::LogF(LOGERROR, "Cannot obtain bookmark for DVD");
        db.Close();
        return {};
      }
    }

    CBookmark bookmark;
    db.GetResumeBookMark(path, bookmark);
    db.Close();

    if (bookmark.IsSet())
    {
      resumeInfo.isResumable = bookmark.IsPartWay();
      resumeInfo.startOffset = CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds);
      resumeInfo.partNumber = static_cast<int>(bookmark.partNumber);
    }
  }
  return resumeInfo;
}

} // unnamed namespace

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
  ResumeInformation info = GetNonFolderItemResumeInformation(item);
  if (info.isResumable)
    return info;

  return GetFolderItemResumeInformation(item);
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

} // namespace KODI::VIDEO::UTILS
