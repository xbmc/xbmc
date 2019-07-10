/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MusicAlbumInfo.h"
#include "MusicArtistInfo.h"
#include "addons/Scraper.h"
#include "threads/Thread.h"

#include <vector>

namespace XFILE
{
class CurlFile;
}

namespace MUSIC_GRABBER
{
class CMusicInfoScraper : public CThread
{
public:
  explicit CMusicInfoScraper(const ADDON::ScraperPtr &scraper);
  ~CMusicInfoScraper(void) override;
  void FindAlbumInfo(const std::string& strAlbum, const std::string& strArtist = "");
  void LoadAlbumInfo(int iAlbum);
  void FindArtistInfo(const std::string& strArtist);
  void LoadArtistInfo(int iArtist, const std::string &strSearch);
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
  bool CheckValidOrFallback(const std::string &fallbackScraper);
protected:
  void FindAlbumInfo();
  void LoadAlbumInfo();
  void FindArtistInfo();
  void LoadArtistInfo();
  void OnStartup() override;
  void Process() override;
  std::vector<CMusicAlbumInfo> m_vecAlbums;
  std::vector<CMusicArtistInfo> m_vecArtists;
  std::string m_strAlbum;
  std::string m_strArtist;
  std::string m_strSearch;
  int m_iAlbum;
  int m_iArtist;
  bool m_bSucceeded;
  bool m_bCanceled;
  XFILE::CCurlFile* m_http;
  ADDON::ScraperPtr m_scraper;
};

}
