/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "MusicInfoScraper.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "filesystem/CurlFile.h"

using namespace MUSIC_GRABBER;
using namespace ADDON;
using namespace std;

CMusicInfoScraper::CMusicInfoScraper(const ADDON::ScraperPtr &scraper) : CThread("CMusicInfoScraper")
{
  m_bSucceeded=false;
  m_bCanceled=false;
  m_iAlbum=-1;
  m_iArtist=-1;
  m_scraper = scraper;
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

void CMusicInfoScraper::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist /* = "" */)
{
  m_strAlbum=strAlbum;
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindArtistInfo(const CStdString& strArtist)
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

void CMusicInfoScraper::LoadArtistInfo(int iArtist, const CStdString &strSearch)
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
  album.GetAlbum().artist.clear();
  if (album.Load(*m_http,m_scraper))
    m_bSucceeded=true;
}

void CMusicInfoScraper::LoadArtistInfo()
{
  if (m_iArtist<0 || m_iArtist>=(int)m_vecArtists.size())
    return;

  CMusicArtistInfo& artist=m_vecArtists[m_iArtist];
  artist.GetArtist().strArtist.Empty();
  if (artist.Load(*m_http,m_scraper,m_strSearch))
    m_bSucceeded=true;
}

bool CMusicInfoScraper::Completed()
{
  return WaitForThreadExit(10);
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
      m_strAlbum.Empty();
      m_strArtist.Empty();
    }
    else if (m_strArtist.size())
    {
      FindArtistInfo();
      m_strArtist.Empty();
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

bool CMusicInfoScraper::CheckValidOrFallback(const CStdString &fallbackScraper)
{
  return true;
/*
 * TODO handle fallback mechanism
  if (m_scraper->Path() != fallbackScraper &&
      parser.Load("special://xbmc/system/scrapers/music/" + fallbackScraper))
  {
    CLog::Log(LOGWARNING, "%s - scraper %s fails to load, falling back to %s", __FUNCTION__, m_info.strPath.c_str(), fallbackScraper.c_str());
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
