/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoTagLoaderNFO.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "NfoFile.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <ranges>
#include <utility>

using namespace XFILE;

namespace
{

std::string InfoTypeToStr(CInfoScanner::InfoType infoType)
{
  using enum CInfoScanner::InfoType;
  switch (infoType)
  {
    case COMBINED:
      return "mixed";
    case FULL:
      return "full";
    case URL:
      return "URL";
    case NONE:
      return "";
    case OVERRIDE:
      return "override";
    default:
      return "malformed";
  }
}

int GetNfoIndex(const CFileItem& item, const ADDON::ScraperPtr& scraper)
{
  if (scraper->Content() == ADDON::ContentType::MOVIES && !item.IsFolder() &&
      item.HasProperty("nfo_index"))
    return item.GetProperty("nfo_index").asInteger32(1); // multiple versions (playlists) in nfo
  return 1;
}

} // Unnamed namespace

CVideoTagLoaderNFO::CVideoTagLoaderNFO(const CFileItem& item,
                                       ADDON::ScraperPtr info,
                                       bool lookInFolder)
  : IVideoInfoTagLoader(item, std::move(info), lookInFolder)
{
  if (m_info && m_info->Content() == ADDON::ContentType::TVSHOWS && m_item.IsFolder())
    m_path = URIUtils::AddFileToFolder(m_item.GetPath(), "tvshow.nfo");
  else
    m_path = FindNFO(m_item, lookInFolder);
}

bool CVideoTagLoaderNFO::HasInfo() const
{
  return !m_path.empty() && CFileUtils::Exists(m_path);
}

CInfoScanner::InfoType CVideoTagLoaderNFO::Load(CVideoInfoTag& tag,
                                                bool prioritise,
                                                std::vector<EmbeddedArt>*)
{
  using enum CInfoScanner::InfoType;

  CInfoScanner::InfoType result = NONE;
  if (m_info)
  {
    CNfoFile nfoReader;
    result = nfoReader.Create(m_path, m_info, GetNfoIndex(m_item, m_info));

    if (result == FULL || result == COMBINED || result == OVERRIDE)
      nfoReader.GetDetails(tag, nullptr, prioritise);

    if (result == URL || result == COMBINED)
    {
      m_url = nfoReader.ScraperUrl();
      m_info = nfoReader.GetScraperInfo();
    }
  }

  if (result != NONE)
  {
    const std::string type{InfoTypeToStr(result)};
    if (m_item.HasProperty("nfo_index"))
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Found additional version ({}) in {} NFO file: {}",
                m_item.GetProperty("nfo_index").asInteger32(), type, CURL::GetRedacted(m_path));
    else
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Found matching {} NFO file: {}", type,
                CURL::GetRedacted(m_path));
  }
  else
  {
    if (m_item.HasProperty("nfo_index"))
      CLog::Log(LOGDEBUG, "VideoInfoScanner: No additional versions found in NFO file.");
    else
      CLog::Log(LOGDEBUG, "VideoInfoScanner: No NFO file found. Using title search for '{}'",
                CURL::GetRedacted(m_item.GetPath()));
  }

  return result;
}

