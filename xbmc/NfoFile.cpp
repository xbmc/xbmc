/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NfoFile.h"
#include "ServiceBroker.h"
#include "video/VideoInfoDownloader.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "music/Album.h"
#include "music/Artist.h"

#include <vector>
#include <string>

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

  if (m_type == ADDON_SCRAPER_ALBUMS)
  {
    CAlbum album;
    bNfo = GetDetails(album);
  }
  else if (m_type == ADDON_SCRAPER_ARTISTS)
  {
    CArtist artist;
    bNfo = GetDetails(artist);
  }
  else if (m_type == ADDON_SCRAPER_TVSHOWS || m_type == ADDON_SCRAPER_MOVIES
           || m_type == ADDON_SCRAPER_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_type == ADDON_SCRAPER_TVSHOWS)
    {
      int infos=0;
      while (m_headPos != std::string::npos && details.m_iEpisode != episode)
      {
        m_headPos = m_doc.find("<episodedetails", m_headPos + 1);
        if (m_headPos == std::string::npos)
          break;

        bNfo  = GetDetails(details);
        infos++;
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
    if (m_scurl.m_url.empty())
    {
      if (m_doc.find("[scrape url]") != std::string::npos)
        return CInfoScanner::OVERRIDE_NFO;
      else
        return CInfoScanner::FULL_NFO;
    }
    else
      return CInfoScanner::COMBINED_NFO;
  }
  return m_scurl.m_url.empty() ? CInfoScanner::NO_NFO : CInfoScanner::URL_NFO;
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

  return url.m_url.empty() ? 1 : 0;
}

int CNfoFile::Load(const std::string& strFile)
{
  Close();
  XFILE::CFile file;
  XFILE::auto_buffer buf;
  if (file.LoadFile(strFile, buf) > 0)
  {
    m_doc.assign(buf.get(), buf.size());
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

std::vector<ScraperPtr> CNfoFile::GetScrapers(TYPE type,
                                              ScraperPtr selectedScraper)
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
