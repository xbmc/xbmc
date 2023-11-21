/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIColorManager.h"

#include "addons/Skin.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/ColorUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

CGUIColorManager::CGUIColorManager(void) = default;

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
  CLog::Log(LOGINFO, "Loading colors from {}", path);

  if (xmlDoc.LoadFile(path))
    LoadXML(xmlDoc);
}

bool CGUIColorManager::LoadXML(CXBMCTinyXML &xmlDoc)
{
  TiXmlElement* pRootElement = xmlDoc.RootElement();

  std::string strValue = pRootElement->Value();
  if (strValue != std::string("colors"))
  {
    CLog::Log(LOGERROR, "color file doesn't start with <colors>");
    return false;
  }

  const TiXmlElement *color = pRootElement->FirstChildElement("color");

  while (color)
  {
    if (color->FirstChild() && color->Attribute("name"))
    {
      UTILS::COLOR::Color value = 0xffffffff;
      sscanf(color->FirstChild()->Value(), "%x", (unsigned int*) &value);
      std::string name = color->Attribute("name");
      const auto it = m_colors.find(name);
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
UTILS::COLOR::Color CGUIColorManager::GetColor(const std::string& color) const
{
  // look in our color map
  std::string trimmed(color);
  StringUtils::TrimLeft(trimmed, "= ");
  const auto it = m_colors.find(trimmed);
  if (it != m_colors.end())
    return (*it).second;

  // try converting hex directly
  UTILS::COLOR::Color value = 0;
  sscanf(trimmed.c_str(), "%x", &value);
  return value;
}

bool CGUIColorManager::LoadColorsListFromXML(
    const std::string& filePath,
    std::vector<std::pair<std::string, UTILS::COLOR::ColorInfo>>& colors,
    bool sortColors)
{
  CLog::Log(LOGDEBUG, "Loading colors from file {}", filePath);
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(filePath))
  {
    CLog::Log(LOGERROR, "{} - Failed to load colors from file {}", __FUNCTION__, filePath);
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  std::string strValue = pRootElement->Value();
  if (strValue != std::string("colors"))
  {
    CLog::Log(LOGERROR, "{} - Color file doesn't start with <colors>", __FUNCTION__);
    return false;
  }

  const TiXmlElement* xmlColor = pRootElement->FirstChildElement("color");
  while (xmlColor)
  {
    if (xmlColor->FirstChild() && xmlColor->Attribute("name"))
    {
      colors.emplace_back(xmlColor->Attribute("name"),
                          UTILS::COLOR::MakeColorInfo(xmlColor->FirstChild()->Value()));
    }
    xmlColor = xmlColor->NextSiblingElement("color");
  }

  if (sortColors)
    std::sort(colors.begin(), colors.end(), UTILS::COLOR::comparePairColorInfo);

  return true;
}
