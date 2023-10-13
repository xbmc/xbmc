/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonUpdateRules.h"

#include "AddonDatabase.h"
#include "addons/addoninfo/AddonInfo.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>

using namespace ADDON;

bool CAddonUpdateRules::RefreshRulesMap(const CAddonDatabase& db)
{
  m_updateRules.clear();
  db.GetAddonUpdateRules(m_updateRules);
  return true;
}

bool CAddonUpdateRules::IsAutoUpdateable(const std::string& id) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_updateRules.find(id) == m_updateRules.end();
}

bool CAddonUpdateRules::IsUpdateableByRule(const std::string& id, AddonUpdateRule updateRule) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto& updateRulesEntry = m_updateRules.find(id);
  return (updateRulesEntry == m_updateRules.end() ||
          std::none_of(updateRulesEntry->second.begin(), updateRulesEntry->second.end(),
                       [updateRule](AddonUpdateRule rule) { return rule == updateRule; }));
}

bool CAddonUpdateRules::AddUpdateRuleToList(CAddonDatabase& db,
                                            const std::string& id,
                                            AddonUpdateRule updateRule)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!IsUpdateableByRule(id, updateRule))
  {
    return true;
  }

  if (db.AddUpdateRuleForAddon(id, updateRule))
  {
    m_updateRules[id].emplace_back(updateRule);
    return true;
  }
  return false;
}

bool CAddonUpdateRules::RemoveUpdateRuleFromList(CAddonDatabase& db,
                                                 const std::string& id,
                                                 AddonUpdateRule updateRule)
{
  return (updateRule != AddonUpdateRule::ANY && RemoveFromUpdateRuleslist(db, id, updateRule));
}

bool CAddonUpdateRules::RemoveAllUpdateRulesFromList(CAddonDatabase& db, const std::string& id)
{
  return RemoveFromUpdateRuleslist(db, id, AddonUpdateRule::ANY);
}

bool CAddonUpdateRules::RemoveFromUpdateRuleslist(CAddonDatabase& db,
                                                  const std::string& id,
                                                  AddonUpdateRule updateRule)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  const auto& updateRulesEntry = m_updateRules.find(id);
  if (updateRulesEntry != m_updateRules.end())
  {
    bool onlySingleRule = (updateRulesEntry->second.size() == 1);

    if (updateRule == AddonUpdateRule::ANY ||
        (onlySingleRule && updateRulesEntry->second.front() == updateRule))
    {
      if (db.RemoveAllUpdateRulesForAddon(id))
      {
        m_updateRules.erase(id);
        return true;
      }
    }
    else if (!onlySingleRule)
    {
      const auto& position =
          std::find(updateRulesEntry->second.begin(), updateRulesEntry->second.end(), updateRule);
      if (position != updateRulesEntry->second.end() && db.RemoveUpdateRuleForAddon(id, updateRule))
      {
        updateRulesEntry->second.erase(position);
        return true;
      }
    }
  }
  return false;
}
