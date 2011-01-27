/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "MusicAlbumInfo.h"
#include "addons/Scraper.h"
#include "utils/log.h"

using namespace MUSIC_GRABBER;
using namespace std;

CMusicAlbumInfo::CMusicAlbumInfo(void)
{
  m_strTitle2 = "";
  m_bLoaded = false;
  m_relevance = -1;
}

CMusicAlbumInfo::~CMusicAlbumInfo(void)
{
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_strTitle2 = strAlbumInfo;
  m_albumURL = strAlbumURL;
  m_relevance = -1;
  m_bLoaded = false;
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_album.strAlbum = strAlbum;
  m_album.strArtist = strArtist;
  m_strTitle2 = strAlbumInfo;
  m_albumURL = strAlbumURL;
  m_relevance = -1;
  m_bLoaded = false;
}

const CAlbum& CMusicAlbumInfo::GetAlbum() const
{
  return m_album;
}

CAlbum& CMusicAlbumInfo::GetAlbum()
{
  return m_album;
}

void CMusicAlbumInfo::SetAlbum(CAlbum& album)
{
  m_album = album;
  m_album.m_strDateOfRelease.Format("%i", album.iYear);
  m_strTitle2 = "";
  m_bLoaded = true;
}

const VECSONGS &CMusicAlbumInfo::GetSongs() const
{
  return m_album.songs;
}

void CMusicAlbumInfo::SetSongs(VECSONGS &songs)
{
  m_album.songs = songs;
}

void CMusicAlbumInfo::SetTitle(const CStdString& strTitle)
{
  m_album.strAlbum = strTitle;
}

const CScraperUrl& CMusicAlbumInfo::GetAlbumURL() const
{
  return m_albumURL;
}

const CStdString& CMusicAlbumInfo::GetTitle2() const
{
  return m_strTitle2;
}

const CStdString& CMusicAlbumInfo::GetDateOfRelease() const
{
  return m_album.m_strDateOfRelease;
}

bool CMusicAlbumInfo::Parse(const TiXmlElement* album, bool bChained)
{
  if (!m_album.Load(album,bChained))
    return false;

  if (m_strTitle2.IsEmpty())
    m_strTitle2 = m_album.strAlbum;

  SetLoaded(true);

  return true;
}


bool CMusicAlbumInfo::Load(XFILE::CFileCurl& http,
                           const ADDON::ScraperPtr& scraper)
{
  // load our scraper xml
  if (!scraper->Load())
    return false;

  vector<CStdString> xml = scraper->Run("GetAlbumDetails",GetAlbumURL(),http);

  bool ret=true;
  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str(),0,TIXML_ENCODING_UTF8);
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      return false;
    }

    ret = Parse(doc.RootElement(),it!=xml.begin());
  }

  return ret;
}

void CMusicAlbumInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicAlbumInfo::Loaded() const
{
  return m_bLoaded;
}
