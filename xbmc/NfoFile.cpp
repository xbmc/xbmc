/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "video/VideoInfoDownloader.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include <vector>

using namespace std;
using namespace XFILE;
using namespace ADDON;

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, const ScraperPtr& info, int episode)
{
  m_info = info; // assume we can use these settings
  m_type = ScraperTypeFromContent(info->Content());
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CFileItemList items;
  bool bNfo=false;

  AddonPtr addon;
  ScraperPtr defaultScraper;
  if (CAddonMgr::Get().GetDefault(m_type, addon))
    defaultScraper = std::dynamic_pointer_cast<CScraper>(addon);

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
  else if (m_type == ADDON_SCRAPER_TVSHOWS || m_type == ADDON_SCRAPER_MOVIES || m_type == ADDON_SCRAPER_MUSICVIDEOS)
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

  vector<ScraperPtr> vecScrapers;

  // add selected scraper - first proirity
  if (m_info)
    vecScrapers.push_back(m_info);

  // Add all scrapers except selected and default
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(m_type,addons);

  for (unsigned i = 0; i < addons.size(); ++i)
  {
    ScraperPtr scraper = std::dynamic_pointer_cast<CScraper>(addons[i]);

    // skip if scraper requires settings and there's nothing set yet
    if (scraper->RequiresSettings() && !scraper->HasUserSettings())
      continue;

    if( (!m_info || m_info->ID() != scraper->ID()) && (!defaultScraper || defaultScraper->ID() != scraper->ID()) )
      vecScrapers.push_back(scraper);
  }

  // add default scraper - not user selectable so it's last priority
  if( defaultScraper && (!m_info || m_info->ID() != defaultScraper->ID()) &&
      ( !defaultScraper->RequiresSettings() || defaultScraper->HasUserSettings() ) )
    vecScrapers.push_back(defaultScraper);

  // search ..
  int res = -1;
  for (unsigned int i=0;i<vecScrapers.size();++i)
    if ((res = Scrape(vecScrapers[i])) == 0 || res == 2)
      break;

  if (res == 2)
    return ERROR_NFO;
  if (bNfo)
    return m_scurl.m_url.empty() ? FULL_NFO : COMBINED_NFO;
  return m_scurl.m_url.empty() ? NO_NFO : URL_NFO;
}

// return value: 0 - success; 1 - no result; skip; 2 - error
int CNfoFile::Scrape(ScraperPtr& scraper)
{
  if (scraper->IsNoop())
  {
    m_scurl = CScraperUrl();
    return 0;
  }
  if (scraper->Type() != m_type)
    return 1;
  scraper->ClearCache();

  try
  {
    m_scurl = scraper->NfoUrl(m_doc);
  }
  catch (const CScraperError &sce)
  {
    CVideoInfoDownloader::ShowErrorDialog(sce);
    if (!sce.FAborted())
      return 2;
  }

  if (!m_scurl.m_url.empty())
    SetScraperInfo(scraper);
  return m_scurl.m_url.empty() ? 1 : 0;
}

int CNfoFile::Load(const CStdString& strFile)
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
