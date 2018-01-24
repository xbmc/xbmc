/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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
