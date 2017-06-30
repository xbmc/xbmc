#pragma once
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

#include <string>
#include <vector>

#include "ISetting.h"
#include "Setting.h"
#include "SettingCategoryAccess.h"

class CSettingsManager;

/*!
 \ingroup settings
 \brief Group of settings being part of a category
 \sa CSettingCategory
 \sa CSetting
 */
class CSettingGroup : public ISetting
{
public:
  /*!
   \brief Creates a new setting group with the given identifier.

   \param id Identifier of the setting group
   \param settingsManager Reference to the settings manager
   */
  CSettingGroup(const std::string &id, CSettingsManager *settingsManager = nullptr);
  ~CSettingGroup() override = default;

  // implementation of ISetting
  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  /*!
   \brief Gets the full list of settings belonging to the setting group.

   \return Full list of settings belonging to the setting group
   */
  const SettingList& GetSettings() const { return m_settings; }
  /*!
   \brief Gets the list of settings assigned to the given setting level (or
   below) and that meet the requirements conditions belonging to the setting
   group.

   \param level Level the settings should be assigned to
   \return List of settings belonging to the setting group
   */
  SettingList GetSettings(SettingLevel level) const;

  void AddSetting(std::shared_ptr<CSetting> setting);
  void AddSettings(const SettingList &settings);

  bool ReplaceSetting(std::shared_ptr<const CSetting> currentSetting, std::shared_ptr<CSetting> newSetting);

  std::shared_ptr<const ISettingControl> GetControl() const { return m_control; }
  std::shared_ptr<ISettingControl> GetControl() { return m_control; }
  void SetControl(std::shared_ptr<ISettingControl> control) { m_control = control; }

private:
  SettingList m_settings;
  std::shared_ptr<ISettingControl> m_control;
};

using SettingGroupPtr = std::shared_ptr<CSettingGroup>;
using SettingGroupList = std::vector<SettingGroupPtr>;

/*!
 \ingroup settings
 \brief Category of groups of settings being part of a section
 \sa CSettingSection
 \sa CSettingGroup
 */
class CSettingCategory : public ISetting
{
public:
  /*!
   \brief Creates a new setting category with the given identifier.

   \param id Identifier of the setting category
   \param settingsManager Reference to the settings manager
   */
  CSettingCategory(const std::string &id, CSettingsManager *settingsManager = nullptr);
  ~CSettingCategory() override = default;

  // implementation of ISetting
  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  /*!
   \brief Gets the full list of setting groups belonging to the setting
   category.

   \return Full list of setting groups belonging to the setting category
   */
  const SettingGroupList& GetGroups() const { return m_groups; }
  /*!
   \brief Gets the list of setting groups belonging to the setting category
   that contain settings assigned to the given setting level (or below) and
   that meet the requirements and visibility conditions.

   \param level Level the settings should be assigned to
   \return List of setting groups belonging to the setting category
   */
  SettingGroupList GetGroups(SettingLevel level) const;

  /*!
   \brief Whether the setting category can be accessed or not.

   \return True if the setting category can be accessed, false otherwise
   */
  bool CanAccess() const;

  void AddGroup(SettingGroupPtr group);
  void AddGroups(const SettingGroupList &groups);

private:
  SettingGroupList m_groups;
  CSettingCategoryAccess m_accessCondition;
};

using SettingCategoryPtr = std::shared_ptr<CSettingCategory>;
using SettingCategoryList = std::vector<SettingCategoryPtr>;

/*!
 \ingroup settings
 \brief Section of setting categories
 \sa CSettings
 \sa CSettingCategory
 */
class CSettingSection : public ISetting
{
public:
  /*!
   \brief Creates a new setting section with the given identifier.

   \param id Identifier of the setting section
   \param settingsManager Reference to the settings manager
   */
  CSettingSection(const std::string &id, CSettingsManager *settingsManager = nullptr);
  ~CSettingSection() override = default;

  // implementation of ISetting
  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  /*!
   \brief Gets the full list of setting categories belonging to the setting
   section.

   \return Full list of setting categories belonging to the setting section
   */
  const SettingCategoryList& GetCategories() const { return m_categories; }
  /*!
   \brief Gets the list of setting categories belonging to the setting section
   that contain settings assigned to the given setting level (or below) and
   that meet the requirements and visibility conditions.

   \param level Level the settings should be assigned to
   \return List of setting categories belonging to the setting section
   */
  SettingCategoryList GetCategories(SettingLevel level) const;

  void AddCategory(SettingCategoryPtr category);
  void AddCategories(const SettingCategoryList &categories);

private:
  SettingCategoryList m_categories;
};

using SettingSectionPtr = std::shared_ptr<CSettingSection>;
using SettingSectionList = std::vector<SettingSectionPtr>;
