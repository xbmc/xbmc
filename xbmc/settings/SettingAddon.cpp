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

#ifndef SETTINGS_SETTINGADDON_H_INCLUDED
#define SETTINGS_SETTINGADDON_H_INCLUDED
#include "SettingAddon.h"
#endif

#ifndef SETTINGS_ADDONS_ADDON_H_INCLUDED
#define SETTINGS_ADDONS_ADDON_H_INCLUDED
#include "addons/Addon.h"
#endif

#ifndef SETTINGS_SETTINGS_LIB_SETTINGSMANAGER_H_INCLUDED
#define SETTINGS_SETTINGS_LIB_SETTINGSMANAGER_H_INCLUDED
#include "settings/lib/SettingsManager.h"
#endif

#ifndef SETTINGS_UTILS_LOG_H_INCLUDED
#define SETTINGS_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef SETTINGS_UTILS_XBMCTINYXML_H_INCLUDED
#define SETTINGS_UTILS_XBMCTINYXML_H_INCLUDED
#include "utils/XBMCTinyXML.h"
#endif

#ifndef SETTINGS_UTILS_XMLUTILS_H_INCLUDED
#define SETTINGS_UTILS_XMLUTILS_H_INCLUDED
#include "utils/XMLUtils.h"
#endif


#define XML_ELM_DEFAULT     "default"

CSettingAddon::CSettingAddon(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, settingsManager),
    m_addonType(ADDON::ADDON_UNKNOWN)
{ }
  
CSettingAddon::CSettingAddon(const std::string &id, const CSettingAddon &setting)
  : CSettingString(id, setting)
{
  copy(setting);
}

CSetting* CSettingAddon::Clone(const std::string &id) const
{
  return new CSettingAddon(id, *this);
}

bool CSettingAddon::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;
    
  if (m_control != NULL &&
     (m_control->GetType() != "button" || m_control->GetFormat() != "addon"))
  {
    CLog::Log(LOGERROR, "CSettingAddon: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }
    
  bool ok = false;
  CStdString strAddonType;
  const TiXmlNode *constraints = node->FirstChild("constraints");
  if (constraints != NULL)
  {
    // get the addon type
    if (XMLUtils::GetString(constraints, "addontype", strAddonType) && !strAddonType.empty())
    {
      m_addonType = ADDON::TranslateType(strAddonType);
      if (m_addonType != ADDON::ADDON_UNKNOWN)
        ok = true;
    }
  }

  if (!ok && !update)
  {
    CLog::Log(LOGERROR, "CSettingAddon: error reading the addontype value \"%s\" of \"%s\"", strAddonType.c_str(), m_id.c_str());
    return false;
  }

  return true;
}

void CSettingAddon::copy(const CSettingAddon &setting)
{
  CSettingString::Copy(setting);
  
  CExclusiveLock lock(m_critical);
  m_addonType = setting.m_addonType;
}
