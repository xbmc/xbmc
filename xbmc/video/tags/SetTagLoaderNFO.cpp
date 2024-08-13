/*
 *  Copyright (C) 2024 Team Kodi
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
#include "video/VideoInfoTag.h"

#include <utility>

#include <ServiceBroker.h>

using namespace XFILE;
using namespace ADDON;

CSetTagLoaderNFO::CSetTagLoaderNFO(const std::string& title) : ISetInfoTagLoader(title)
{
  if (!title.empty() && !KODI::VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(title).empty())
    m_path = URIUtils::AddFileToFolder(KODI::VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(title),
                                       "set.nfo");
}

bool CSetTagLoaderNFO::HasInfo() const
{
  return !m_path.empty() && CFileUtils::Exists(m_path);
}

CInfoScanner::INFO_TYPE CSetTagLoaderNFO::Load(CSetInfoTag& tag, const bool prioritise)
{
  CNfoFile nfoReader;
  AddonPtr addon;
  CServiceBroker::GetAddonMgr().GetAddon("metadata.local", addon, ADDON::OnlyEnabled::CHOICE_YES);
  const ScraperPtr scraper = std::dynamic_pointer_cast<CScraper>(addon);
  const CInfoScanner::INFO_TYPE result{nfoReader.Create(m_path, scraper)};

  if (result == CInfoScanner::FULL_NFO || result == CInfoScanner::COMBINED_NFO)
    nfoReader.GetDetails(tag, nullptr);

  // ** Get set name from originally passed if needed
  if (tag.GetTitle().empty())
    tag.SetTitle(m_title);

  std::string type;
  switch (result)
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
    CLog::LogF(LOGDEBUG, "Found matching {} NFO file: {}", type, CURL::GetRedacted(m_path));
  else
    CLog::LogF(LOGDEBUG, "No NFO file found. Using title search for '{}'",
               CURL::GetRedacted(m_path));

  return result;
}