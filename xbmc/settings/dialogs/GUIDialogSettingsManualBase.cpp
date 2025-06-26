/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSettingsManualBase.h"

#include "settings/SettingAddon.h"
#include "settings/SettingDateTime.h"
#include "settings/SettingPath.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <string>
#include <vector>

CGUIDialogSettingsManualBase::CGUIDialogSettingsManualBase(int windowId, const std::string& xmlFile)
  : CGUIDialogSettingsManagerBase(windowId, xmlFile)
{ }

CGUIDialogSettingsManualBase::~CGUIDialogSettingsManualBase()
{
  if (GetSettingsManager())
  {
    GetSettingsManager()->Clear();
    m_section = nullptr;
    delete GetSettingsManager();
  }
}

void CGUIDialogSettingsManualBase::SetupView()
{
  InitializeSettings();

  if (GetSettingsManager())
  {
    // add the created setting section to the settings manager and mark it as ready
    GetSettingsManager()->AddSection(m_section);
    GetSettingsManager()->SetInitialized();
    GetSettingsManager()->SetLoaded();
  }

  CGUIDialogSettingsBase::SetupView();
}

CSettingsManager* CGUIDialogSettingsManualBase::GetSettingsManager() const
{
  if (!m_settingsManager)
    m_settingsManager = new CSettingsManager();

  return m_settingsManager;
}

void CGUIDialogSettingsManualBase::InitializeSettings()
{
  if (GetSettingsManager())
  {
    GetSettingsManager()->Clear();
    m_section = nullptr;

    // create a std::make_shared<section
    m_section = std::make_shared<CSettingSection>(GetProperty("xmlfile").asString(), GetSettingsManager());
  }
}

SettingCategoryPtr CGUIDialogSettingsManualBase::AddCategory(const std::string& id,
                                                             int label,
                                                             int help /* = -1 */) const
{
  if (id.empty())
    return nullptr;

  auto category = std::make_shared<CSettingCategory>(id, GetSettingsManager());
  if (!category)
    return nullptr;

  category->SetLabel(label);
  if (help >= 0)
    category->SetHelp(help);

  m_section->AddCategory(category);
  return category;
}

SettingGroupPtr CGUIDialogSettingsManualBase::AddGroup(const SettingCategoryPtr& category,
                                                       int label /* = -1 */,
                                                       int help /* = -1 */,
                                                       bool separatorBelowLabel /* = true */,
                                                       bool hideSeparator /* = false */) const
{
  if (!category)
    return nullptr;

  size_t groups = category->GetGroups().size();

  auto group =
      std::make_shared<CSettingGroup>(StringUtils::Format("{0}", groups + 1), GetSettingsManager());
  if (!group)
    return nullptr;

  if (label >= 0)
    group->SetLabel(label);
  if (help >= 0)
    group->SetHelp(help);
  group->SetControl(GetTitleControl(separatorBelowLabel, hideSeparator));

  category->AddGroup(group);
  return group;
}

