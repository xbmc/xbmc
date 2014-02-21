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
#include "music/Album.h"
#include "addons/Scraper.h"
#include "utils/ScraperUrl.h"

class CXBMCTinyXML;

namespace XFILE { class CCurlFile; }

namespace MUSIC_GRABBER
{
class CMusicAlbumInfo
{
public:
  CMusicAlbumInfo() : m_bLoaded(false), m_relevance(-1) {}
  CMusicAlbumInfo(const std::string& strAlbumInfo, const CScraperUrl& strAlbumURL);
  CMusicAlbumInfo(const std::string& strAlbum, const std::string& strArtist, const std::string& strAlbumInfo, const CScraperUrl& strAlbumURL);
  virtual ~CMusicAlbumInfo() {}

  bool Loaded() const { return m_bLoaded; }
  void SetLoaded(bool bLoaded) { m_bLoaded = bLoaded; }
  const CAlbum &GetAlbum() const { return m_album; }
  CAlbum& GetAlbum() { return m_album; }
  void SetAlbum(CAlbum& album);
  const VECSONGS &GetSongs() const { return m_album.infoSongs; }
  const std::string& GetTitle2() const { return m_strTitle2; }
  void SetTitle(const std::string& strTitle) { m_album.strAlbum = strTitle; }
  const CScraperUrl& GetAlbumURL() const { return m_albumURL; }
  float GetRelevance() const { return m_relevance; }
  void SetRelevance(float relevance) { m_relevance = relevance; }

  bool Load(XFILE::CCurlFile& http, const ADDON::ScraperPtr& scraper);

protected:
  bool m_bLoaded;
  CAlbum m_album;
  float m_relevance;
  std::string m_strTitle2;
  CScraperUrl m_albumURL;
};

}
