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

#include "FileItem.h"
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

CInfoScanner::INFO_TYPE CNfoFile::Create(const std::string& strPath,
                                         const ScraperPtr& info, int episode)
{
  m_info = info; // assume we can use these settings
  m_type = ScraperTypeFromContent(info->Content());
  if (Load(strPath) != 0)
    return CInfoScanner::NO_NFO;

  CFileItemList items;
  bool bNfo=false;

  if (m_type == AddonType::SCRAPER_ALBUMS)
  {
    CAlbum album;
    bNfo = GetDetails(album);
  }
  else if (m_type == AddonType::SCRAPER_ARTISTS)
  {
    CArtist artist;
    bNfo = GetDetails(artist);
  }
  else if (m_type == AddonType::SCRAPER_TVSHOWS || m_type == AddonType::SCRAPER_MOVIES ||
           m_type == AddonType::SCRAPER_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_type == AddonType::SCRAPER_TVSHOWS)
    {
      int infos=0;
      while (m_headPos != std::string::npos && details.m_iEpisode != episode)
      {
        m_headPos = m_doc.find("<episodedetails", m_headPos);
        if (m_headPos == std::string::npos)
          break;

        bNfo  = GetDetails(details);
        infos++;
        m_headPos++;
      }
      if (details.m_iEpisode != episode)
      {
        bNfo = false;
        details.Reset();
        m_headPos = 0;
        if (infos == 1) // still allow differing nfo/file numbers for single ep nfo's
          bNfo = GetDetails(details);
      }
    }
  }

  std::vector<ScraperPtr> vecScrapers = GetScrapers(m_type, m_info);

  // search ..
  int res = -1;
  for (unsigned int i=0; i<vecScrapers.size(); ++i)
    if ((res = Scrape(vecScrapers[i], m_scurl, m_doc)) == 0 || res == 2)
      break;

  if (res == 2)
    return CInfoScanner::ERROR_NFO;
  if (bNfo)
  {
    if (!m_scurl.HasUrls())
    {
      if (m_doc.find("[scrape url]") != std::string::npos)
        return CInfoScanner::OVERRIDE_NFO;
      else
        return CInfoScanner::FULL_NFO;
    }
    else
      return CInfoScanner::COMBINED_NFO;
  }
  return m_scurl.HasUrls() ? CInfoScanner::URL_NFO : CInfoScanner::NO_NFO;
}

// return value: 0 - success; 1 - no result; skip; 2 - error
int CNfoFile::Scrape(ScraperPtr& scraper, CScraperUrl& url,
                     const std::string& content)
{
  if (scraper->IsNoop())
  {
    url = CScraperUrl();
    return 0;
  }

  scraper->ClearCache();

  try
  {
    url = scraper->NfoUrl(content);
  }
  catch (const CScraperError &sce)
  {
    CVideoInfoDownloader::ShowErrorDialog(sce);
    if (!sce.FAborted())
      return 2;
  }

  return url.HasUrls() ? 0 : 1;
}

int CNfoFile::Load(const std::string& strFile)
{
  Close();
  XFILE::CFile file;
  std::vector<uint8_t> buf;
  if (file.LoadFile(strFile, buf) > 0)
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

std::vector<ScraperPtr> CNfoFile::GetScrapers(AddonType type, const ScraperPtr& selectedScraper)
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
