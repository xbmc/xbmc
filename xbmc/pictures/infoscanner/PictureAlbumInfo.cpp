/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "PictureAlbumInfo.h"
#include "addons/Scraper.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

using namespace std;
using namespace PICTURE_GRABBER;

CPictureAlbumInfo::CPictureAlbumInfo(const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
    m_strTitle2 = strAlbumInfo;
    m_albumURL = strAlbumURL;
    m_relevance = -1;
    m_bLoaded = false;
}

CPictureAlbumInfo::CPictureAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist,
                                 const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
    m_album.strAlbum = strAlbum;
    m_album.face = StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator);
    m_strTitle2 = strAlbumInfo;
    m_albumURL = strAlbumURL;
    m_relevance = -1;
    m_bLoaded = false;
}

void CPictureAlbumInfo::SetAlbum(CPictureAlbum& album)
{
    m_album = album;
    m_strTitle2 = "";
    m_bLoaded = true;
}

bool CPictureAlbumInfo::Load(XFILE::CCurlFile& http, const ADDON::ScraperPtr& scraper)
{
    bool fSuccess = false;//scraper->GetAlbumDetails(http, m_albumURL, m_album);
    if (fSuccess && m_strTitle2.empty())
        m_strTitle2 = m_album.strAlbum;
    SetLoaded(fSuccess);
    return fSuccess;
}

