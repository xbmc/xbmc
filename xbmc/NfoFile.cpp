/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NfoFile.h"

#include "FileItemList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "video/VideoInfoDownloader.h"

#include <string>
#include <vector>

using namespace XFILE;
using namespace ADDON;

// Forward declarations
namespace
{
std::vector<ScraperPtr> GetScrapers(AddonType type, const ScraperPtr& selectedScraper);

} // unnamed namespace

CInfoScanner::InfoType CNfoFile::TryParsing(ADDON::AddonType addonType) const
{
  using enum CInfoScanner::InfoType;
  using enum ADDON::AddonType;

  if (addonType == SCRAPER_ALBUMS)
  {
    CAlbum album;
    return GetDetails(album) ? FULL : NONE;
  }
  if (addonType == SCRAPER_ARTISTS)
  {
    CArtist artist;
    return GetDetails(artist) ? FULL : NONE;
  }
  if (addonType == SCRAPER_MOVIES || addonType == SCRAPER_TVSHOWS ||
      addonType == SCRAPER_MUSICVIDEOS)
  {
    if (CVideoInfoTag details; GetDetails(details))
      return details.GetOverride() ? OVERRIDE : FULL;
  }
  return NONE;
}

bool CNfoFile::SeekToMovieIndex(int index)
{
  // Find nth <movie> tag (index is 1 based)
  m_headPos = m_doc.find("<movies>");
  while (index-- > 0)
  {
    m_headPos = m_doc.find("<movie", m_headPos + 1);
    if (m_headPos == std::string::npos)
      break;
  }
  return m_headPos != std::string::npos;
}

CInfoScanner::InfoType CNfoFile::TryParsing(const CURL& nfoPath,
                                            ADDON::ContentType contentType,
                                            int index /* =1 */)
{
  if (Load(nfoPath) != 0) // Setup m_doc and m_headPos
    return CInfoScanner::InfoType::ERROR_NFO;

  const AddonType addonType = ScraperTypeFromContent(contentType);

  if (addonType == ADDON::AddonType::SCRAPER_MOVIES && !SeekToMovieIndex(index))
    return CInfoScanner::InfoType::NONE;

  return TryParsing(addonType);
}

CInfoScanner::InfoType CNfoFile::Create(const std::string& nfoPath,
                                        const ScraperPtr& info,
                                        int index)
{
  /* `TryParsing` creates a close approximation to the desired result.
   * The desired result would be knowing if any valid URLs have been
   * found in the NFO which could be interpreted by a scraper.
   * Determining if any valid URLs exist in the NFO file is an expensive
   * operation so should only be called if necessary. If the approximate
   * result from `TryParsing` is sufficient then use that result.
   *
   * Below is a table to show the result from `TryParsing` and the
   * potential desired results.
   *
   * | result   | Could be converted to:     |
   * | -------- | -------------------------- |
   * | NONE     | URL NONE                   |
   * | OVERRIDE | COMBINED OVERRIDE          |
   * | FULL     | COMBINED OVERRIDE FULL     |
   *
   * The following call to `SearchNfoForScraperUrls` will generate the
   * desired result from this approximation.
   *
   * This call is expensive as it encodes the NFO file into a URL param
   * and executes a python interpreter for each installed python scraper.
  */
  const CInfoScanner::InfoType result = TryParsing(CURL{nfoPath}, info->Content(), index);
  if (result == CInfoScanner::InfoType::ERROR_NFO)
    return CInfoScanner::InfoType::NONE;
  return SearchNfoForScraperUrls(result, info);
}

CInfoScanner::InfoType CNfoFile::SearchNfoForScraperUrls(CInfoScanner::InfoType parseResult,
                                                         const ScraperPtr& info)
{
  using enum CInfoScanner::InfoType;

  SetScraperInfo(info); // assume we can use these settings
  const AddonType addonType = ScraperTypeFromContent(info->Content());

  for (const auto& scraper : GetScrapers(addonType, info))
  {
    if (scraper->IsNoop())
    {
      m_scurl = CScraperUrl();
      break;
    }

    scraper->ClearCache();
    try
    {
      m_scurl = scraper->NfoUrl(m_doc);
    }
    catch (const CScraperError& sce)
    {
      CVideoInfoDownloader::ShowErrorDialog(sce);
      if (!sce.FAborted())
        return ERROR_NFO;
    }

    if (m_scurl.HasUrls())
      return (parseResult == FULL || parseResult == OVERRIDE) ? COMBINED : URL;
  }

  if (parseResult == FULL && m_doc.find("[scrape url]") != std::string::npos)
    return OVERRIDE;

  return parseResult;
}

int CNfoFile::Load(const CURL& nfoPath)
{
  Close();
  XFILE::CFile file;
  if (std::vector<uint8_t> buf; file.LoadFile(nfoPath, buf) > 0)
  {
    m_doc.assign(reinterpret_cast<char*>(buf.data()), buf.size());
    m_headPos = 0;
    return 0;
  }
  m_doc.clear();
  return 1;
}

void CNfoFile::Close()
{
  m_doc.clear();
  m_headPos = 0;
  m_scurl.Clear();
}

namespace
{

std::vector<ScraperPtr> GetScrapers(AddonType type, const ScraperPtr& selectedScraper)
{
  AddonPtr addon;
  ScraperPtr defaultScraper;
  if (CAddonSystemSettings::GetInstance().GetActive(type, addon))
    defaultScraper = std::dynamic_pointer_cast<CScraper>(addon);

  std::vector<ScraperPtr> vecScrapers;

  // add selected scraper - first priority
  if (selectedScraper)
    vecScrapers.push_back(selectedScraper);

  // Add all scrapers except selected and default
  VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetAddons(addons, type);

  for (auto& addon : addons)
  {
    ScraperPtr scraper = std::dynamic_pointer_cast<CScraper>(addon);

    // skip if scraper requires settings and there's nothing set yet
    if (scraper->RequiresSettings() && !scraper->HasUserSettings())
      continue;

    if ((!selectedScraper || selectedScraper->ID() != scraper->ID()) &&
         (!defaultScraper || defaultScraper->ID() != scraper->ID()))
      vecScrapers.push_back(scraper);
  }

  // add default scraper - not user selectable so it's last priority
  if (defaultScraper && (!selectedScraper ||
                          selectedScraper->ID() != defaultScraper->ID()) &&
      (!defaultScraper->RequiresSettings() || defaultScraper->HasUserSettings()))
    vecScrapers.push_back(defaultScraper);

  return vecScrapers;
}

} // unnamed namespace
