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

using namespace std;
using namespace MUSIC_GRABBER;

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

void CMusicAlbumInfo::SetAlbum(CAlbum& album)
{
  m_album = album;
  m_album.m_strDateOfRelease.Format("%i", album.iYear);
  m_strTitle2 = "";
  m_bLoaded = true;
}

bool CMusicAlbumInfo::Parse(const TiXmlElement* album, bool bChained)
{
  if (!m_album.Load(album,bChained))
    return false;

  if (m_strTitle2.IsEmpty())
    m_strTitle2 = m_album.strAlbum;

  SetLoaded();

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

