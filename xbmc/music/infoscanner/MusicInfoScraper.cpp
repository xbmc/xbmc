/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoScraper.h"

#include "filesystem/CurlFile.h"
#include "utils/log.h"

using namespace MUSIC_GRABBER;
using namespace ADDON;
using namespace std::chrono_literals;

CMusicInfoScraper::CMusicInfoScraper(const ADDON::ScraperPtr& scraper)
  : CThread("MusicInfoScraper"), m_scraper(scraper)
{
  m_bSucceeded=false;
  m_bCanceled=false;
  m_iAlbum=-1;
  m_iArtist=-1;
  m_http = new XFILE::CCurlFile;
}

CMusicInfoScraper::~CMusicInfoScraper(void)
{
  StopThread();
  delete m_http;
}

int CMusicInfoScraper::GetAlbumCount() const
{
  return (int)m_vecAlbums.size();
}

int CMusicInfoScraper::GetArtistCount() const
{
  return (int)m_vecArtists.size();
}

CMusicAlbumInfo& CMusicInfoScraper::GetAlbum(int iAlbum)
{
  return m_vecAlbums[iAlbum];
}

CMusicArtistInfo& CMusicInfoScraper::GetArtist(int iArtist)
{
  return m_vecArtists[iArtist];
}

void CMusicInfoScraper::FindAlbumInfo(const std::string& strAlbum, const std::string& strArtist /* = "" */)
{
  m_strAlbum=strAlbum;
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindArtistInfo(const std::string& strArtist)
{
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindAlbumInfo()
{
  m_vecAlbums = m_scraper->FindAlbum(*m_http, m_strAlbum, m_strArtist);
  m_bSucceeded = !m_vecAlbums.empty();
}

void CMusicInfoScraper::FindArtistInfo()
{
  m_vecArtists = m_scraper->FindArtist(*m_http, m_strArtist);
  m_bSucceeded = !m_vecArtists.empty();
}

void CMusicInfoScraper::LoadAlbumInfo(int iAlbum)
{
  m_iAlbum=iAlbum;
  m_iArtist=-1;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadArtistInfo(int iArtist, const std::string &strSearch)
{
  m_iAlbum=-1;
  m_iArtist=iArtist;
  m_strSearch=strSearch;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadAlbumInfo()
{
  if (m_iAlbum<0 || m_iAlbum>=(int)m_vecAlbums.size())
    return;

  CMusicAlbumInfo& album=m_vecAlbums[m_iAlbum];
  // Clear album artist credits
  album.GetAlbum().artistCredits.clear();
  if (album.Load(*m_http,m_scraper))
    m_bSucceeded=true;
}

void CMusicInfoScraper::LoadArtistInfo()
{
  if (m_iArtist<0 || m_iArtist>=(int)m_vecArtists.size())
    return;

  CMusicArtistInfo& artist=m_vecArtists[m_iArtist];
  artist.GetArtist().strArtist.clear();
  if (artist.Load(*m_http,m_scraper,m_strSearch))
    m_bSucceeded=true;
}

bool CMusicInfoScraper::Completed()
{
  return Join(10ms);
}

bool CMusicInfoScraper::Succeeded()
{
  return !m_bCanceled && m_bSucceeded;
}

void CMusicInfoScraper::Cancel()
{
  m_http->Cancel();
  m_bCanceled=true;
  m_http->Reset();
}

bool CMusicInfoScraper::IsCanceled()
{
  return m_bCanceled;
}

void CMusicInfoScraper::OnStartup()
{
  m_bSucceeded=false;
  m_bCanceled=false;
}

void CMusicInfoScraper::Process()
{
  try
  {
    if (m_strAlbum.size())
    {
      FindAlbumInfo();
      m_strAlbum.clear();
      m_strArtist.clear();
    }
    else if (m_strArtist.size())
    {
      FindArtistInfo();
      m_strArtist.clear();
    }
    if (m_iAlbum>-1)
    {
      LoadAlbumInfo();
      m_iAlbum=-1;
    }
    if (m_iArtist>-1)
    {
      LoadArtistInfo();
      m_iArtist=-1;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicInfoScraper::Process()");
  }
}

bool CMusicInfoScraper::CheckValidOrFallback(const std::string &fallbackScraper)
{
  return true;
//! @todo Handle fallback mechanism
  /*
  if (m_scraper->Path() != fallbackScraper &&
      parser.Load("special://xbmc/system/scrapers/music/" + fallbackScraper))
  {
    CLog::Log(LOGWARNING, "{} - scraper {} fails to load, falling back to {}", __FUNCTION__, m_info.strPath, fallbackScraper);
    m_info.strPath = fallbackScraper;
    m_info.strContent = "albums";
    m_info.strTitle = parser.GetName();
    m_info.strDate = parser.GetDate();
    m_info.strFramework = parser.GetFramework();
    m_info.strLanguage = parser.GetLanguage();
    m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + m_info.strPath);
    return true;
  }
  return false; */
}
