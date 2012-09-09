#pragma once

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

#include "MusicAlbumInfo.h"
#include "MusicArtistInfo.h"
#include "addons/Scraper.h"
#include "threads/Thread.h"

namespace XFILE
{
class CurlFile;
}

namespace MUSIC_GRABBER
{
class CMusicInfoScraper : public CThread
{
public:
  CMusicInfoScraper(const ADDON::ScraperPtr &scraper);
  virtual ~CMusicInfoScraper(void);
  void FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist = "");
  void LoadAlbumInfo(int iAlbum);
  void FindArtistInfo(const CStdString& strArtist);
  void LoadArtistInfo(int iArtist, const CStdString &strSearch);
  bool Completed();
  bool Succeeded();
  void Cancel();
  bool IsCanceled();
  int GetAlbumCount() const;
  int GetArtistCount() const;
  CMusicAlbumInfo& GetAlbum(int iAlbum);
  CMusicArtistInfo& GetArtist(int iArtist);
  std::vector<CMusicArtistInfo>& GetArtists()
  {
    return m_vecArtists;
  }
  std::vector<CMusicAlbumInfo>& GetAlbums()
  {
    return m_vecAlbums;
  }
  void SetScraperInfo(const ADDON::ScraperPtr& scraper)
  {
    m_scraper = scraper;
  }
  /*!
   \brief Checks whether we have a valid scraper.  If not, we try the fallbackScraper
   First tests the current scraper for validity by loading it.  If it is not valid we
   attempt to load the fallback scraper.  If this is also invalid we return false.
   \param fallbackScraper name of scraper to use as a fallback
   \return true if we have a valid scraper (or the default is valid).
   */
  bool CheckValidOrFallback(const CStdString &fallbackScraper);
protected:
  void FindAlbumInfo();
  void LoadAlbumInfo();
  void FindArtistInfo();
  void LoadArtistInfo();
  virtual void OnStartup();
  virtual void Process();
  std::vector<CMusicAlbumInfo> m_vecAlbums;
  std::vector<CMusicArtistInfo> m_vecArtists;
  CStdString m_strAlbum;
  CStdString m_strArtist;
  CStdString m_strSearch;
  int m_iAlbum;
  int m_iArtist;
  bool m_bSucceeded;
  bool m_bCanceled;
  XFILE::CCurlFile* m_http;
  ADDON::ScraperPtr m_scraper;
};

}
