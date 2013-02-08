/*
 *      Copyright (C) 2013 Team XBMC
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

#include <string.h>
#include <limits.h>

#include "ApplicationSettings.h"
#include "guilib/LocalizeStrings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/XMLUtils.h"

using namespace std;

CApplicationSettings::CApplicationSettings()
{
  Clear();

#if defined(TARGET_DARWIN)
  string logDir = getenv("HOME");
  logDir += "/Library/Logs/";
  m_logFolder = logDir;
#else
  m_logFolder = "special://home/";
#endif
}

CApplicationSettings::~CApplicationSettings()
{ }

CApplicationSettings& CApplicationSettings::Get()
{
  static CApplicationSettings sApplicationSettings;
  return sApplicationSettings;
}

bool CApplicationSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  const TiXmlElement *pElement = NULL;

  // general settings
  pElement = settings->FirstChildElement("general");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "systemtotaluptime", m_iSystemTimeTotalUp, 0, INT_MAX);
    XMLUtils::GetBoolean(pElement, "addonautoupdate", m_bAddonAutoUpdate);
    XMLUtils::GetBoolean(pElement, "addonnotifications", m_bAddonNotifications);
    XMLUtils::GetBoolean(pElement, "addonforeignfilter", m_bAddonForeignFilter);
  }

  // audio settings
  pElement = settings->FirstChildElement("audio");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mute", m_bMute);
    XMLUtils::GetFloat(pElement, "fvolumelevel", m_fVolumeLevel, VOLUME_MINIMUM, VOLUME_MAXIMUM);
  }

  return true;
}

bool CApplicationSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  TiXmlNode *pNode = NULL;

  // general settings
  TiXmlElement generalNode("general");
  pNode = settings->InsertEndChild(generalNode);
  if (pNode == NULL)
    return false;
  XMLUtils::SetInt(pNode, "systemtotaluptime", m_iSystemTimeTotalUp);
  XMLUtils::SetBoolean(pNode, "addonautoupdate", m_bAddonAutoUpdate);
  XMLUtils::SetBoolean(pNode, "addonnotifications", m_bAddonNotifications);
  XMLUtils::SetBoolean(pNode, "addonforeignfilter", m_bAddonForeignFilter);

  // audio settings
  TiXmlElement volumeNode("audio");
  pNode = settings->InsertEndChild(volumeNode);
  if (pNode == NULL)
    return false;
  XMLUtils::SetBoolean(pNode, "mute", m_bMute);
  XMLUtils::SetFloat(pNode, "fvolumelevel", m_fVolumeLevel);

  return true;
}

void CApplicationSettings::Clear()
{
  m_fVolumeLevel = 1.0f;
  m_bMute = false;
  m_iSystemTimeTotalUp = 0;
  m_userAgent = g_sysinfo.GetUserAgent();

  m_bAddonAutoUpdate = true;
  m_bAddonNotifications = true;
  m_bAddonForeignFilter = false;
}