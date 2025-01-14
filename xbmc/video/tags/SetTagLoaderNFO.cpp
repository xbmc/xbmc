/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SetTagLoaderNFO.h"

#include "NfoFile.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/SetInfoTag.h"
#include "video/VideoInfoScanner.h"

#include <utility>

#include <ServiceBroker.h>

using namespace XFILE;
using namespace ADDON;

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
  CNfoFile nfoReader;
  AddonPtr addon;
  CServiceBroker::GetAddonMgr().GetAddon("metadata.local", addon, ADDON::OnlyEnabled::CHOICE_YES);
  const ScraperPtr scraper = std::dynamic_pointer_cast<CScraper>(addon);
  const CInfoScanner::InfoType result{nfoReader.Create(m_path, scraper)};

  if (result == CInfoScanner::InfoType::FULL || result == CInfoScanner::InfoType::COMBINED)
    nfoReader.GetDetails(tag, nullptr);

  // ** Get set name from originally passed if needed
  if (tag.GetTitle().empty())
    tag.SetTitle(m_title);

  std::string type;
  switch (result)
  {
    case CInfoScanner::InfoType::COMBINED:
      type = "mixed";
      break;
    case CInfoScanner::InfoType::FULL:
      type = "full";
      break;
    case CInfoScanner::InfoType::URL:
      type = "URL";
      break;
    case CInfoScanner::InfoType::NONE:
      type = "";
      break;
    case CInfoScanner::InfoType::OVERRIDE:
      type = "override";
      break;
    default:
      type = "malformed";
  }
  if (result != CInfoScanner::InfoType::NONE)
    CLog::LogF(LOGDEBUG, "Found matching {} NFO file: {}", type, CURL::GetRedacted(m_path));
  else
    CLog::LogF(LOGDEBUG, "No NFO file found. Using title search for '{}'",
               CURL::GetRedacted(m_path));

  return result;
}
