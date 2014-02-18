#pragma once

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

#include "music/Song.h"
#include "music/Artist.h"
#include "addons/Scraper.h"

class CXBMCTinyXML;
class CScraperUrl;

namespace MUSIC_GRABBER
{
class CMusicArtistInfo
{
public:
  CMusicArtistInfo() : m_bLoaded(false) {}
  CMusicArtistInfo(const std::string& strArtist, const CScraperUrl& strArtistURL);
  virtual ~CMusicArtistInfo() {}
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
  bool m_bLoaded;
};
}
