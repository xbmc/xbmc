/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISettingControl.h"
#include "SettingDefinitions.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

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
    CLog::Log(LOGERROR, "ISettingControl: error reading \"format\" attribute of <control>");
    return false;
  }

  if ((strTmp = elem->Attribute(SETTING_XML_ATTR_DELAYED)) != nullptr)
  {
    if (!StringUtils::EqualsNoCase(strTmp, "false") && !StringUtils::EqualsNoCase(strTmp, "true"))
    {
      CLog::Log(LOGERROR, "ISettingControl: error reading \"delayed\" attribute of <control>");
      return false;
    }
    else
      m_delayed = StringUtils::EqualsNoCase(strTmp, "true");
  }

  return true;
}
