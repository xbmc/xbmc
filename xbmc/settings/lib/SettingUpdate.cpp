/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingUpdate.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <tinyxml2.h>

Logger CSettingUpdate::s_logger;

CSettingUpdate::CSettingUpdate()
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingUpdate");
}

bool CSettingUpdate::Deserialize(const tinyxml2::XMLNode* node)
{
  if (!node)
    return false;

  auto elem = node->ToElement();
  if (!elem)
    return false;

  auto strType = elem->Attribute(SETTING_XML_ATTR_TYPE);
  if (strType == nullptr || strlen(strType) <= 0 || !setType(strType))
  {
    s_logger->warn("missing or unknown update type definition");
    return false;
  }

  if (m_type == SettingUpdateType::Rename)
  {
    if (!node->FirstChild() || !node->FirstChild()->ToText())
    {
      s_logger->warn("missing or invalid setting id for rename update definition");
      return false;
    }

    m_value = node->FirstChild()->Value();
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