std::string CVideoTagLoaderNFO::FindNFO(const CFileItem& item,
                                        bool movieFolder) const
{
  std::string nfoFile;
  // Find a matching .nfo file
  if (!item.IsFolder())
  {
    if (URIUtils::IsInRAR(item.GetPath())) // we have a rarred item - we want to check outside the rars
    {
      CFileItem item2(item);
      CURL url(item.GetPath());
      std::string strPath = URIUtils::GetDirectory(url.GetHostName());
      item2.SetPath(URIUtils::AddFileToFolder(strPath,
                                            URIUtils::GetFileName(item.GetPath())));
      return FindNFO(item2, movieFolder);
    }

    // grab the folder path
    std::string strPath = URIUtils::GetDirectory(item.GetPath());

    if (movieFolder && !item.IsStack())
    { // looking up by folder name - movie.nfo takes priority - but not for stacked items (handled below)
      nfoFile = URIUtils::AddFileToFolder(strPath, "movie.nfo");
      if (CFileUtils::Exists(nfoFile))
        return nfoFile;
    }

    // try looking for .nfo file for a stacked item
    if (item.IsStack())
    {
      // first try .nfo file matching first file in stack
      CStackDirectory dir;
      std::string firstFile = dir.GetFirstStackedFile(item.GetPath());
      CFileItem item2;
      item2.SetPath(firstFile);
      nfoFile = FindNFO(item2, movieFolder);
      // else try .nfo file matching stacked title
      if (nfoFile.empty())
      {
        std::string stackedTitlePath = dir.GetStackedTitlePath(item.GetPath());
        item2.SetPath(stackedTitlePath);
        nfoFile = FindNFO(item2, movieFolder);
      }
    }
    else
    {
      // already an .nfo file?
      if (URIUtils::HasExtension(item.GetPath(), ".nfo"))
        nfoFile = item.GetPath();
      // no, create .nfo file
      else
      {
        nfoFile = URIUtils::ReplaceExtension(item.GetPath(), ".nfo");

        // Look for specific SxxEyy nfo and use this if present
        if (item.HasVideoInfoTag())
        {
          const CVideoInfoTag* tag{item.GetVideoInfoTag()};
          if (tag->m_iSeason >= 0 && tag->m_iEpisode >= 0)
          {
            std::string file{item.GetPath()};
            URIUtils::RemoveExtension(file);
            file = fmt::format("{}-S{:02}E{:02}.nfo", file, tag->m_iSeason, tag->m_iEpisode);
            if (CFileUtils::Exists(file))
              nfoFile = file;
          }
        }
      }
    }

    // test file existence
    if (!nfoFile.empty() && !CFileUtils::Exists(nfoFile))
      nfoFile.clear();

    if (nfoFile.empty()) // final attempt - strip off any cd1 folders
    {
      URIUtils::RemoveSlashAtEnd(strPath); // need no slash for the check that follows
      CFileItem item2;
      if (StringUtils::EndsWithNoCase(strPath, "cd1"))
      {
        strPath.erase(strPath.size() - 3);
        item2.SetPath(URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(item.GetPath())));
        return FindNFO(item2, movieFolder);
      }
    }

    if (nfoFile.empty() && item.IsOpticalMediaFile())
    {
      CFileItem parentDirectory(item.GetLocalMetadataPath(), true);
      nfoFile = FindNFO(parentDirectory, true);
    }
  }

  // folders (or stacked dvds) can take any nfo file if there's a unique one
  if (nfoFile.empty() && (item.IsFolder() || item.IsOpticalMediaFile() || movieFolder))
  {
    // see if there is a unique nfo file in this folder, and if so, use that
    // if we are looking for a specific episode nfo the file name must end with SxxEyy
    // (otherwise it could match the wrong episode nfo)
    const std::string strPath{item.IsFolder() ? item.GetPath()
                                              : URIUtils::GetDirectory(item.GetPath())};
    CFileItemList items;
    if (CDirectory::GetDirectory(strPath, items, ".nfo", DIR_FLAG_DEFAULTS) && !items.IsEmpty())
    {
      const CVideoInfoTag* tag{item.GetVideoInfoTag()};
      auto nfoItems{items | std::views::filter(
                                [&tag](const auto& nfoItem)
                                {
                                  if (!nfoItem->IsNFO())
                                    return false;

                                  if (tag && tag->m_iSeason >= 0 && tag->m_iEpisode >= 0)
                                  {
                                    std::string path{nfoItem->GetPath()};
                                    const std::string extension{URIUtils::GetExtension(path)};
                                    if (!extension.empty())
                                      path.erase(path.size() - extension.size());
                                    return path.ends_with(fmt::format(
                                        "S{:02}E{:02}", tag->m_iSeason, tag->m_iEpisode));
                                  }

                                  return true;
                                })};
      if (std::ranges::distance(nfoItems) == 1)
        return (*nfoItems.begin())->GetPath();
    }
  }

  return nfoFile;
}
