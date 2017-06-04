/*
 *      Copyright (C) 2013 Team XBMC
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

#include "SettingUpdate.h"
#include "SettingDefinitions.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

bool CSettingUpdate::Deserialize(const TiXmlNode *node)
{
  if (node == nullptr)
    return false;

  auto elem = node->ToElement();
  if (elem == nullptr)
    return false;
  
  auto strType = elem->Attribute(SETTING_XML_ATTR_TYPE);
  if (strType == nullptr || strlen(strType) <= 0 || !setType(strType))
  {
    CLog::Log(LOGWARNING, "CSettingUpdate: missing or unknown update type definition");
    return false;
  }

  if (m_type == SettingUpdateType::Rename)
  {
    if (node->FirstChild() == nullptr || node->FirstChild()->Type() != TiXmlNode::TINYXML_TEXT)
    {
      CLog::Log(LOGWARNING, "CSettingUpdate: missing or invalid setting id for rename update definition");
      return false;
    }

    m_value = node->FirstChild()->ValueStr();
  }

  return true;
}

bool CSettingUpdate::setType(const std::string &type)
{
  if (StringUtils::EqualsNoCase(type, "change"))
    m_type = SettingUpdateType::Change;
  else if (StringUtils::EqualsNoCase(type, "rename"))
    m_type = SettingUpdateType::Rename;
  else
    return false;

  return true;
}
