/*!
 \file PictureAlbum.h
 \brief
 */
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

#include <map>
#include <vector>
#include "Face.h"
#include "Picture.h"
#include "utils/ScraperUrl.h"

class TiXmlNode;
class CFileItem;
class CPictureAlbum
{
public:
    CPictureAlbum(const CFileItem& item);
    CPictureAlbum() { idAlbum = 0; iPictureCount = 0; };
    bool operator<(const CPictureAlbum &a) const;
    
    void Reset()
    {
        idAlbum = -1;
        strAlbum.Empty();
        face.clear();
        faceCredits.clear();
        location.clear();
        thumbURL.Clear();
        moods.clear();
        styles.clear();
        themes.clear();
        art.clear();
        strReview.Empty();
        strLabel.Empty();
        strPictureType.Empty();
        strPath.Empty();
        m_strDateOfRelease.Empty();
        bCompilation = false;
        iPictureCount = 0;
        pictures.clear();
    }
    
    CStdString GetFaceString() const;
    CStdString GetLocationString() const;
    
    /*! \brief Load PictureAlbum information from an XML file.
     See CVideoInfoTag::Load for a description of the types of elements we load.
     \param element    the root XML element to parse.
     \param append     whether information should be added to the existing tag, or whether it should be reset first.
     \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.
     \sa CVideoInfoTag::Load
     */
    bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
    bool Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath);
    
    long idAlbum;
    CStdString strAlbum;
    std::vector<std::string> face;
    VECFACECREDITS faceCredits;
    std::vector<std::string> location;
    CScraperUrl thumbURL;
    std::vector<std::string> moods;
    std::vector<std::string> styles;
    std::vector<std::string> themes;
    std::map<std::string, std::string> art;
    CStdString strReview;
    CStdString strLabel;
    CStdString strPictureType;
    CStdString strPath;
    CStdString m_strDateOfRelease;
    bool bCompilation;
    int iPictureCount;
    VECPICTURES pictures;
};

typedef std::vector<CPictureAlbum> VECPICTUREALBUMS;
