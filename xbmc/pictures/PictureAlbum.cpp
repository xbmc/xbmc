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

#include "PictureAlbum.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/MathUtils.h"
#include "FileItem.h"

using namespace std;
using namespace PICTURE_INFO;

CPictureAlbum::CPictureAlbum(const CFileItem& item)
{
    
    Reset();
    const CPictureInfoTag& tag = *item.GetPictureInfoTag();
    SYSTEMTIME stTime;
    strAlbum = tag.GetPictureAlbum();
    location = tag.GetLocation();
    face = tag.GetFace();
    iPictureCount = 0;
     
}

CStdString CPictureAlbum::GetFaceString() const
{
    return StringUtils::Join(face, g_advancedSettings.m_pictureItemSeparator);
}

CStdString CPictureAlbum::GetLocationString() const
{
    return StringUtils::Join(location, g_advancedSettings.m_pictureItemSeparator);
}

bool CPictureAlbum::operator<(const CPictureAlbum &a) const
{
    if (strAlbum < a.strAlbum) return true;
    if (strAlbum > a.strAlbum) return false;
    return false;
}

bool CPictureAlbum::Load(const TiXmlElement *PictureAlbum, bool append, bool prioritise)
{
    if (!PictureAlbum) return false;
    if (!append)
        Reset();
    
    XMLUtils::GetString(PictureAlbum,              "title", strAlbum);
  
    XMLUtils::GetStringArray(PictureAlbum, "face", face, prioritise, g_advancedSettings.m_pictureItemSeparator);
    XMLUtils::GetStringArray(PictureAlbum, "location", location, prioritise, g_advancedSettings.m_pictureItemSeparator);
    XMLUtils::GetStringArray(PictureAlbum, "style", styles, prioritise, g_advancedSettings.m_pictureItemSeparator);
    XMLUtils::GetStringArray(PictureAlbum, "mood", moods, prioritise, g_advancedSettings.m_pictureItemSeparator);
    XMLUtils::GetStringArray(PictureAlbum, "theme", themes, prioritise, g_advancedSettings.m_pictureItemSeparator);
    XMLUtils::GetBoolean(PictureAlbum, "compilation", bCompilation);
    
    XMLUtils::GetString(PictureAlbum,"review",strReview);
    XMLUtils::GetString(PictureAlbum,"releasedate",m_strDateOfRelease);
    XMLUtils::GetString(PictureAlbum,"label",strLabel);
    XMLUtils::GetString(PictureAlbum,"picturetype",strPictureType);
    
    size_t iThumbCount = thumbURL.m_url.size();
    CStdString xmlAdd = thumbURL.m_xml;
    const TiXmlElement* thumb = PictureAlbum->FirstChildElement("thumb");
    while (thumb)
    {
        thumbURL.ParseElement(thumb);
        if (prioritise)
        {
            CStdString temp;
            temp << *thumb;
            xmlAdd = temp+xmlAdd;
        }
        thumb = thumb->NextSiblingElement("thumb");
    }
    // prioritise thumbs from nfos
    if (prioritise && iThumbCount && iThumbCount != thumbURL.m_url.size())
    {
        rotate(thumbURL.m_url.begin(),
               thumbURL.m_url.begin()+iThumbCount,
               thumbURL.m_url.end());
        thumbURL.m_xml = xmlAdd;
    }
    
    const TiXmlElement* PictureAlbumFaceCreditsNode = PictureAlbum->FirstChildElement("PictureAlbumFaceCredits");
    if (PictureAlbumFaceCreditsNode)
        faceCredits.clear();
    
    while (PictureAlbumFaceCreditsNode)
    {
        if (PictureAlbumFaceCreditsNode->FirstChild())
        {
            /*
            CFaceCredit faceCredit;
            XMLUtils::GetString(PictureAlbumFaceCreditsNode,  "face",               faceCredit.m_strFace);
            XMLUtils::GetString(PictureAlbumFaceCreditsNode,  "pictureBrainzFaceID",  faceCredit.m_strPictureBrainzFaceID);
            XMLUtils::GetString(PictureAlbumFaceCreditsNode,  "joinphrase",           faceCredit.m_strJoinPhrase);
            XMLUtils::GetBoolean(PictureAlbumFaceCreditsNode, "featuring",            faceCredit.m_boolFeatured);
            faceCredits.push_back(faceCredit);
             */
        }
        
        PictureAlbumFaceCreditsNode = PictureAlbumFaceCreditsNode->NextSiblingElement("PictureAlbumFaceCredits");
    }
    
    const TiXmlElement* node = PictureAlbum->FirstChildElement("track");
    if (node)
        pictures.clear();  // this means that the tracks can't be spread over separate pages
    // but this is probably a reasonable limitation
    bool bIncrement = false;
    while (node)
    {
        /*
        if (node->FirstChild())
        {
            
            CPicture picture;
            const TiXmlElement* pictureFaceCreditsNode = node->FirstChildElement("pictureFaceCredits");
            if (pictureFaceCreditsNode)
                picture.faceCredits.clear();
            
            while (pictureFaceCreditsNode)
            {
                if (pictureFaceCreditsNode->FirstChild())
                {
                    
                    CFaceCredit faceCredit;
                    XMLUtils::GetString(pictureFaceCreditsNode,  "face",               faceCredit.m_strFace);
                    XMLUtils::GetString(pictureFaceCreditsNode,  "pictureBrainzFaceID",  faceCredit.m_strPictureBrainzFaceID);
                    XMLUtils::GetString(pictureFaceCreditsNode,  "joinphrase",           faceCredit.m_strJoinPhrase);
                    XMLUtils::GetBoolean(pictureFaceCreditsNode, "featuring",            faceCredit.m_boolFeatured);
                    picture.faceCredits.push_back(faceCredit);
                     
                }
                
                pictureFaceCreditsNode = pictureFaceCreditsNode->NextSiblingElement("PictureAlbumFaceCredits");
            }
            
            XMLUtils::GetString(node,   "pictureBrainzTrackID",   picture.strPictureBrainzTrackID);
            XMLUtils::GetInt(node, "position", picture.iTrack);
            
            if (picture.iTrack == 0)
                bIncrement = true;
            
            XMLUtils::GetString(node,"title",picture.strTitle);
            CStdString strDur;
            XMLUtils::GetString(node,"duration",strDur);
            picture.iDuration = StringUtils::TimeStringToSeconds(strDur);
            
            if (bIncrement)
                picture.iTrack = picture.iTrack + 1;
            
            pictures.push_back(picture);
         
        }*/
        node = node->NextSiblingElement("track");
    }
    
    return true;
}

