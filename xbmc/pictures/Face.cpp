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

#include "Face.h"
#include "utils/XMLUtils.h"
#include "settings/AdvancedSettings.h"

using namespace std;

bool CFace::Load(const TiXmlElement *Face, bool append, bool prioritise)
{
    if (!Face) return false;
    if (!append)
        Reset();
    
    XMLUtils::GetString(Face,                "name", strFace);
    XMLUtils::GetString(Face, "musicBrainzFaceID", strMusicBrainzFaceID);
    
    XMLUtils::GetStringArray(Face,       "location", location, prioritise, g_advancedSettings.m_musicItemSeparator);
    XMLUtils::GetStringArray(Face,       "style", styles, prioritise, g_advancedSettings.m_musicItemSeparator);
    XMLUtils::GetStringArray(Face,        "mood", moods, prioritise, g_advancedSettings.m_musicItemSeparator);
    XMLUtils::GetStringArray(Face, "yearsactive", yearsActive, prioritise, g_advancedSettings.m_musicItemSeparator);
    XMLUtils::GetStringArray(Face, "instruments", instruments, prioritise, g_advancedSettings.m_musicItemSeparator);
    
    XMLUtils::GetString(Face,      "born", strBorn);
    XMLUtils::GetString(Face,    "formed", strFormed);
    XMLUtils::GetString(Face, "biography", strBiography);
    XMLUtils::GetString(Face,      "died", strDied);
    XMLUtils::GetString(Face, "disbanded", strDisbanded);
    
    size_t iThumbCount = thumbURL.m_url.size();
    CStdString xmlAdd = thumbURL.m_xml;
    
    const TiXmlElement* thumb = Face->FirstChildElement("thumb");
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
    // prefix thumbs from nfos
    if (prioritise && iThumbCount && iThumbCount != thumbURL.m_url.size())
    {
        rotate(thumbURL.m_url.begin(),
               thumbURL.m_url.begin()+iThumbCount,
               thumbURL.m_url.end());
        thumbURL.m_xml = xmlAdd;
    }
    const TiXmlElement* node = Face->FirstChildElement("album");
    while (node)
    {
        const TiXmlNode* title = node->FirstChild("title");
        if (title && title->FirstChild())
        {
            CStdString strTitle = title->FirstChild()->Value();
            CStdString strYear;
            const TiXmlNode* year = node->FirstChild("year");
            if (year && year->FirstChild())
                strYear = year->FirstChild()->Value();
            discography.push_back(make_pair(strTitle,strYear));
        }
        node = node->NextSiblingElement("album");
    }
    
    // fanart
    const TiXmlElement *fanart2 = Face->FirstChildElement("fanart");
    if (fanart2)
    {
        // we prefix to handle mixed-mode nfo's with fanart set
        if (prioritise)
        {
            CStdString temp;
            temp << *fanart2;
            fanart.m_xml = temp+fanart.m_xml;
        }
        else
            fanart.m_xml << *fanart2;
        fanart.Unpack();
    }
    
    return true;
}

bool CFace::Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath)
{
    if (!node) return false;
    
    // we start with a <tag> tag
    TiXmlElement FaceElement(tag.c_str());
    TiXmlNode *Face = node->InsertEndChild(FaceElement);
    
    if (!Face) return false;
    
    XMLUtils::SetString(Face,                      "name", strFace);
    XMLUtils::SetString(Face,       "musicBrainzFaceID", strMusicBrainzFaceID);
    XMLUtils::SetStringArray(Face,                "location", location);
    XMLUtils::SetStringArray(Face,                "style", styles);
    XMLUtils::SetStringArray(Face,                 "mood", moods);
    XMLUtils::SetStringArray(Face,          "yearsactive", yearsActive);
    XMLUtils::SetStringArray(Face,          "instruments", instruments);
    XMLUtils::SetString(Face,                      "born", strBorn);
    XMLUtils::SetString(Face,                    "formed", strFormed);
    XMLUtils::SetString(Face,                 "biography", strBiography);
    XMLUtils::SetString(Face,                      "died", strDied);
    XMLUtils::SetString(Face,                 "disbanded", strDisbanded);
    if (!thumbURL.m_xml.empty())
    {
        CXBMCTinyXML doc;
        doc.Parse(thumbURL.m_xml);
        const TiXmlNode* thumb = doc.FirstChild("thumb");
        while (thumb)
        {
            Face->InsertEndChild(*thumb);
            thumb = thumb->NextSibling("thumb");
        }
    }
    XMLUtils::SetString(Face,        "path", strPath);
    if (fanart.m_xml.size())
    {
        CXBMCTinyXML doc;
        doc.Parse(fanart.m_xml);
        Face->InsertEndChild(*doc.RootElement());
    }
    
    // albums
    for (vector< pair<CStdString,CStdString> >::const_iterator it = discography.begin(); it != discography.end(); ++it)
    {
        // add a <album> tag
        TiXmlElement cast("album");
        TiXmlNode *node = Face->InsertEndChild(cast);
        TiXmlElement title("title");
        TiXmlNode *titleNode = node->InsertEndChild(title);
        TiXmlText name(it->first);
        titleNode->InsertEndChild(name);
        TiXmlElement year("year");
        TiXmlNode *yearNode = node->InsertEndChild(year);
        TiXmlText name2(it->second);
        yearNode->InsertEndChild(name2);
    }
    
    return true;
}

