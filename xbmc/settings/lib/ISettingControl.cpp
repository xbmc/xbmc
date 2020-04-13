/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISettingControl.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

ISettingControl::ISettingControl() : CStaticLoggerBase("ISettingControl")
{
}

bool ISettingControl::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (node == nullptr)
    return false;

  auto elem = node->ToElement();
  if (elem == nullptr)
    return false;

  auto strTmp = elem->Attribute(SETTING_XML_ATTR_FORMAT);
  std::string format;
  if (strTmp != nullptr)
    format = strTmp;
  if (!SetFormat(format))
  {
    s_logger->error("error reading \"{}\" attribute of <control>", SETTING_XML_ATTR_FORMAT);
    return false;
  }

  if ((strTmp = elem->Attribute(SETTING_XML_ATTR_DELAYED)) != nullptr)
  {
    if (!StringUtils::EqualsNoCase(strTmp, "false") && !StringUtils::EqualsNoCase(strTmp, "true"))
    {
      s_logger->error("error reading \"{}\" attribute of <control>", SETTING_XML_ATTR_DELAYED);
      return false;
    }
    else
      m_delayed = StringUtils::EqualsNoCase(strTmp, "true");
  }

  return true;
}