bool CPictureAlbum::Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath)
{
    if (!node) return false;
    
    // we start with a <tag> tag
    TiXmlElement PictureAlbumElement(tag.c_str());
    TiXmlNode *PictureAlbum = node->InsertEndChild(PictureAlbumElement);
    
    if (!PictureAlbum) return false;
    
    XMLUtils::SetString(PictureAlbum,                    "title", strAlbum);
    XMLUtils::SetStringArray(PictureAlbum,              "face", face);
    XMLUtils::SetStringArray(PictureAlbum,               "location", location);
    XMLUtils::SetStringArray(PictureAlbum,               "style", styles);
    XMLUtils::SetStringArray(PictureAlbum,                "mood", moods);
    XMLUtils::SetStringArray(PictureAlbum,               "theme", themes);
    XMLUtils::SetBoolean(PictureAlbum,      "compilation", bCompilation);
    
    XMLUtils::SetString(PictureAlbum,      "review", strReview);
    XMLUtils::SetString(PictureAlbum,        "picturetype", strPictureType);
    XMLUtils::SetString(PictureAlbum, "releasedate", m_strDateOfRelease);
    XMLUtils::SetString(PictureAlbum,       "label", strLabel);
    if (!thumbURL.m_xml.empty())
    {
        CXBMCTinyXML doc;
        doc.Parse(thumbURL.m_xml);
        const TiXmlNode* thumb = doc.FirstChild("thumb");
        while (thumb)
        {
            PictureAlbum->InsertEndChild(*thumb);
            thumb = thumb->NextSibling("thumb");
        }
    }
    XMLUtils::SetString(PictureAlbum,        "path", strPath);
    
    /*
    for( VECFACECREDITS::const_iterator faceCredit = faceCredits.begin();faceCredit != faceCredits.end();++faceCredit)
    {
        // add an <PictureAlbumFaceCredits> tag
        TiXmlElement PictureAlbumFaceCreditsElement("PictureAlbumFaceCredits");
        TiXmlNode *PictureAlbumFaceCreditsNode = PictureAlbum->InsertEndChild(PictureAlbumFaceCreditsElement);
        XMLUtils::SetString(PictureAlbumFaceCreditsNode,               "face", faceCredit->m_strFace);
        XMLUtils::SetString(PictureAlbumFaceCreditsNode,  "pictureBrainzFaceID", faceCredit->m_strPictureBrainzFaceID);
        XMLUtils::SetString(PictureAlbumFaceCreditsNode,           "joinphrase", faceCredit->m_strJoinPhrase);
        XMLUtils::SetString(PictureAlbumFaceCreditsNode,            "featuring", faceCredit->GetFace());
    }
    
    for( VECSONGS::const_iterator picture = pictures.begin(); picture != pictures.end(); ++picture)
    {
        // add a <picture> tag
        TiXmlElement cast("track");
        TiXmlNode *node = PictureAlbum->InsertEndChild(cast);
        for( VECFACECREDITS::const_iterator faceCredit = picture->faceCredits.begin(); faceCredit != faceCredits.end(); ++faceCredit)
        {
            // add an <PictureAlbumFaceCredits> tag
            TiXmlElement pictureFaceCreditsElement("pictureFaceCredits");
            TiXmlNode *pictureFaceCreditsNode = node->InsertEndChild(pictureFaceCreditsElement);
            XMLUtils::SetString(pictureFaceCreditsNode,               "face", faceCredit->m_strFace);
            XMLUtils::SetString(pictureFaceCreditsNode,  "pictureBrainzFaceID", faceCredit->m_strPictureBrainzFaceID);
            XMLUtils::SetString(pictureFaceCreditsNode,           "joinphrase", faceCredit->m_strJoinPhrase);
            XMLUtils::SetString(pictureFaceCreditsNode,            "featuring", faceCredit->GetFace());
        }
        XMLUtils::SetString(node,   "pictureBrainzTrackID",   picture->strPictureBrainzTrackID);
        XMLUtils::SetString(node,   "title",                picture->strTitle);
        XMLUtils::SetInt(node,      "position",             picture->iTrack);
        XMLUtils::SetString(node,   "duration",             StringUtils::SecondsToTimeString(picture->iDuration));
     
    }*/
    
    return true;
}

