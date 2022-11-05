/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <string>
#include <vector>

namespace ADDON
{

class CAddonDatabase;

enum class AddonUpdateRule;

/**
 * Class - CAddonUpdateRules
 * Manages information about the updateability of addons by defining
 * and handling certain rules.
 */
class CAddonUpdateRules
{
public:
  /* \brief Refresh addon update rules map from the database
   * \param db database connection
   * \return true on success, false otherwise
   */
  bool RefreshRulesMap(const CAddonDatabase& db);

  /* \brief Check if an addon version is auto updateable
   * \param id addon id to be checked
   * \return true is addon is auto updateable, false otherwise
   */
  bool IsAutoUpdateable(const std::string& id) const;

  /* \brief Add a single rule to the update rules list for an addon
   * \param db database connection
   * \param id addon-id to set rule for
   * \param the rule to set
   * \return true on success, false otherwise
   */
  bool AddUpdateRuleToList(CAddonDatabase& db, const std::string& id, AddonUpdateRule updateRule);

  /* \brief Remove a single rule from the update rules list for an addon
   * \param db database connection
   * \param id addon-id to remove rule for
   * \param the rule to remove
   * \return true on success, false otherwise
   */
  bool RemoveUpdateRuleFromList(CAddonDatabase& db,
                                const std::string& id,
                                AddonUpdateRule updateRule);

  /* \brief Remove all rules from the update rules list for an addon
   * \param db database connection
   * \param id addon-id to remove rules for
   * \return true on success, false otherwise
   */
  bool RemoveAllUpdateRulesFromList(CAddonDatabase& db, const std::string& id);

private:
  /* \brief Checks if an addon version is updateable with a specific rule
   * \param id addon id to be checked
   * \param updateRule the rule to check for
   * \return true is addon is updateable by that updateRule, false otherwise
   * \sa CAddonUpdateRules::IsAutoUpdateable()
   */
  bool IsUpdateableByRule(const std::string& id, AddonUpdateRule updateRule) const;

  /* \brief Executor for @ref RemoveUpdateRuleFromList() and @ref RemoveAllUpdateRulesFromList()
   */
  bool RemoveFromUpdateRuleslist(CAddonDatabase& db,
                                 const std::string& id,
                                 AddonUpdateRule updateRule);

  mutable CCriticalSection m_critSection;
  std::map<std::string, std::vector<AddonUpdateRule>> m_updateRules;
};

}; /* namespace ADDON */
