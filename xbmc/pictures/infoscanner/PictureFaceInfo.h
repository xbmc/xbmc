#pragma once

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

#include "pictures/Picture.h"
#include "pictures/Face.h"
#include "addons/Scraper.h"

class CXBMCTinyXML;
class CScraperUrl;

namespace PICTURE_GRABBER
{
    class CPictureFaceInfo
    {
    public:
        CPictureFaceInfo() : m_bLoaded(false) {}
        CPictureFaceInfo(const CStdString& strFace, const CScraperUrl& strFaceURL);
        virtual ~CPictureFaceInfo() {}
        bool Loaded() const { return m_bLoaded; }
        void SetLoaded() { m_bLoaded = true; }
        void SetFace(const CFace& face);
        const CFace& GetFace() const { return m_face; }
        CFace& GetFace() { return m_face; }
        const CScraperUrl& GetFaceURL() const { return m_faceURL; }
        bool Load(XFILE::CCurlFile& http, const ADDON::ScraperPtr& scraper,
                  const CStdString &strSearch);
        
    protected:
        CFace m_face;
        CScraperUrl m_faceURL;
        bool m_bLoaded;
    };
}
