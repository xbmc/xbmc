/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
