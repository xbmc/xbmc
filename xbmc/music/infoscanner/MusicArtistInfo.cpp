/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicArtistInfo.h"
#include "addons/Scraper.h"

using namespace XFILE;
using namespace MUSIC_GRABBER;

CMusicArtistInfo::CMusicArtistInfo(const std::string& strArtist, const CScraperUrl& strArtistURL):
  m_artistURL(strArtistURL)
{
  m_artist.strArtist = strArtist;
  m_bLoaded = false;
}

void CMusicArtistInfo::SetArtist(const CArtist& artist)
{
  m_artist = artist;
  m_bLoaded = true;
}

bool CMusicArtistInfo::Load(CCurlFile& http, const ADDON::ScraperPtr& scraper,
  const std::string &strSearch)
{
  return m_bLoaded = scraper->GetArtistDetails(http, m_artistURL, strSearch, m_artist);
}

