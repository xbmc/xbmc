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

#ifndef INFOSCANNER_MUSICALBUMINFO_H_INCLUDED
#define INFOSCANNER_MUSICALBUMINFO_H_INCLUDED
#include "MusicAlbumInfo.h"
#endif

#ifndef INFOSCANNER_ADDONS_SCRAPER_H_INCLUDED
#define INFOSCANNER_ADDONS_SCRAPER_H_INCLUDED
#include "addons/Scraper.h"
#endif

#ifndef INFOSCANNER_UTILS_LOG_H_INCLUDED
#define INFOSCANNER_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef INFOSCANNER_UTILS_STRINGUTILS_H_INCLUDED
#define INFOSCANNER_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef INFOSCANNER_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define INFOSCANNER_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif


using namespace std;
using namespace MUSIC_GRABBER;

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_strTitle2 = strAlbumInfo;
  m_albumURL = strAlbumURL;
  m_relevance = -1;
  m_bLoaded = false;
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist,
  const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_album.strAlbum = strAlbum;
  m_album.artist = StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator);
  m_strTitle2 = strAlbumInfo;
  m_albumURL = strAlbumURL;
  m_relevance = -1;
  m_bLoaded = false;
}

void CMusicAlbumInfo::SetAlbum(CAlbum& album)
{
  m_album = album;
  m_album.m_strDateOfRelease = StringUtils::Format("%i", album.iYear);
  m_strTitle2 = "";
  m_bLoaded = true;
}

bool CMusicAlbumInfo::Load(XFILE::CCurlFile& http, const ADDON::ScraperPtr& scraper)
{
  bool fSuccess = scraper->GetAlbumDetails(http, m_albumURL, m_album);
  if (fSuccess && m_strTitle2.empty())
    m_strTitle2 = m_album.strAlbum;
  SetLoaded(fSuccess);
  return fSuccess;
}

