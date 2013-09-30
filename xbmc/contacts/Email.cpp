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

#include "Email.h"
#include "utils/XMLUtils.h"
#include "settings/AdvancedSettings.h"

using namespace std;

bool CEmail::Load(const TiXmlElement *email, bool append, bool prioritise)
{
  if (!email) return false;
  if (!append)
    Reset();

  XMLUtils::GetString(email,                "number", strEmail);


  size_t iThumbCount = thumbURL.m_url.size();
  CStdString xmlAdd = thumbURL.m_xml;

  const TiXmlElement* thumb = email->FirstChildElement("thumb");
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
  const TiXmlElement* node = email->FirstChildElement("contact");
  while (node)
  {
    const TiXmlNode* title = node->FirstChild("title");
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
    }
    node = node->NextSiblingElement("contact");
  }

  return true;
}

bool CEmail::Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement emailElement(tag.c_str());
  TiXmlNode *email = node->InsertEndChild(emailElement);

  if (!email) return false;

  XMLUtils::SetString(email,                      "number", strEmail);
  if (!thumbURL.m_xml.empty())
  {
    CXBMCTinyXML doc;
    doc.Parse(thumbURL.m_xml);
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      email->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }

  return true;
}

