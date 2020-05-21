/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingControl.h"

#include "settings/lib/SettingDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <vector>

const char* SHOW_ADDONS_ALL = "all";
const char* SHOW_ADDONS_INSTALLED = "installed";
const char* SHOW_ADDONS_INSTALLABLE = "installable";

std::shared_ptr<ISettingControl> CSettingControlCreator::CreateControl(const std::string &controlType) const
{
  if (StringUtils::EqualsNoCase(controlType, "toggle"))
    return std::make_shared<CSettingControlCheckmark>();
  else if (StringUtils::EqualsNoCase(controlType, "spinner"))
    return std::make_shared<CSettingControlSpinner>();
  else if (StringUtils::EqualsNoCase(controlType, "edit"))
    return std::make_shared<CSettingControlEdit>();
  else if (StringUtils::EqualsNoCase(controlType, "button"))
    return std::make_shared<CSettingControlButton>();
  else if (StringUtils::EqualsNoCase(controlType, "list"))
    return std::make_shared<CSettingControlList>();
  else if (StringUtils::EqualsNoCase(controlType, "slider"))
    return std::make_shared<CSettingControlSlider>();
  else if (StringUtils::EqualsNoCase(controlType, "range"))
    return std::make_shared<CSettingControlRange>();
  else if (StringUtils::EqualsNoCase(controlType, "title"))
    return std::make_shared<CSettingControlTitle>();
  else if (StringUtils::EqualsNoCase(controlType, "label"))
    return std::make_shared<CSettingControlLabel>();

  return nullptr;
}

bool CSettingControlCheckmark::SetFormat(const std::string &format)
{
  return format.empty() || StringUtils::EqualsNoCase(format, "boolean");
}

bool CSettingControlFormattedRange::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  if (m_format == "string")
  {
    XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_FORMATLABEL, m_formatLabel);

    // get the minimum label from <setting><constraints><minimum label="X" />
    auto settingNode = node->Parent();
    if (settingNode != nullptr)
    {
      auto constraintsNode = settingNode->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
      if (constraintsNode != nullptr)
      {
        auto minimumNode = constraintsNode->FirstChild(SETTING_XML_ELM_MINIMUM);
        if (minimumNode != nullptr)
        {
          auto minimumElem = minimumNode->ToElement();
          if (minimumElem != nullptr)
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
      !StringUtils::EqualsNoCase(format, "md5") &&
      !StringUtils::EqualsNoCase(format, "urlencoded"))
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

  if (m_format == "action")
  {
    bool closeDialog = false;
    if (XMLUtils::GetBoolean(node, "close", closeDialog))
      m_closeDialog = closeDialog;
    std::string strActionData;
    if (XMLUtils::GetString(node, SETTING_XML_ELM_DATA, strActionData))
      m_actionData = strActionData;
  }
  else if (m_format == "addon")
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

      auto show = node->FirstChildElement("show");
      if (show != nullptr)
      {
        const char *strShowDetails = nullptr;
        if ((strShowDetails = show->Attribute(SETTING_XML_ATTR_SHOW_DETAILS)) != nullptr)
        {
          if (StringUtils::EqualsNoCase(strShowDetails, "false") || StringUtils::EqualsNoCase(strShowDetails, "true"))
            m_showAddonDetails = StringUtils::EqualsNoCase(strShowDetails, "true");
          else
            CLog::Log(LOGWARNING, "CSettingControlButton: error reading \"details\" attribute of <show>");
        }

        if (!m_showInstallableAddons)
        {
          const char *strShowMore = nullptr;
          if ((strShowMore = show->Attribute(SETTING_XML_ATTR_SHOW_MORE)) != nullptr)
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
      !StringUtils::EqualsNoCase(format, "file") &&
      !StringUtils::EqualsNoCase(format, "image") &&
      !StringUtils::EqualsNoCase(format, "addon") &&
      !StringUtils::EqualsNoCase(format, "action") &&
      !StringUtils::EqualsNoCase(format, "infolabel") &&
      !StringUtils::EqualsNoCase(format, "date") &&
      !StringUtils::EqualsNoCase(format, "time"))
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);

  return true;
}

bool CSettingControlList::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!CSettingControlFormattedRange::Deserialize(node, update))
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
  if (!StringUtils::EqualsNoCase(format, "percentage") &&
      !StringUtils::EqualsNoCase(format, "integer") &&
      !StringUtils::EqualsNoCase(format, "number"))
    return false;

  m_format = format;
  StringUtils::ToLower(m_format);
  m_formatString = GetDefaultFormatString();

  return true;
}

std::string CSettingControlSlider::GetDefaultFormatString() const
{
  if (m_format == "percentage")
    return "{} %";
  if (m_format == "integer")
    return "{:d}";
  if (m_format == "number")
    return "{:.1f}";

  return "{}";
}

bool CSettingControlRange::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (!ISettingControl::Deserialize(node, update))
    return false;

  auto formatLabel = node->FirstChildElement(SETTING_XML_ELM_CONTROL_FORMATLABEL);
  if (formatLabel != nullptr)
  {
    XMLUtils::GetInt(node, SETTING_XML_ELM_CONTROL_FORMATLABEL, m_formatLabel);
    if (m_formatLabel < 0)
      return false;

    auto formatValue = formatLabel->Attribute(SETTING_XML_ELM_CONTROL_FORMATVALUE);
    if (formatValue != nullptr)
    {
      if (StringUtils::IsInteger(formatValue))
        m_valueFormatLabel = (int)strtol(formatValue, nullptr, 0);
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

CSettingControlLabel::CSettingControlLabel()
{
  m_format = "string";
}
