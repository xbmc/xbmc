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

#include <vector>

#include "SettingControl.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

bool CSettingControl::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *strTmp = elem->Attribute("type");
  if ((strTmp == NULL && !update && m_type == SettingControlTypeNone) || (strTmp != NULL && !setType(strTmp)))
  {
    CLog::Log(LOGERROR, "CSetting: error reading \"type\" attribute of <control>");
    return false;
  }

  strTmp = elem->Attribute("format");
  if ((strTmp == NULL && !update && m_format == SettingControlFormatNone) || (strTmp != NULL && !setFormat(strTmp)))
  {
    CLog::Log(LOGERROR, "CSetting: error reading \"format\" attribute of <control>");
    return false;
  }

  if ((strTmp = elem->Attribute("attributes")) != NULL && !setAttributes(strTmp))
  {
    CLog::Log(LOGERROR, "CSetting: error reading \"attributes\" attribute of <control>");
    return false;
  }

  if ((strTmp = elem->Attribute("delayed")) != NULL)
  {
    if (!StringUtils::EqualsNoCase(strTmp, "false") && !StringUtils::EqualsNoCase(strTmp, "true"))
    {
      CLog::Log(LOGERROR, "CSetting: error reading \"delayed\" attribute of <control>");
      return false;
    }
    else
      m_delayed = StringUtils::EqualsNoCase(strTmp, "true");
  }
  
  return true;
}

bool CSettingControl::setType(const std::string &strType)
{
  if (StringUtils::EqualsNoCase(strType, "toggle"))
    m_type = SettingControlTypeCheckmark;
  else if (StringUtils::EqualsNoCase(strType, "spinner"))
    m_type = SettingControlTypeSpinner;
  else if (StringUtils::EqualsNoCase(strType, "edit"))
  {
    m_type = SettingControlTypeEdit;
    m_delayed = true;
  }
  else if (StringUtils::EqualsNoCase(strType, "list"))
    m_type = SettingControlTypeList;
  else if (StringUtils::EqualsNoCase(strType, "button"))
    m_type = SettingControlTypeButton;
  else
    return false;

  return true;
}

bool CSettingControl::setFormat(const std::string &strFormat)
{
  if (StringUtils::EqualsNoCase(strFormat, "boolean"))
    m_format = SettingControlFormatBoolean;
  else if (StringUtils::EqualsNoCase(strFormat, "string"))
    m_format = SettingControlFormatString;
  else if (StringUtils::EqualsNoCase(strFormat, "integer"))
    m_format = SettingControlFormatInteger;
  else if (StringUtils::EqualsNoCase(strFormat, "number"))
    m_format = SettingControlFormatNumber;
  else if (StringUtils::EqualsNoCase(strFormat, "ip"))
    m_format = SettingControlFormatIP;
  else if (StringUtils::EqualsNoCase(strFormat, "md5"))
    m_format = SettingControlFormatMD5;
  else if (StringUtils::EqualsNoCase(strFormat, "path"))
    m_format = SettingControlFormatPath;
  else if (StringUtils::EqualsNoCase(strFormat, "addon"))
    m_format = SettingControlFormatAddon;
  else if (StringUtils::EqualsNoCase(strFormat, "action"))
    m_format = SettingControlFormatAction;
  else
    return false;

  return true;
}

bool CSettingControl::setAttributes(const std::string &strAttributes)
{
  std::vector<std::string> attributeList = StringUtils::Split(strAttributes, ",");

  int controlAttributes = SettingControlAttributeNone;
  for (std::vector<std::string>::const_iterator attribute = attributeList.begin(); attribute != attributeList.end(); ++attribute)
  {
    if (StringUtils::EqualsNoCase(*attribute, "hidden"))
      controlAttributes |= (int)SettingControlAttributeHidden;
    else if (StringUtils::EqualsNoCase(*attribute, "new"))
      controlAttributes |= (int)SettingControlAttributeVerifyNew;
    else
      return false;
  }

  m_attributes = (SettingControlAttribute)controlAttributes;
  return true;
}