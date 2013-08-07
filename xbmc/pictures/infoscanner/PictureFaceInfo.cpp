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

#include "PictureFaceInfo.h"
#include "addons/Scraper.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;
using namespace PICTURE_GRABBER;

CPictureFaceInfo::CPictureFaceInfo(const CStdString& strFace, const CScraperUrl& strFaceURL)
{
    m_face.strFace = strFace;
    m_faceURL = strFaceURL;
    m_bLoaded = false;
}

void CPictureFaceInfo::SetFace(const CFace& face)
{
    m_face = face;
    m_bLoaded = true;
}

bool CPictureFaceInfo::Load(CCurlFile& http, const ADDON::ScraperPtr& scraper,
                            const CStdString &strSearch)
{
    return false;//m_bLoaded = scraper->GetFaceDetails(http, m_faceURL, strSearch, m_face);
}

