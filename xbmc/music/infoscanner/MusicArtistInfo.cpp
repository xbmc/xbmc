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

#include "MusicArtistInfo.h"
#include "addons/Scraper.h"

using namespace std;
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

