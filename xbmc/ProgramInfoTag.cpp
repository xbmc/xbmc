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

#include "stdafx.h"
#include "Settings.h"
#include "ProgramInfoTag.h"
#include "XMLUtils.h"

using namespace std;

bool CProgramInfoTag::Load(const TiXmlElement *node, bool chained/*=false*/)
{
    if (!node) return false;
    if (!chained)
      Reset();

    XMLUtils::GetInt(node, "type", m_iType);
    XMLUtils::GetString(node,"title", m_strTitle);
    XMLUtils::GetString(node,"platform", m_strPlatform);
    XMLUtils::GetString(node,"description", m_strDescription);
    XMLUtils::GetString(node,"genre", m_strGenre);
    XMLUtils::GetString(node,"style", m_strStyle);
    XMLUtils::GetString(node,"year", m_strYear);
    XMLUtils::GetString(node,"publisher", m_strPublisher);
    XMLUtils::GetString(node,"releasedate", m_strDateOfRelease);

    // thumbs
    const TiXmlElement* link = node->FirstChildElement("thumb");
    while (link && link->FirstChild())
    {
      m_thumbURL.ParseElement(link);
      link = link->NextSiblingElement("thumb");
    }

    // fanart
    const TiXmlElement *fanart = node->FirstChildElement("fanart");
    if (fanart)
    {
      m_fanart.m_xml << *fanart;
      m_fanart.Unpack();
    }
    return true;
}

bool CProgramInfoTag::Save(TiXmlNode *node, const CStdString &tag)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement programElement(tag.c_str());
  TiXmlNode *program = node->InsertEndChild(programElement);

  if (!program) return false;

  XMLUtils::SetInt(program, "type", m_iType);
  XMLUtils::SetString(program,"title",m_strTitle);
  XMLUtils::SetString(program,"platform", m_strPlatform);
  XMLUtils::SetString(program,"description", m_strDescription);
  XMLUtils::SetString(program,"genre", m_strGenre);
  XMLUtils::SetString(program,"style", m_strStyle);
  XMLUtils::SetString(program,"year", m_strYear);
  XMLUtils::SetString(program,"publisher", m_strPublisher);
  XMLUtils::SetString(program,"dateOfRelease", m_strDateOfRelease);
  XMLUtils::SetString(program, "thumb", m_thumbURL.m_xml);
  if (m_fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(m_fanart.m_xml);
    program->InsertEndChild(*doc.RootElement());
  }

  return true;
}

void CProgramInfoTag::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_idProgram;
    ar << m_iType;
    ar << m_strTitle;
    ar << m_strPlatform;
    ar << m_strDescription;
    ar << m_strGenre;
    ar << m_strStyle;
    ar << m_strPublisher;
    ar << m_strDateOfRelease;
    ar << m_strYear;
    ar << m_thumbURL.m_spoof;
    ar << m_thumbURL.m_xml;
    ar << m_fanart.m_xml;
  }
  else
  {
    ar >> m_idProgram;
    ar >> m_iType;
    ar >> m_strTitle;
    ar >> m_strPlatform;
    ar >> m_strDescription;
    ar >> m_strGenre;
    ar >> m_strStyle;
    ar >> m_strPublisher;
    ar >> m_strDateOfRelease;
    ar >> m_strYear;
    ar >> m_thumbURL.m_spoof;
    ar >> m_thumbURL.m_xml;
    m_thumbURL.Parse();
    ar >> m_fanart.m_xml;
    m_fanart.Unpack();
  }
}

  
