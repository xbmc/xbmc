/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingAddon.h"

#include "addons/Addon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <mutex>

CSettingAddon::CSettingAddon(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CSettingString(id, settingsManager)
{ }

CSettingAddon::CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = nullptr */)
  : CSettingString(id, label, value, settingsManager)
{ }

CSettingAddon::CSettingAddon(const std::string &id, const CSettingAddon &setting)
  : CSettingString(id, setting)
{
  copyaddontype(setting);
}

SettingPtr CSettingAddon::Clone(const std::string &id) const
{
  return std::make_shared<CSettingAddon>(id, *this);
}

bool CSettingAddon::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  std::unique_lock<CSharedSection> lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;

  if (m_control != nullptr &&
     (m_control->GetType() != "button" || m_control->GetFormat() != "addon"))
  {
    CLog::Log(LOGERROR, "CSettingAddon: invalid <control> of \"{}\"", m_id);
    return false;
  }

  bool ok = false;
  std::string strAddonType;
  auto constraints = node->FirstChild("constraints");
  if (constraints != nullptr)
  {
    // get the addon type
    if (XMLUtils::GetString(constraints, "addontype", strAddonType) && !strAddonType.empty())
    {
      m_addonType = ADDON::CAddonInfo::TranslateType(strAddonType);
      if (m_addonType != ADDON::AddonType::UNKNOWN)
        ok = true;
    }
  }

  if (!ok && !update)
  {
    CLog::Log(LOGERROR, "CSettingAddon: error reading the addontype value \"{}\" of \"{}\"",
              strAddonType, m_id);
    return false;
  }

  return true;
}

void CSettingAddon::copyaddontype(const CSettingAddon &setting)
{
  CSettingString::Copy(setting);

  std::unique_lock<CSharedSection> lock(m_critical);
  m_addonType = setting.m_addonType;
}
