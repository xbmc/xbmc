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
#include "settings/lib/SettingDefinitions.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define SHOW_ADDONS_ALL               "all"
#define SHOW_ADDONS_INSTALLED         "installed"
#define SHOW_ADDONS_INSTALLABLE       "installable"

ISettingControl* CSettingControlCreator::CreateControl(const std::string &controlType) const
{
  if (StringUtils::EqualsNoCase(controlType, "toggle"))
    return new CSettingControlCheckmark();
  else if (StringUtils::EqualsNoCase(controlType, "spinner"))
    return new CSettingControlSpinner();
  else if (StringUtils::EqualsNoCase(controlType, "edit"))
    return new CSettingControlEdit();
  else if (StringUtils::EqualsNoCase(controlType, "button"))
    return new CSettingControlButton();
  else if (StringUtils::EqualsNoCase(controlType, "list"))
    return new CSettingControlList();
  else if (StringUtils::EqualsNoCase(controlType, "slider"))
    return new CSettingControlSlider();
  else if (StringUtils::EqualsNoCase(controlType, "range"))
    return new CSettingControlRange();
  else if (StringUtils::EqualsNoCase(controlType, "title"))
    return new CSettingControlTitle();

  return NULL;
}

bool CSettingControlCheckmark::SetFormat(const std::string &format)
{
  return format.empty() || StringUtils::EqualsNoCase(format, "boolean");
}

bool CSettingControlSpinner::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  if (m_format == "string")
  {
    XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_FORMATLABEL, m_formatLabel);

    // get the minimum label from <setting><constraints><minimum label="X" />
    const TiXmlNode *settingNode = node->Parent();
    if (settingNode != NULL)
    {
      const TiXmlNode *contraintsNode = settingNode->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
      if (contraintsNode != NULL)
      {
        const TiXmlNode *minimumNode = contraintsNode->FirstChild(SETTING_XML_ELM_MINIMUM);
        if (minimumNode != NULL)
        {
          const TiXmlElement *minimumElem = minimumNode->ToElement();
          if (minimumElem != NULL)
          {
            if (minimumElem->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &m_minimumLabel) != TIXML_SUCCESS)
              m_minimumLabel = -1;
          }
        }
      }
    }

    if (m_minimumLabel < 0)
    {
      std::string strFormat;
      if (XMLUtils::GetString(node, SETTING_XML_ATTR_FORMAT, strFormat) && !strFormat.empty())
        m_formatString = strFormat;
    }
  }

  return true;
}

bool CSettingControlSpinner::SetFormat(const std::string &format)
{
  if (!StringUtils::EqualsNoCase(format, "string") &&
      !StringUtils::EqualsNoCase(format, "integer") &&
      !StringUtils::EqualsNoCase(format, "number"))
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlEdit::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  XMLUtils::GetBoolean(node, SETTING_XML_ELM_CONTROL_HIDDEN, m_hidden);
  XMLUtils::GetBoolean(node, SETTING_XML_ELM_CONTROL_VERIFYNEW, m_verifyNewValue);
  XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_HEADING, m_heading);

  return true;
}

bool CSettingControlEdit::SetFormat(const std::string &format)
{
  if (!StringUtils::EqualsNoCase(format, "string") &&
      !StringUtils::EqualsNoCase(format, "integer") &&
      !StringUtils::EqualsNoCase(format, "number") &&
      !StringUtils::EqualsNoCase(format, "ip") &&
      !StringUtils::EqualsNoCase(format, "md5"))
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlButton::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;
  
  XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_HEADING, m_heading);
  XMLUtils::GetBoolean(node, SETTING_XML_ELM_CONTROL_HIDEVALUE, m_hideValue);

  if (m_format == "addon")
  {
    std::string strShowAddons;
    if (XMLUtils::GetString(node, "show", strShowAddons) && !strShowAddons.empty())
    {
      if (StringUtils::EqualsNoCase(strShowAddons, SHOW_ADDONS_ALL))
      {
        m_showInstalledAddons = true;
        m_showInstallableAddons = true;
      }
      else if (StringUtils::EqualsNoCase(strShowAddons, SHOW_ADDONS_INSTALLED))
      {
        m_showInstalledAddons = true;
        m_showInstallableAddons = false;
      }
      else if (StringUtils::EqualsNoCase(strShowAddons, SHOW_ADDONS_INSTALLABLE))
      {
        m_showInstalledAddons = false;
        m_showInstallableAddons = true;
      }
      else
        CLog::Log(LOGWARNING, "CSettingControlButton: invalid <show>");

      const TiXmlElement *show = node->FirstChildElement("show");
      if (show != NULL)
      {
        const char *strShowDetails = NULL;
        if ((strShowDetails = show->Attribute(SETTING_XML_ATTR_SHOW_DETAILS)) != NULL)
        {
          if (StringUtils::EqualsNoCase(strShowDetails, "false") || StringUtils::EqualsNoCase(strShowDetails, "true"))
            m_showAddonDetails = StringUtils::EqualsNoCase(strShowDetails, "true");
          else
            CLog::Log(LOGWARNING, "CSettingControlButton: error reading \"details\" attribute of <show>");
        }

        if (!m_showInstallableAddons)
        {
          const char *strShowMore = NULL;
          if ((strShowMore = show->Attribute(SETTING_XML_ATTR_SHOW_MORE)) != NULL)
          {
            if (StringUtils::EqualsNoCase(strShowMore, "false") || StringUtils::EqualsNoCase(strShowMore, "true"))
              m_showMoreAddons = StringUtils::EqualsNoCase(strShowMore, "true");
            else
              CLog::Log(LOGWARNING, "CSettingControlButton: error reading \"more\" attribute of <show>");
          }
        }
      }
    }
  }

  return true;
}

