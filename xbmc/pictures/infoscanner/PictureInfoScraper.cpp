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

#include "PictureInfoScraper.h"
#include "utils/log.h"
#include "filesystem/CurlFile.h"

using namespace PICTURE_GRABBER;
using namespace ADDON;
using namespace std;

CPictureInfoScraper::CPictureInfoScraper(const ADDON::ScraperPtr &scraper) : CThread("PictureInfoScraper")
{
    m_bSucceeded=false;
    m_bCanceled=false;
    m_iAlbum=-1;
    m_iFace=-1;
    m_scraper = scraper;
    m_http = new XFILE::CCurlFile;
}

CPictureInfoScraper::~CPictureInfoScraper(void)
{
    StopThread();
    delete m_http;
}

int CPictureInfoScraper::GetAlbumCount() const
{
    return (int)m_vecAlbums.size();
}

int CPictureInfoScraper::GetFaceCount() const
{
    return (int)m_vecFaces.size();
}

CPictureAlbumInfo& CPictureInfoScraper::GetAlbum(int iAlbum)
{
    return m_vecAlbums[iAlbum];
}

CPictureFaceInfo& CPictureInfoScraper::GetFace(int iFace)
{
    return m_vecFaces[iFace];
}

void CPictureInfoScraper::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strFace /* = "" */)
{
    m_strAlbum=strAlbum;
    m_strFace=strFace;
    m_bSucceeded=false;
    StopThread();
    Create();
}

void CPictureInfoScraper::FindFaceInfo(const CStdString& strFace)
{
    m_strFace=strFace;
    m_bSucceeded=false;
    StopThread();
    Create();
}

void CPictureInfoScraper::FindAlbumInfo()
{
//    m_vecAlbums = m_scraper->FindAlbum(*m_http, m_strAlbum, m_strFace);
    m_bSucceeded = !m_vecAlbums.empty();
}

void CPictureInfoScraper::FindFaceInfo()
{
//    m_vecFaces = m_scraper->FindFace(*m_http, m_strFace);
    m_bSucceeded = !m_vecFaces.empty();
}

void CPictureInfoScraper::LoadAlbumInfo(int iAlbum)
{
    m_iAlbum=iAlbum;
    m_iFace=-1;
    StopThread();
    Create();
}

void CPictureInfoScraper::LoadFaceInfo(int iFace, const CStdString &strSearch)
{
    m_iAlbum=-1;
    m_iFace=iFace;
    m_strSearch=strSearch;
    StopThread();
    Create();
}

void CPictureInfoScraper::LoadAlbumInfo()
{
    if (m_iAlbum<0 || m_iAlbum>=(int)m_vecAlbums.size())
        return;
    
    CPictureAlbumInfo& album=m_vecAlbums[m_iAlbum];
    album.GetAlbum().face.clear();
    if (album.Load(*m_http,m_scraper))
        m_bSucceeded=true;
}

void CPictureInfoScraper::LoadFaceInfo()
{
    if (m_iFace<0 || m_iFace>=(int)m_vecFaces.size())
        return;
    
    CPictureFaceInfo& face=m_vecFaces[m_iFace];
    face.GetFace().strFace.Empty();
    if (face.Load(*m_http,m_scraper,m_strSearch))
        m_bSucceeded=true;
}

bool CPictureInfoScraper::Completed()
{
    return WaitForThreadExit(10);
}

bool CPictureInfoScraper::Succeeded()
{
    return !m_bCanceled && m_bSucceeded;
}

void CPictureInfoScraper::Cancel()
{
    m_http->Cancel();
    m_bCanceled=true;
    m_http->Reset();
}

bool CPictureInfoScraper::IsCanceled()
{
    return m_bCanceled;
}

void CPictureInfoScraper::OnStartup()
{
    m_bSucceeded=false;
    m_bCanceled=false;
}

void CPictureInfoScraper::Process()
{
    try
    {
        if (m_strAlbum.size())
        {
            FindAlbumInfo();
            m_strAlbum.Empty();
            m_strFace.Empty();
        }
        else if (m_strFace.size())
        {
            FindFaceInfo();
            m_strFace.Empty();
        }
        if (m_iAlbum>-1)
        {
            LoadAlbumInfo();
            m_iAlbum=-1;
        }
        if (m_iFace>-1)
        {
            LoadFaceInfo();
            m_iFace=-1;
        }
    }
    catch(...)
    {
        CLog::Log(LOGERROR, "Exception in CPictureInfoScraper::Process()");
    }
}

bool CPictureInfoScraper::CheckValidOrFallback(const CStdString &fallbackScraper)
{
    return true;
    /*
     * TODO handle fallback mechanism
     if (m_scraper->Path() != fallbackScraper &&
     parser.Load("special://xbmc/system/scrapers/pictures/" + fallbackScraper))
     {
     CLog::Log(LOGWARNING, "%s - scraper %s fails to load, falling back to %s", __FUNCTION__, m_info.strPath.c_str(), fallbackScraper.c_str());
     m_info.strPath = fallbackScraper;
     m_info.strContent = "albums";
     m_info.strTitle = parser.GetName();
     m_info.strDate = parser.GetDate();
     m_info.strFramework = parser.GetFramework();
     m_info.strLanguage = parser.GetLanguage();
     m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/pictures/" + m_info.strPath);
     return true;
     }
     return false; */
}
