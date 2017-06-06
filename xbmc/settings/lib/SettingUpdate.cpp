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

CSettingUpdate::CSettingUpdate()
  : m_type(SettingUpdateTypeNone)
{ }

bool CSettingUpdate::operator<(const CSettingUpdate& rhs) const
{
  return m_type < rhs.m_type && m_value < rhs.m_value;
}

bool CSettingUpdate::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;
  
  const char *strType = elem->Attribute(SETTING_XML_ATTR_TYPE);
  if (strType == NULL || strlen(strType) <= 0 || !setType(strType))
  {
    CLog::Log(LOGWARNING, "CSettingUpdate: missing or unknown update type definition");
    return false;
  }

  if (m_type == SettingUpdateTypeRename)
  {
    if (node->FirstChild() == NULL || node->FirstChild()->Type() != TiXmlNode::TINYXML_TEXT)
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
    m_type = SettingUpdateTypeChange;
  else if (StringUtils::EqualsNoCase(type, "rename"))
    m_type = SettingUpdateTypeRename;
  else if (StringUtils::EqualsNoCase(type, "keep"))
    m_type = SettingUpdateTypeKeep;
  else
    return false;

  return true;
}
