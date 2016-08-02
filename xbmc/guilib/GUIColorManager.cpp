/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIColorManager.h"

#include <utility>

#include "addons/Skin.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"

CGUIColorManager g_colorManager;

CGUIColorManager::CGUIColorManager(void)
{
}

CGUIColorManager::~CGUIColorManager(void)
{
  Clear();
}

void CGUIColorManager::Clear()
{
  m_colors.clear();
}

// load the color file in
void CGUIColorManager::Load(const std::string &colorFile)
{
  Clear();

  // load the global color map if it exists
  CXBMCTinyXML xmlDoc;
  if (xmlDoc.LoadFile(CSpecialProtocol::TranslatePathConvertCase("special://xbmc/system/colors.xml")))
    LoadXML(xmlDoc);

  // first load the default color map if it exists
  std::string path = URIUtils::AddFileToFolder(g_SkinInfo->Path(), "colors", "defaults.xml");

  if (xmlDoc.LoadFile(CSpecialProtocol::TranslatePathConvertCase(path)))
    LoadXML(xmlDoc);

  // now the color map requested
  if (StringUtils::EqualsNoCase(colorFile, "SKINDEFAULT"))
    return; // nothing to do

  path = URIUtils::AddFileToFolder(g_SkinInfo->Path(), "colors", colorFile);
  if (!URIUtils::HasExtension(path))
    path += ".xml";
  CLog::Log(LOGINFO, "Loading colors from %s", path.c_str());

  if (xmlDoc.LoadFile(path))
    LoadXML(xmlDoc);
}

bool CGUIColorManager::LoadXML(CXBMCTinyXML &xmlDoc)
{
  TiXmlElement* pRootElement = xmlDoc.RootElement();

  std::string strValue = pRootElement->Value();
  if (strValue != std::string("colors"))
  {
    CLog::Log(LOGERROR, "color file doesnt start with <colors>");
    return false;
  }

  const TiXmlElement *color = pRootElement->FirstChildElement("color");

  while (color)
  {
    if (color->FirstChild() && color->Attribute("name"))
    {
      color_t value = 0xffffffff;
      sscanf(color->FirstChild()->Value(), "%x", (unsigned int*) &value);
      std::string name = color->Attribute("name");
      iColor it = m_colors.find(name);
      if (it != m_colors.end())
        (*it).second = value;
      else
        m_colors.insert(make_pair(name, value));
    }
    color = color->NextSiblingElement("color");
  }
  return true;
}

// lookup a color and return it's hex value
color_t CGUIColorManager::GetColor(const std::string &color) const
{
  // look in our color map
  std::string trimmed(color);
  StringUtils::TrimLeft(trimmed, "= ");
  icColor it = m_colors.find(trimmed);
  if (it != m_colors.end())
    return (*it).second;

  // try converting hex directly
  color_t value = 0;
  sscanf(trimmed.c_str(), "%x", &value);
  return value;
}

