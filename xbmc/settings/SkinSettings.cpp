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

#include <string.h>

#include "SkinSettings.h"
#include "GUIInfoManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define XML_SKINSETTINGS  "skinsettings"
#define XML_SETTING       "setting"
#define XML_ATTR_TYPE     "type"
#define XML_ATTR_NAME     "name"

using namespace std;

CSkinSettings::CSkinSettings()
{
  Clear();
}

CSkinSettings::~CSkinSettings()
{ }

CSkinSettings& CSkinSettings::Get()
{
  static CSkinSettings sSkinSettings;
  return sSkinSettings;
}

int CSkinSettings::TranslateString(const string &setting)
{
  std::string settingName = StringUtils::Format("%s.%s", GetCurrentSkin().c_str(), setting.c_str());

  CSingleLock lock(m_critical);
  // run through and see if we have this setting
  for (map<int, CSkinString>::const_iterator it = m_strings.begin(); it != m_strings.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(settingName, it->second.name))
      return it->first;
  }

  // didn't find it - insert it
  CSkinString skinString;
  skinString.name = settingName;

  int number = m_bools.size() + m_strings.size();
  m_strings.insert(pair<int, CSkinString>(number, skinString));
  return number;
}

const string& CSkinSettings::GetString(int setting) const
{
  CSingleLock lock(m_critical);
  map<int, CSkinString>::const_iterator it = m_strings.find(setting);
  if (it != m_strings.end())
    return it->second.value;

  return StringUtils::Empty;
}

void CSkinSettings::SetString(int setting, const string &label)
{
  CSingleLock lock(m_critical);
  map<int, CSkinString>::iterator it = m_strings.find(setting);
  if (it != m_strings.end())
  {
    it->second.value = label;
    return;
  }

  assert(false);
  CLog::Log(LOGFATAL, "%s: unknown setting (%d) requested", __FUNCTION__, setting);
}

int CSkinSettings::TranslateBool(const string &setting)
{
  string settingName = StringUtils::Format("%s.%s", GetCurrentSkin().c_str(), setting.c_str());

  CSingleLock lock(m_critical);
  // run through and see if we have this setting
  for (map<int, CSkinBool>::const_iterator it = m_bools.begin(); it != m_bools.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(settingName, it->second.name))
      return it->first;
  }

  // didn't find it - insert it
  CSkinBool skinBool;
  skinBool.name = settingName;
  skinBool.value = false;

  int number = m_bools.size() + m_strings.size();
  m_bools.insert(pair<int, CSkinBool>(number, skinBool));
  return number;
}

bool CSkinSettings::GetBool(int setting) const
{
  CSingleLock lock(m_critical);
  map<int, CSkinBool>::const_iterator it = m_bools.find(setting);
  if (it != m_bools.end())
    return it->second.value;

  // default is to return false
  return false;
}

void CSkinSettings::SetBool(int setting, bool set)
{
  CSingleLock lock(m_critical);
  map<int, CSkinBool>::iterator it = m_bools.find(setting);
  if (it != m_bools.end())
  {
    it->second.value = set;
    return;
  }

  assert(false);
  CLog::Log(LOGFATAL,"%s: unknown setting (%d) requested", __FUNCTION__, setting);
}

void CSkinSettings::Reset(const string &setting)
{
  string settingName = StringUtils::Format("%s.%s", GetCurrentSkin().c_str(), setting.c_str());

  CSingleLock lock(m_critical);
  // run through and see if we have this setting as a string
  for (map<int, CSkinString>::iterator it = m_strings.begin(); it != m_strings.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(settingName, it->second.name))
    {
      it->second.value.clear();
      return;
    }
  }

  // and now check for the skin bool
  for (map<int, CSkinBool>::iterator it = m_bools.begin(); it != m_bools.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(settingName, it->second.name))
    {
      it->second.value = false;
      return;
    }
  }
}