std::shared_ptr<CSettingBool> CGUIDialogSettingsManualBase::AddToggle(const SettingGroupPtr& group,
                                                                      const std::string& id,
                                                                      int label,
                                                                      SettingLevel level,
                                                                      bool value,
                                                                      bool delayed /* = false */,
                                                                      bool visible /* = true */,
                                                                      int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingBool>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetCheckmarkControl(delayed));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddEdit(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    int minimum /* = 0 */,
    int step /* = 1 */,
    int maximum /* = 0 */,
    bool verifyNewValue /* = false */,
    int heading /* = -1 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting =
      std::make_shared<CSettingInt>(id, label, value, minimum, step, maximum, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetEditControl("integer", delayed, false, verifyNewValue, heading));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingNumber> CGUIDialogSettingsManualBase::AddEdit(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float value,
    float minimum /* = 0.0f */,
    float step /* = 1.0f */,
    float maximum /* = 0.0f */,
    bool verifyNewValue /* = false */,
    int heading /* = -1 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingNumber>(id, label, value, minimum, step, maximum,
                                                  GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetEditControl("number", delayed, false, verifyNewValue, heading));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingString> CGUIDialogSettingsManualBase::AddEdit(const SettingGroupPtr& group,
                                                                      const std::string& id,
                                                                      int label,
                                                                      SettingLevel level,
                                                                      const std::string& value,
                                                                      bool allowEmpty /* = false */,
                                                                      bool hidden /* = false */,
                                                                      int heading /* = -1 */,
                                                                      bool delayed /* = false */,
                                                                      bool visible /* = true */,
                                                                      int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingString>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetEditControl("string", delayed, hidden, false, heading));
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingString> CGUIDialogSettingsManualBase::AddIp(const SettingGroupPtr& group,
                                                                    const std::string& id,
                                                                    int label,
                                                                    SettingLevel level,
                                                                    const std::string& value,
                                                                    bool allowEmpty /* = false */,
                                                                    int heading /* = -1 */,
                                                                    bool delayed /* = false */,
                                                                    bool visible /* = true */,
                                                                    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingString>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetEditControl("ip", delayed, false, false, heading));
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingString> CGUIDialogSettingsManualBase::AddPasswordMd5(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& value,
    bool allowEmpty /* = false */,
    int heading /* = -1 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingString>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetEditControl("md5", delayed, false, false, heading));
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingAction> CGUIDialogSettingsManualBase::AddButton(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& data /* = "" */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingAction>(id, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetButtonControl("action", delayed));
  setting->SetData(data);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingString> CGUIDialogSettingsManualBase::AddInfoLabelButton(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& info,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingString>(id, label, info, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetButtonControl("infolabel", false));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingAddon> CGUIDialogSettingsManualBase::AddAddon(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& value,
    ADDON::AddonType addonType,
    bool allowEmpty /* = false */,
    int heading /* = -1 */,
    bool hideValue /* = false */,
    bool showInstalledAddons /* = true */,
    bool showInstallableAddons /* = false */,
    bool showMoreAddons /* = true */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingAddon>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetButtonControl("addon", delayed, heading, hideValue, showInstalledAddons, showInstallableAddons, showMoreAddons));
  setting->SetAddonType(addonType);
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingPath> CGUIDialogSettingsManualBase::AddPath(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& value,
    bool writable /* = true */,
    const std::vector<std::string>& sources /* = std::vector<std::string>() */,
    bool allowEmpty /* = false */,
    int heading /* = -1 */,
    bool hideValue /* = false */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingPath>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetButtonControl("path", delayed, heading, hideValue));
  setting->SetWritable(writable);
  setting->SetSources(sources);
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingDate> CGUIDialogSettingsManualBase::AddDate(const SettingGroupPtr& group,
                                                                    const std::string& id,
                                                                    int label,
                                                                    SettingLevel level,
                                                                    const std::string& value,
                                                                    bool allowEmpty /* = false */,
                                                                    int heading /* = -1 */,
                                                                    bool delayed /* = false */,
                                                                    bool visible /* = true */,
                                                                    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingDate>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetButtonControl("date", delayed, heading));
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingTime> CGUIDialogSettingsManualBase::AddTime(const SettingGroupPtr& group,
                                                                    const std::string& id,
                                                                    int label,
                                                                    SettingLevel level,
                                                                    const std::string& value,
                                                                    bool allowEmpty /* = false */,
                                                                    int heading /* = -1 */,
                                                                    bool delayed /* = false */,
                                                                    bool visible /* = true */,
                                                                    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingTime>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetButtonControl("time", delayed, heading));
  setting->SetAllowEmpty(allowEmpty);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingString> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& value,
    const StringSettingOptionsFiller& filler,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || !filler || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingString>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetOptionsFiller(filler);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSpinner(const SettingGroupPtr& group,
                                                                      const std::string& id,
                                                                      int label,
                                                                      SettingLevel level,
                                                                      int value,
                                                                      int minimum,
                                                                      int step,
                                                                      int maximum,
                                                                      int formatLabel /* = -1 */,
                                                                      int minimumLabel /* = -1 */,
                                                                      bool delayed /* = false */,
                                                                      bool visible /* = true */,
                                                                      int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("string", delayed, minimumLabel, formatLabel));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    int minimum,
    int step,
    int maximum,
    const std::string& formatString,
    int minimumLabel /* = -1 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("string", delayed, minimumLabel, -1, formatString));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const TranslatableIntegerSettingOptions& entries,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || entries.empty() || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetTranslatableOptions(entries);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const IntegerSettingOptions& entries,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || entries.empty() || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetOptions(entries);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const IntegerSettingOptionsFiller& filler,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || !filler || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetOptionsFiller(filler);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingNumber> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float value,
    float minimum,
    float step,
    float maximum,
    int formatLabel /* = -1 */,
    int minimumLabel /* = -1 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingNumber>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("number", delayed, minimumLabel, formatLabel));
  setting->SetMinimum(static_cast<double>(minimum));
  setting->SetStep(static_cast<double>(step));
  setting->SetMaximum(static_cast<double>(maximum));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingNumber> CGUIDialogSettingsManualBase::AddSpinner(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float value,
    float minimum,
    float step,
    float maximum,
    const std::string& formatString,
    int minimumLabel /* = -1 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingNumber>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSpinnerControl("number", delayed, minimumLabel, -1, formatString));
  setting->SetMinimum(static_cast<double>(minimum));
  setting->SetStep(static_cast<double>(step));
  setting->SetMaximum(static_cast<double>(maximum));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingString> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::string& value,
    const StringSettingOptionsFiller& filler,
    int heading,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || !filler || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingString>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetListControl("string", false, heading, false, nullptr, details));
  setting->SetOptionsFiller(filler);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const TranslatableIntegerSettingOptions& entries,
    int heading,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || entries.empty() || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetListControl("integer", false, heading, false, nullptr, details));
  setting->SetTranslatableOptions(entries);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const IntegerSettingOptions& entries,
    int heading,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || entries.empty() || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetListControl("integer", false, heading, false, nullptr, details));
  setting->SetOptions(entries);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const IntegerSettingOptionsFiller& filler,
    int heading,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || !filler || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetListControl("integer", false, heading, false, nullptr, details));
  setting->SetOptionsFiller(filler);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::vector<std::string>& values,
    const StringSettingOptionsFiller& filler,
    int heading,
    int minimumItems /* = 0 */,
    int maximumItems /* = -1 */,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || !filler || GetSetting(id))
    return nullptr;

  const auto settingDefinition = std::make_shared<CSettingString>(id, GetSettingsManager());
  if (!settingDefinition)
    return nullptr;

  settingDefinition->SetOptionsFiller(filler);

  auto setting = std::make_shared<CSettingList>(id, settingDefinition, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  std::vector<CVariant> valueList;
  valueList.reserve(values.size());
  for (const auto& value : values)
    valueList.emplace_back(value);

  SettingList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
    return nullptr;

  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("string", false, heading, true, nullptr, details));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::vector<int>& values,
    const TranslatableIntegerSettingOptions& entries,
    int heading,
    int minimumItems /* = 0 */,
    int maximumItems /* = -1 */,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || entries.empty() || GetSetting(id))
    return nullptr;

  const auto settingDefinition = std::make_shared<CSettingInt>(id, GetSettingsManager());
  if (settingDefinition == NULL)
    return NULL;

  settingDefinition->SetTranslatableOptions(entries);

  auto setting = std::make_shared<CSettingList>(id, settingDefinition, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  std::vector<CVariant> valueList;
  valueList.reserve(values.size());
  for (const auto& value : values)
    valueList.emplace_back(value);

  SettingList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
    return NULL;
  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("integer", false, heading, true, nullptr, details));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::vector<int>& values,
    const IntegerSettingOptions& entries,
    int heading,
    int minimumItems /* = 0 */,
    int maximumItems /* = -1 */,
    bool visible /* = true */,
    int help /* = -1 */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || entries.empty() || GetSetting(id))
    return nullptr;

  const auto settingDefinition = std::make_shared<CSettingInt>(id, GetSettingsManager());
  if (!settingDefinition)
    return nullptr;

  settingDefinition->SetOptions(entries);

  auto setting = std::make_shared<CSettingList>(id, settingDefinition, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  std::vector<CVariant> valueList;
  valueList.reserve(values.size());
  for (const auto& value : values)
    valueList.emplace_back(value);

  SettingList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
    return nullptr;

  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("integer", false, heading, true, nullptr, details));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddList(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    const std::vector<int>& values,
    const IntegerSettingOptionsFiller& filler,
    int heading,
    int minimumItems /* = 0 */,
    int maximumItems /* = -1 */,
    bool visible /* = true */,
    int help /* = -1 */,
    const SettingControlListValueFormatter& formatter /* = {} */,
    bool details /* = false */)
{
  if (!group || id.empty() || label < 0 || !filler || GetSetting(id))
    return nullptr;

  const auto settingDefinition = std::make_shared<CSettingInt>(id, GetSettingsManager());
  if (!settingDefinition)
    return nullptr;

  settingDefinition->SetOptionsFiller(filler);

  auto setting = std::make_shared<CSettingList>(id, settingDefinition, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  std::vector<CVariant> valueList;
  valueList.reserve(values.size());
  for (const auto& value : values)
    valueList.emplace_back(value);

  SettingList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
    return nullptr;

  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("integer", false, heading, true, formatter, details));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddPercentageSlider(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    int formatLabel,
    int step /* = 1 */,
    int heading /* = -1 */,
    bool usePopup /* = false */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSliderControl("percentage", delayed, heading, usePopup, formatLabel));
  setting->SetMinimum(0);
  setting->SetStep(step);
  setting->SetMaximum(100);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddPercentageSlider(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const std::string& formatString,
    int step /* = 1 */,
    int heading /* = -1 */,
    bool usePopup /* = false */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSliderControl("percentage", delayed, heading, usePopup, -1, formatString));
  setting->SetMinimum(0);
  setting->SetStep(step);
  setting->SetMaximum(100);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSlider(const SettingGroupPtr& group,
                                                                     const std::string& id,
                                                                     int label,
                                                                     SettingLevel level,
                                                                     int value,
                                                                     int formatLabel,
                                                                     int minimum,
                                                                     int step,
                                                                     int maximum,
                                                                     int heading /* = -1 */,
                                                                     bool usePopup /* = false */,
                                                                     bool delayed /* = false */,
                                                                     bool visible /* = true */,
                                                                     int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSliderControl("integer", delayed, heading, usePopup, formatLabel));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingInt> CGUIDialogSettingsManualBase::AddSlider(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int value,
    const std::string& formatString,
    int minimum,
    int step,
    int maximum,
    int heading /* = -1 */,
    bool usePopup /* = false */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingInt>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSliderControl("integer", delayed, heading, usePopup, -1, formatString));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingNumber> CGUIDialogSettingsManualBase::AddSlider(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float value,
    int formatLabel,
    float minimum,
    float step,
    float maximum,
    int heading /* = -1 */,
    bool usePopup /* = false */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingNumber>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSliderControl("number", delayed, heading, usePopup, formatLabel));
  setting->SetMinimum(static_cast<double>(minimum));
  setting->SetStep(static_cast<double>(step));
  setting->SetMaximum(static_cast<double>(maximum));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingNumber> CGUIDialogSettingsManualBase::AddSlider(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float value,
    const std::string& formatString,
    float minimum,
    float step,
    float maximum,
    int heading /* = -1 */,
    bool usePopup /* = false */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  auto setting = std::make_shared<CSettingNumber>(id, label, value, GetSettingsManager());
  if (!setting)
    return nullptr;

  setting->SetControl(GetSliderControl("number", delayed, heading, usePopup, -1, formatString));
  setting->SetMinimum(static_cast<double>(minimum));
  setting->SetStep(static_cast<double>(step));
  setting->SetMaximum(static_cast<double>(maximum));
  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddPercentageRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int valueFormatLabel,
    int step /* = 1 */,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, 0, step, 100, "percentage", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddPercentageRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    const std::string& valueFormatString /* = "{:d} %" */,
    int step /* = 1 */,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, 0, step, 100, "percentage", formatLabel, -1, valueFormatString, delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddRange(const SettingGroupPtr& group,
                                                                     const std::string& id,
                                                                     int label,
                                                                     SettingLevel level,
                                                                     int valueLower,
                                                                     int valueUpper,
                                                                     int minimum,
                                                                     int step,
                                                                     int maximum,
                                                                     int valueFormatLabel,
                                                                     int formatLabel /* = 21469 */,
                                                                     bool delayed /* = false */,
                                                                     bool visible /* = true */,
                                                                     int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "integer", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int minimum,
    int step,
    int maximum,
    const std::string& valueFormatString /* = "{:d}" */,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "integer", formatLabel, -1, valueFormatString, delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddRange(const SettingGroupPtr& group,
                                                                     const std::string& id,
                                                                     int label,
                                                                     SettingLevel level,
                                                                     float valueLower,
                                                                     float valueUpper,
                                                                     float minimum,
                                                                     float step,
                                                                     float maximum,
                                                                     int valueFormatLabel,
                                                                     int formatLabel /* = 21469 */,
                                                                     bool delayed /* = false */,
                                                                     bool visible /* = true */,
                                                                     int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "number", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float valueLower,
    float valueUpper,
    float minimum,
    float step,
    float maximum,
    const std::string& valueFormatString /* = "{:.1f}" */,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "number", formatLabel, -1, valueFormatString, delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddDateRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int minimum,
    int step,
    int maximum,
    int valueFormatLabel,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "date", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddDateRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int minimum,
    int step,
    int maximum,
    const std::string& valueFormatString /* = "" */,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "date", formatLabel, -1, valueFormatString, delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddTimeRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int minimum,
    int step,
    int maximum,
    int valueFormatLabel,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "time", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddTimeRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int minimum,
    int step,
    int maximum,
    const std::string& valueFormatString /* = "mm:ss" */,
    int formatLabel /* = 21469 */,
    bool delayed /* = false */,
    bool visible /* = true */,
    int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "time", formatLabel, -1, valueFormatString, delayed, visible, help);
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    int valueLower,
    int valueUpper,
    int minimum,
    int step,
    int maximum,
    const std::string& format,
    int formatLabel,
    int valueFormatLabel,
    const std::string& valueFormatString,
    bool delayed,
    bool visible,
    int help)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  const auto settingDefinition = std::make_shared<CSettingInt>(id, GetSettingsManager());
  if (!settingDefinition)
    return nullptr;

  settingDefinition->SetMinimum(minimum);
  settingDefinition->SetStep(step);
  settingDefinition->SetMaximum(maximum);

  auto setting = std::make_shared<CSettingList>(id, settingDefinition, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  std::vector<CVariant> valueList;
  valueList.emplace_back(valueLower);
  valueList.emplace_back(valueUpper);
  SettingList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
    return nullptr;

  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetRangeControl(format, delayed, formatLabel, valueFormatLabel, valueFormatString));
  setting->SetMinimumItems(2);
  setting->SetMaximumItems(2);

  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

std::shared_ptr<CSettingList> CGUIDialogSettingsManualBase::AddRange(
    const SettingGroupPtr& group,
    const std::string& id,
    int label,
    SettingLevel level,
    float valueLower,
    float valueUpper,
    float minimum,
    float step,
    float maximum,
    const std::string& format,
    int formatLabel,
    int valueFormatLabel,
    const std::string& valueFormatString,
    bool delayed,
    bool visible,
    int help)
{
  if (!group || id.empty() || label < 0 || GetSetting(id))
    return nullptr;

  const auto settingDefinition = std::make_shared<CSettingNumber>(id, GetSettingsManager());
  if (!settingDefinition)
    return nullptr;

  settingDefinition->SetMinimum(static_cast<double>(minimum));
  settingDefinition->SetStep(static_cast<double>(step));
  settingDefinition->SetMaximum(static_cast<double>(maximum));

  auto setting = std::make_shared<CSettingList>(id, settingDefinition, label, GetSettingsManager());
  if (!setting)
    return nullptr;

  std::vector<CVariant> valueList;
  valueList.emplace_back(valueLower);
  valueList.emplace_back(valueUpper);
  SettingList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
    return nullptr;

  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetRangeControl(format, delayed, formatLabel, valueFormatLabel, valueFormatString));
  setting->SetMinimumItems(2);
  setting->SetMaximumItems(2);

  SetSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

void CGUIDialogSettingsManualBase::SetSettingDetails(const std::shared_ptr<CSetting>& setting,
                                                     SettingLevel level,
                                                     bool visible,
                                                     int help) const
{
  if (!setting)
    return;

  if (level < SettingLevel::Basic)
    level = SettingLevel::Basic;
  else if (level > SettingLevel::Expert)
    level = SettingLevel::Expert;

  setting->SetLevel(level);
  setting->SetVisible(visible);
  if (help >= 0)
    setting->SetHelp(help);
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetCheckmarkControl(
    bool delayed /* = false */) const
{
  const auto control = std::make_shared<CSettingControlCheckmark>();
  control->SetDelayed(delayed);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetTitleControl(
    bool separatorBelowLabel /* = true */, bool hideSeparator /* = false */) const
{
  const auto control = std::make_shared<CSettingControlTitle>();
  control->SetSeparatorBelowLabel(separatorBelowLabel);
  control->SetSeparatorHidden(hideSeparator);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetEditControl(
    const std::string& format,
    bool delayed /* = false */,
    bool hidden /* = false */,
    bool verifyNewValue /* = false */,
    int heading /* = -1 */) const
{
  const auto control = std::make_shared<CSettingControlEdit>();
  if (!control->SetFormat(format))
    return nullptr;

  control->SetDelayed(delayed);
  control->SetHidden(hidden);
  control->SetVerifyNewValue(verifyNewValue);
  control->SetHeading(heading);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetButtonControl(
    const std::string& format,
    bool delayed /* = false */,
    int heading /* = -1 */,
    bool hideValue /* = false */,
    bool showInstalledAddons /* = true */,
    bool showInstallableAddons /* = false */,
    bool showMoreAddons /* = true */) const
{
  const auto control = std::make_shared<CSettingControlButton>();
  if (!control->SetFormat(format))
    return nullptr;

  control->SetDelayed(delayed);
  control->SetHeading(heading);
  control->SetHideValue(hideValue);
  control->SetShowInstalledAddons(showInstalledAddons);
  control->SetShowInstallableAddons(showInstallableAddons);
  control->SetShowMoreAddons(showMoreAddons);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetSpinnerControl(
    const std::string& format,
    bool delayed /* = false */,
    int minimumLabel /* = -1 */,
    int formatLabel /* = -1 */,
    const std::string& formatString /* = "" */) const
{
  const auto control = std::make_shared<CSettingControlSpinner>();
  if (!control->SetFormat(format))
    return nullptr;

  control->SetDelayed(delayed);
  if (formatLabel >= 0)
    control->SetFormatLabel(formatLabel);
  if (!formatString.empty())
    control->SetFormatString(formatString);
  if (minimumLabel >= 0)
    control->SetMinimumLabel(minimumLabel);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetListControl(
    const std::string& format,
    bool delayed /* = false */,
    int heading /* = -1 */,
    bool multiselect /* = false */,
    const SettingControlListValueFormatter& formatter /* = {} */,
    bool details /* = false */) const
{
  const auto control = std::make_shared<CSettingControlList>();
  if (!control->SetFormat(format))
    return nullptr;

  control->SetDelayed(delayed);
  control->SetHeading(heading);
  control->SetMultiSelect(multiselect);
  control->SetFormatter(formatter);
  control->SetUseDetails(details);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetSliderControl(
    const std::string& format,
    bool delayed /* = false */,
    int heading /* = -1 */,
    bool usePopup /* = false */,
    int formatLabel /* = -1 */,
    const std::string& formatString /* = "" */) const
{
  const auto control = std::make_shared<CSettingControlSlider>();
  if (!control->SetFormat(format))
    return nullptr;

  control->SetDelayed(delayed);
  if (heading >= 0)
    control->SetHeading(heading);
  control->SetPopup(usePopup);
  if (formatLabel >= 0)
    control->SetFormatLabel(formatLabel);
  if (!formatString.empty())
    control->SetFormatString(formatString);

  return control;
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManualBase::GetRangeControl(
    const std::string& format,
    bool delayed /* = false */,
    int formatLabel /* = -1 */,
    int valueFormatLabel /* = -1 */,
    const std::string& valueFormatString /* = "" */) const
{
  const auto control = std::make_shared<CSettingControlRange>();
  if (!control->SetFormat(format))
    return nullptr;

  control->SetDelayed(delayed);
  if (formatLabel >= 0)
    control->SetFormatLabel(formatLabel);
  if (valueFormatLabel >= 0)
    control->SetValueFormatLabel(valueFormatLabel);
  if (!valueFormatString.empty())
    control->SetValueFormat(valueFormatString);

  return control;
}