bool CSettingControlButton::SetFormat(const std::string &format)
{
  if (!StringUtils::EqualsNoCase(format, "path") &&
      !StringUtils::EqualsNoCase(format, "addon") &&
      !StringUtils::EqualsNoCase(format, "action") &&
      !StringUtils::EqualsNoCase(format, "infolabel"))
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlList::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;
  
  XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_HEADING, m_heading);
  XMLUtils::GetBoolean(node, SETTING_XML_ELM_CONTROL_MULTISELECT, m_multiselect);
  XMLUtils::GetBoolean(node, SETTING_XML_ELM_CONTROL_HIDEVALUE, m_hideValue);

  return true;
}

bool CSettingControlList::SetFormat(const std::string &format)
{
  if (!StringUtils::EqualsNoCase(format, "string") &&
      !StringUtils::EqualsNoCase(format, "integer"))
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlSlider::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_HEADING, m_heading);
  XMLUtils::GetBoolean(node, SETTING_XML_ELM_CONTROL_POPUP, m_popup);

  XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_FORMATLABEL, m_formatLabel);
  if (m_formatLabel < 0)
  {
    std::string strFormat;
    if (XMLUtils::GetString(node, SETTING_XML_ATTR_FORMAT, strFormat) && !strFormat.empty())
      m_formatString = strFormat;
  }

  return true;
}

bool CSettingControlSlider::SetFormat(const std::string &format)
{
  if (StringUtils::EqualsNoCase(format, "percentage"))
    m_format = "%i %%";
  else if (StringUtils::EqualsNoCase(format, "integer"))
    m_format = "%d";
  else if (StringUtils::EqualsNoCase(format, "number"))
    m_format = "%.1f";
  else
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlRange::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  const TiXmlElement *formatLabel = node->FirstChildElement(SETTING_XML_ELM_CONTROL_FORMATLABEL);
  if (formatLabel != NULL)
  {
    XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_FORMATLABEL, m_formatLabel);
    if (m_formatLabel < 0)
      return false;

    const char *formatValue = formatLabel->Attribute(SETTING_XML_ELM_CONTROL_FORMATVALUE);
    if (formatValue != NULL)
    {
      if (StringUtils::IsInteger(formatValue))
        m_valueFormatLabel = (int)strtol(formatValue, NULL, 0);
      else
      {
        m_valueFormat = formatValue;
        if (!m_valueFormat.empty())
          m_valueFormatLabel = -1;
      }
    }
  }

  return true;
}

bool CSettingControlRange::SetFormat(const std::string &format)
{
  if (StringUtils::EqualsNoCase(format, "percentage"))
    m_valueFormat = "%i %%";
  else if (StringUtils::EqualsNoCase(format, "integer"))
    m_valueFormat = "%d";
  else if (StringUtils::EqualsNoCase(format, "number"))
    m_valueFormat = "%.1f";
  else if (StringUtils::EqualsNoCase(format, "date") ||
           StringUtils::EqualsNoCase(format, "time"))
    m_valueFormat.clear();
  else
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlTitle::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  std::string strTmp;
  if (XMLUtils::GetString(node, SETTING_XML_ATTR_SEPARATOR_POSITION, strTmp))
  {
    if (!StringUtils::EqualsNoCase(strTmp, "top") && !StringUtils::EqualsNoCase(strTmp, "bottom"))
      CLog::Log(LOGWARNING, "CSettingControlTitle: error reading \"value\" attribute of <%s>", SETTING_XML_ATTR_SEPARATOR_POSITION);
    else
      m_separatorBelowLabel = StringUtils::EqualsNoCase(strTmp, "bottom");
  }
  XMLUtils::GetBoolean(node, SETTING_XML_ATTR_HIDE_SEPARATOR, m_separatorHidden);

  return true;
}
