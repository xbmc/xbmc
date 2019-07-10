/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Scraper.h"
#include "music/Artist.h"

class CXBMCTinyXML;
class CScraperUrl;

namespace MUSIC_GRABBER
{
class CMusicArtistInfo
{
public:
  CMusicArtistInfo() = default;
  CMusicArtistInfo(const std::string& strArtist, const CScraperUrl& strArtistURL);
  virtual ~CMusicArtistInfo() = default;
  bool Loaded() const { return m_bLoaded; }
  void SetLoaded() { m_bLoaded = true; }
  void SetArtist(const CArtist& artist);
  const CArtist& GetArtist() const { return m_artist; }
  CArtist& GetArtist() { return m_artist; }
  const CScraperUrl& GetArtistURL() const { return m_artistURL; }
  bool Load(XFILE::CCurlFile& http, const ADDON::ScraperPtr& scraper,
    const std::string &strSearch);

protected:
  CArtist m_artist;
  CScraperUrl m_artistURL;
  bool m_bLoaded = false;
};
}
