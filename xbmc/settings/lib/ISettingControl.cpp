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

#ifndef LIB_ISETTINGCONTROL_H_INCLUDED
#define LIB_ISETTINGCONTROL_H_INCLUDED
#include "ISettingControl.h"
#endif

#ifndef LIB_SETTINGDEFINITIONS_H_INCLUDED
#define LIB_SETTINGDEFINITIONS_H_INCLUDED
#include "SettingDefinitions.h"
#endif

#ifndef LIB_UTILS_LOG_H_INCLUDED
#define LIB_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef LIB_UTILS_STRINGUTILS_H_INCLUDED
#define LIB_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef LIB_UTILS_XBMCTINYXML_H_INCLUDED
#define LIB_UTILS_XBMCTINYXML_H_INCLUDED
#include "utils/XBMCTinyXML.h"
#endif


bool ISettingControl::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *strTmp = elem->Attribute(SETTING_XML_ATTR_FORMAT);
  std::string format;
  if (strTmp != NULL)
    format = strTmp;
  if (!SetFormat(format))
  {
    CLog::Log(LOGERROR, "ISettingControl: error reading \"format\" attribute of <control>");
    return false;
  }

  if ((strTmp = elem->Attribute(SETTING_XML_ATTR_DELAYED)) != NULL)
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
