/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "KeymapLoader.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#ifdef TARGET_WINDOWS
#include "windowing/windows/WinEventsWin32.h"
#endif

using namespace std;
using namespace XFILE;

static std::map<CStdString, CKeymapLoader::KeyMapProfile> deviceMappings;
bool CKeymapLoader::parsedMappings = false;
static bool m_bDefaultEnableMultimediaKeys = false;

void CKeymapLoader::DeviceAdded(const CStdString& deviceId)
{
  ParseDeviceMappings();
  KeyMapProfile keymapProfile;
  if (FindMappedDevice(deviceId, keymapProfile))
  {
    if (g_settings.m_activeKeyboardMapping != keymapProfile.KeymapName)
    {
      CLog::Log(LOGDEBUG, "Switching Active Keymapping to: %s", keymapProfile.KeymapName.c_str());
      g_settings.m_activeKeyboardMapping = keymapProfile.KeymapName;
      g_advancedSettings.m_enableMultimediaKeys = keymapProfile.UseMultimediaKeys;
#ifdef TARGET_WINDOWS
      CWinEventsWin32::DIB_InitOSKeymap();
#endif
    }
  }
}

void CKeymapLoader::DeviceRemoved(const CStdString& deviceId)
{
  ParseDeviceMappings();
  KeyMapProfile keymapProfile;
  if (FindMappedDevice(deviceId, keymapProfile))
  {
    if (g_settings.m_activeKeyboardMapping == keymapProfile.KeymapName)
    {
      CLog::Log(LOGDEBUG, "Switching Active Keymapping to: default");
      g_settings.m_activeKeyboardMapping = "default";
      g_advancedSettings.m_enableMultimediaKeys = m_bDefaultEnableMultimediaKeys;
#ifdef TARGET_WINDOWS
      CWinEventsWin32::DIB_InitOSKeymap();
#endif
    }
  }
}

void CKeymapLoader::ParseDeviceMappings()
{
  if (!parsedMappings)
  {
    m_bDefaultEnableMultimediaKeys = g_advancedSettings.m_enableMultimediaKeys;
    parsedMappings = true;
    CStdString file("special://xbmc/system/deviceidmappings.xml");
    TiXmlDocument deviceXML;
    if (!CFile::Exists(file) || !deviceXML.LoadFile(file))
      return;

    TiXmlElement *pRootElement = deviceXML.RootElement();
    if (!pRootElement || strcmpi(pRootElement->Value(), "devicemappings") != 0)
      return;
  
    TiXmlElement *pDevice = pRootElement->FirstChildElement("device");
    while (pDevice)
    {
      CStdString deviceId(pDevice->Attribute("id"));
      CStdString keymapName(pDevice->Attribute("keymap"));
      if (!deviceId.empty() && !keymapName.empty())
      {
        CStdString useMultimediaKeys(pDevice->Attribute("usemultimediakeys"));
        KeyMapProfile profile;
        profile.KeymapName = keymapName;
        profile.UseMultimediaKeys = useMultimediaKeys && useMultimediaKeys == "true";
        deviceMappings.insert(pair<CStdString, KeyMapProfile>(deviceId.ToUpper(), profile));
      }
      pDevice = pDevice->NextSiblingElement("device");
    }
  }
}

bool CKeymapLoader::FindMappedDevice(const CStdString& deviceId, KeyMapProfile& keymapProfile)
{
  CStdString deviceIdTemp = deviceId;
  std::map<CStdString, KeyMapProfile>::iterator deviceIdIt = deviceMappings.find(deviceIdTemp.ToUpper());
  if (deviceIdIt == deviceMappings.end())
    return false;

  keymapProfile = deviceIdIt->second;
  return true;
}

CStdString CKeymapLoader::ParseWin32HIDName(const CStdString& deviceLongName)
{
  return deviceLongName.Mid(deviceLongName.find_last_of('\\')+1, deviceLongName.find_last_of('#') - deviceLongName.find_last_of('\\'));
}