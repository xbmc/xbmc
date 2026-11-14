/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SetTagLoaderNFO.h"

#include "URL.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "video/SetInfoTag.h"
#include "video/VideoInfoScanner.h"

CSetTagLoaderNFO::CSetTagLoaderNFO(const std::string& title) : ISetInfoTagLoader(title)
{
  if (!title.empty() && !KODI::VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(title).empty())
  {
    const std::string msif{KODI::VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(title)};
    m_path = URIUtils::AddFileToFolder(msif, "set.nfo");
  }
}

bool CSetTagLoaderNFO::HasInfo() const
{
  return !m_path.empty() && CFileUtils::Exists(m_path);
}

CInfoScanner::InfoType CSetTagLoaderNFO::Load(CSetInfoTag& tag, const bool prioritise)
{
  // Read as it is, as no scraper reads sets: classified as a scraper's nfo, it read as an album's.
  // As before, what parses of a document with an error in it is read.
  CXBMCTinyXML doc;
  doc.LoadFile(m_path);
  const bool loaded{tag.Load(doc.RootElement(), true, prioritise)};

  // ** Get set name from originally passed if needed
  if (tag.GetTitle().empty())
    tag.SetTitle(m_title);

  if (!loaded)
  {
    CLog::LogF(LOGDEBUG, "No NFO file found. Using title search for '{}'",
               CURL::GetRedacted(m_path));
    return CInfoScanner::InfoType::NONE;
  }

  CLog::LogF(LOGDEBUG, "Found matching full NFO file: {}", CURL::GetRedacted(m_path));
  return CInfoScanner::InfoType::FULL;
}
