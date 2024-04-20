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

#include <utility>

using namespace XFILE;

CVideoTagLoaderNFO::CVideoTagLoaderNFO(const CFileItem& item,
                                       ADDON::ScraperPtr info,
                                       bool lookInFolder)
  : IVideoInfoTagLoader(item, std::move(info), lookInFolder)
{
  if (m_info && m_info->Content() == CONTENT_TVSHOWS && m_item.m_bIsFolder)
    m_path = URIUtils::AddFileToFolder(m_item.GetPath(), "tvshow.nfo");
  else
    m_path = FindNFO(m_item, lookInFolder);
}

bool CVideoTagLoaderNFO::HasInfo() const
{
  return !m_path.empty() && CFileUtils::Exists(m_path);
}

CInfoScanner::INFO_TYPE CVideoTagLoaderNFO::Load(CVideoInfoTag& tag,
                                                 bool prioritise,
                                                 std::vector<EmbeddedArt>*)
{
  CNfoFile nfoReader;
  CInfoScanner::INFO_TYPE result = CInfoScanner::NO_NFO;
  if (m_info && m_info->Content() == CONTENT_TVSHOWS && !m_item.m_bIsFolder)
    result = nfoReader.Create(m_path, m_info, m_item.GetVideoInfoTag()->m_iEpisode);
  else if (m_info)
    result = nfoReader.Create(m_path, m_info);

  if (result == CInfoScanner::FULL_NFO || result == CInfoScanner::COMBINED_NFO)
    nfoReader.GetDetails(tag, nullptr, prioritise);

  if (result == CInfoScanner::URL_NFO || result == CInfoScanner::COMBINED_NFO)
  {
    m_url = nfoReader.ScraperUrl();
    m_info = nfoReader.GetScraperInfo();
  }

  std::string type;
  switch(result)
  {
    case CInfoScanner::COMBINED_NFO:
      type = "mixed";
      break;
    case CInfoScanner::FULL_NFO:
      type = "full";
      break;
    case CInfoScanner::URL_NFO:
      type = "URL";
      break;
    case CInfoScanner::NO_NFO:
      type = "";
      break;
    case CInfoScanner::OVERRIDE_NFO:
      type = "override";
      break;
    default:
      type = "malformed";
  }
  if (result != CInfoScanner::NO_NFO)
    CLog::Log(LOGDEBUG, "VideoInfoScanner: Found matching {} NFO file: {}", type,
              CURL::GetRedacted(m_path));
  else
    CLog::Log(LOGDEBUG, "VideoInfoScanner: No NFO file found. Using title search for '{}'",
              CURL::GetRedacted(m_item.GetPath()));

  return result;
}

std::string CVideoTagLoaderNFO::FindNFO(const CFileItem& item,
                                        bool movieFolder) const
{
  std::string nfoFile;
  // Find a matching .nfo file
  if (!item.m_bIsFolder)
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
        nfoFile = URIUtils::ReplaceExtension(item.GetPath(), ".nfo");
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
  if (item.m_bIsFolder || item.IsOpticalMediaFile() || (movieFolder && nfoFile.empty()))
  {
    // see if there is a unique nfo file in this folder, and if so, use that
    CFileItemList items;
    CDirectory dir;
    std::string strPath;
    if (item.m_bIsFolder)
      strPath = item.GetPath();
    else
      strPath = URIUtils::GetDirectory(item.GetPath());

    if (dir.GetDirectory(strPath, items, ".nfo", DIR_FLAG_DEFAULTS) && items.Size())
    {
      int numNFO = -1;
      for (int i = 0; i < items.Size(); i++)
      {
        if (items[i]->IsNFO())
        {
          if (numNFO == -1)
            numNFO = i;
          else
          {
            numNFO = -1;
            break;
          }
        }
      }
      if (numNFO > -1)
        return items[numNFO]->GetPath();
    }
  }

  return nfoFile;
}