void CSkinSettings::Reset()
{
  string currentSkin = GetCurrentSkin() + ".";

  CSingleLock lock(m_critical);
  // clear all the settings and strings from this skin.
  for (map<int, CSkinBool>::iterator it = m_bools.begin(); it != m_bools.end(); ++it)
  {
    if (StringUtils::StartsWithNoCase(it->second.name, currentSkin))
      it->second.value = false;
  }

  for (map<int, CSkinString>::iterator it = m_strings.begin(); it != m_strings.end(); ++it)
  {
    if (StringUtils::StartsWithNoCase(it->second.name, currentSkin))
      it->second.value.clear();
  }

  g_infoManager.ResetCache();
}

bool CSkinSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  const TiXmlElement *pElement = settings->FirstChildElement(XML_SKINSETTINGS);
  if (pElement == NULL)
  {
    CLog::Log(LOGWARNING, "CSkinSettings: no <skinsettings> tag found");
    return false;
  }

  CSingleLock lock(m_critical);
  m_strings.clear();
  m_bools.clear();

  int number = 0;
  const TiXmlElement *pChild = pElement->FirstChildElement(XML_SETTING);
  while (pChild)
  {
    std::string settingName = XMLUtils::GetAttribute(pChild, XML_ATTR_NAME);
    std::string settingType = XMLUtils::GetAttribute(pChild, XML_ATTR_TYPE);
    if (settingType == "string")
    { // string setting
      CSkinString string;
      string.name = settingName;
      string.value = pChild->FirstChild() ? pChild->FirstChild()->Value() : "";
      m_strings.insert(pair<int, CSkinString>(number++, string));
    }
    else
    { // bool setting
      CSkinBool setting;
      setting.name = settingName;
      setting.value = pChild->FirstChild() ? StringUtils::EqualsNoCase(pChild->FirstChild()->Value(), "true") : false;
      m_bools.insert(pair<int, CSkinBool>(number++, setting));
    }
    pChild = pChild->NextSiblingElement(XML_SETTING);
  }

  return true;
}

bool CSkinSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  // add the <skinsettings> tag
  TiXmlElement xmlSettingsElement(XML_SKINSETTINGS);
  TiXmlNode *pSettingsNode = settings->InsertEndChild(xmlSettingsElement);
  if (pSettingsNode == NULL)
  {
    CLog::Log(LOGWARNING, "CSkinSettings: could not create <skinsettings> tag");
    return false;
  }

  for (map<int, CSkinBool>::const_iterator it = m_bools.begin(); it != m_bools.end(); ++it)
  {
    // Add a <setting type="bool" name="name">true/false</setting>
    TiXmlElement xmlSetting(XML_SETTING);
    xmlSetting.SetAttribute(XML_ATTR_TYPE, "bool");
    xmlSetting.SetAttribute(XML_ATTR_NAME, (*it).second.name.c_str());
    TiXmlText xmlBool((*it).second.value ? "true" : "false");
    xmlSetting.InsertEndChild(xmlBool);
    pSettingsNode->InsertEndChild(xmlSetting);
  }
  for (map<int, CSkinString>::const_iterator it = m_strings.begin(); it != m_strings.end(); ++it)
  {
    // Add a <setting type="string" name="name">string</setting>
    TiXmlElement xmlSetting(XML_SETTING);
    xmlSetting.SetAttribute(XML_ATTR_TYPE, "string");
    xmlSetting.SetAttribute(XML_ATTR_NAME, (*it).second.name.c_str());
    TiXmlText xmlLabel((*it).second.value);
    xmlSetting.InsertEndChild(xmlLabel);
    pSettingsNode->InsertEndChild(xmlSetting);
  }

  return true;
}

void CSkinSettings::Clear()
{
  CSingleLock lock(m_critical);
  m_strings.clear();
  m_bools.clear();
}

std::string CSkinSettings::GetCurrentSkin() const
{
  return CSettings::Get().GetString("lookandfeel.skin");
}
