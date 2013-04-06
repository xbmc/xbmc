/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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

#include <limits.h>

#include "Settings.h"
#include "filesystem/File.h"
#include "guilib/WindowIDs.h"
#include "profiles/ProfilesManager.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"

using namespace std;
using namespace XFILE;

CSettings::CSettings(void)
{
}

void CSettings::RegisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.insert(settingsHandler);
}

void CSettings::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.erase(settingsHandler);
}

void CSettings::RegisterSubSettings(ISubSettings *subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.insert(subSettings);
}

void CSettings::UnregisterSubSettings(ISubSettings *subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.erase(subSettings);
}

void CSettings::Initialize()
{
  m_fVolumeLevel = 1.0f;
  m_bMute = false;

  m_iSystemTimeTotalUp = 0;
}

CSettings::~CSettings(void)
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_subSettings.clear();

  Clear();
}

void CSettings::Save() const
{
  if (!SaveSettings(CProfilesManager::Get().GetSettingsFile()))
    CLog::Log(LOGERROR, "Unable to save settings to %s", CProfilesManager::Get().GetSettingsFile().c_str());
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(CProfilesManager::Get().GetSettingsFile());
  Save();
  return LoadSettings(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Load()
{
  if (!OnSettingsLoading())
    return false;

  CLog::Log(LOGNOTICE, "loading %s", CProfilesManager::Get().GetSettingsFile().c_str());
  if (!LoadSettings(CProfilesManager::Get().GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", CProfilesManager::Get().GetSettingsFile().c_str(), CProfilesManager::Get().GetSettingsFile().c_str());
    if (!Reset())
      return false;
  }

  OnSettingsLoaded();

  return true;
}

bool CSettings::GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue)
{
  CStdString strDefault = strValue;
  if (XMLUtils::GetPath(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefault;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue, const CStdString& strDefaultValue)
{
  if (XMLUtils::GetString(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefaultValue;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, char *szValue, const CStdString& strDefaultValue)
{
  CStdString strValue;
  bool ret = GetString(pRootElement, tagName, strValue, strDefaultValue);
  if (szValue)
    strcpy(szValue, strValue.c_str());
  return ret;
}

bool CSettings::GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
  if (XMLUtils::GetInt(pRootElement, tagName, iValue, iMin, iMax))
    return true;
  // default
  iValue = iDefault;
  return false;
}

bool CSettings::GetFloat(const TiXmlElement* pRootElement, const char *tagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
  if (XMLUtils::GetFloat(pRootElement, tagName, fValue, fMin, fMax))
    return true;
  // default
  fValue = fDefault;
  return false;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
{
  // load the xml file
  CXBMCTinyXML xmlDoc;

  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    CLog::Log(LOGERROR, "%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "%s\nDoesn't contain <settings>", strSettingsFile.c_str());
    return false;
  }

  // general settings
  const TiXmlElement *pElement = pRootElement->FirstChildElement("general");
  if (pElement)
  {
    GetInteger(pElement, "systemtotaluptime", m_iSystemTimeTotalUp, 0, 0, INT_MAX);
  }

  // audio settings
  pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mute", m_bMute);
    GetFloat(pElement, "fvolumelevel", m_fVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
  }

  g_guiSettings.LoadXML(pRootElement);
  
  // load any ISubSettings implementations
  return Load(pRootElement);
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings /* = NULL */) const
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement("settings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // write our tags one by one - just a big list for now (can be flashed up later)

  if (!OnSettingsSaving())
    return false;

  // general settings
  TiXmlElement generalNode("general");
  TiXmlNode *pNode = pRoot->InsertEndChild(generalNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "systemtotaluptime", m_iSystemTimeTotalUp);

  // audio settings
  TiXmlElement volumeNode("audio");
  pNode = pRoot->InsertEndChild(volumeNode);
  if (!pNode) return false;
  XMLUtils::SetBoolean(pNode, "mute", m_bMute);
  XMLUtils::SetFloat(pNode, "fvolumelevel", m_fVolumeLevel);

  if (localSettings) // local settings to save
    localSettings->SaveXML(pRoot);
  else // save the global settings
    g_guiSettings.SaveXML(pRoot);

  OnSettingsSaved();
  
  if (!Save(pRoot))
    return false;

  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

void CSettings::Clear()
{
  OnSettingsCleared();

  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    (*it)->Clear();
}

bool CSettings::OnSettingsLoading()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
  {
    if (!(*it)->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettings::OnSettingsLoaded()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsLoaded();
}

bool CSettings::OnSettingsSaving() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
  {
    if (!(*it)->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettings::OnSettingsSaved() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsSaved();
}

void CSettings::OnSettingsCleared()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsCleared();
}

bool CSettings::Load(const TiXmlNode *settings)
{
  bool ok = true;
  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    ok &= (*it)->Load(settings);

  return ok;
}

bool CSettings::Save(TiXmlNode *settings) const
{
  CSingleLock lock(m_critical);
  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
  {
    if (!(*it)->Save(settings))
      return false;
  }

  return true;
}
