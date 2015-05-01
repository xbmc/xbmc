/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIDialogSettingsManualBase.h"
#include "settings/SettingAddon.h"
#include "settings/SettingPath.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"

using namespace std;

CGUIDialogSettingsManualBase::CGUIDialogSettingsManualBase(int windowId, const std::string &xmlFile)
    : CGUIDialogSettingsManagerBase(windowId, xmlFile),
      m_section(NULL)
{
  m_settingsManager = new CSettingsManager();
}

CGUIDialogSettingsManualBase::~CGUIDialogSettingsManualBase()
{
  m_settingsManager->Clear();
  m_section = NULL;
  delete m_settingsManager;
}

void CGUIDialogSettingsManualBase::OnOkay()
{
  Save();

  CGUIDialogSettingsBase::OnOkay();
}

void CGUIDialogSettingsManualBase::SetupView()
{
  InitializeSettings();

  // add the created setting section to the settings manager and mark it as ready
  m_settingsManager->AddSection(m_section);
  m_settingsManager->SetInitialized();
  m_settingsManager->SetLoaded();

  CGUIDialogSettingsBase::SetupView();
}

void CGUIDialogSettingsManualBase::InitializeSettings()
{
  m_settingsManager->Clear();
  m_section = NULL;

  // create a new section
  m_section = new CSettingSection(GetProperty("xmlfile").asString(), m_settingsManager);
}

CSettingCategory* CGUIDialogSettingsManualBase::AddCategory(const std::string &id, int label, int help /* = -1 */)
{
  if (id.empty())
    return NULL;

  CSettingCategory *category = new CSettingCategory(id, m_settingsManager);
  if (category == NULL)
    return NULL;

  category->SetLabel(label);
  if (help >= 0)
    category->SetHelp(help);

  m_section->AddCategory(category);
  return category;
}

CSettingGroup* CGUIDialogSettingsManualBase::AddGroup(CSettingCategory *category, int label /* = -1 */, int help /* = -1 */, bool separatorBelowLabel /* = true */, bool hideSeparator /* = false */)
{
  if (category == NULL)
    return NULL;

  size_t groups = category->GetGroups().size();

  CSettingGroup *group = new CSettingGroup(StringUtils::Format("%zu", groups + 1), m_settingsManager);
  if (group == NULL)
    return NULL;

  if (label >= 0)
    group->SetLabel(label);
  if (help >= 0)
    group->SetHelp(help);
  group->SetControl(GetTitleControl(separatorBelowLabel, hideSeparator));

  category->AddGroup(group);
  return group;
}

CSettingBool* CGUIDialogSettingsManualBase::AddToggle(CSettingGroup *group, const std::string &id, int label, int level, bool value,
                                                      bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingBool *setting = new CSettingBool(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetCheckmarkControl(delayed));
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddEdit(CSettingGroup *group, const std::string &id, int label, int level, int value,
                                                   int minimum /* = 0 */, int step /* = 1 */, int maximum /* = 0 */, bool verifyNewValue /* = false */,
                                                   int heading /* = -1 */, bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, minimum, step, maximum, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetEditControl("integer", delayed, false, verifyNewValue, heading));
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingNumber* CGUIDialogSettingsManualBase::AddEdit(CSettingGroup *group, const std::string &id, int label, int level, float value,
                                                      float minimum /* = 0.0f */, float step /* = 1.0f */, float maximum /* = 0.0f */,
                                                      bool verifyNewValue /* = false */, int heading /* = -1 */, bool delayed /* = false */,
                                                      bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingNumber *setting = new CSettingNumber(id, label, value, minimum, step, maximum, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetEditControl("number", delayed, false, verifyNewValue, heading));
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingString* CGUIDialogSettingsManualBase::AddEdit(CSettingGroup *group, const std::string &id, int label, int level, std::string value,
                                                      bool allowEmpty /* = false */, bool hidden /* = false */,
                                                      int heading /* = -1 */, bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *setting = new CSettingString(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetEditControl("string", delayed, hidden, false, heading));
  setting->SetAllowEmpty(allowEmpty);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingString* CGUIDialogSettingsManualBase::AddIp(CSettingGroup *group, const std::string &id, int label, int level, std::string value,
                                                    bool allowEmpty /* = false */, int heading /* = -1 */, bool delayed /* = false */,
                                                    bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *setting = new CSettingString(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetEditControl("ip", delayed, false, false, heading));
  setting->SetAllowEmpty(allowEmpty);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingString* CGUIDialogSettingsManualBase::AddPasswordMd5(CSettingGroup *group, const std::string &id, int label, int level, std::string value,
                                                             bool allowEmpty /* = false */, int heading /* = -1 */, bool delayed /* = false */,
                                                             bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *setting = new CSettingString(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetEditControl("md5", delayed, false, false, heading));
  setting->SetAllowEmpty(allowEmpty);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingAction* CGUIDialogSettingsManualBase::AddButton(CSettingGroup *group, const std::string &id, int label, int level, bool delayed /* = false */,
                                                        bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingAction *setting = new CSettingAction(id, label, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetButtonControl("action", delayed));
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingString* CGUIDialogSettingsManualBase::AddInfoLabelButton(CSettingGroup *group, const std::string &id, int label, int level, std::string info,
                                                                 bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *setting = new CSettingString(id, label, info, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetButtonControl("infolabel", false));
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingAddon* CGUIDialogSettingsManualBase::AddAddon(CSettingGroup *group, const std::string &id, int label, int level, std::string value, ADDON::TYPE addonType,
                                                      bool allowEmpty /* = false */, int heading /* = -1 */, bool hideValue /* = false */, bool showInstalledAddons /* = true */,
                                                      bool showInstallableAddons /* = false */, bool showMoreAddons /* = true */, bool delayed /* = false */,
                                                      bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingAddon *setting = new CSettingAddon(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetButtonControl("addon", delayed, heading, hideValue, showInstalledAddons, showInstallableAddons, showMoreAddons));
  setting->SetAddonType(addonType);
  setting->SetAllowEmpty(allowEmpty);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingPath* CGUIDialogSettingsManualBase::AddPath(CSettingGroup *group, const std::string &id, int label, int level, std::string value, bool writable /* = true */,
                                                    const std::vector<std::string> &sources /* = std::vector<std::string>() */, bool allowEmpty /* = false */,
                                                    int heading /* = -1 */, bool hideValue /* = false */, bool delayed /* = false */,
                                                    bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingPath *setting = new CSettingPath(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetButtonControl("path", delayed, heading, hideValue));
  setting->SetWritable(writable);
  setting->SetSources(sources);
  setting->SetAllowEmpty(allowEmpty);
  setting->SetAllowEmpty(allowEmpty);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingString* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, std::string value,
                                                         StringSettingOptionsFiller filler, bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || filler == NULL ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *setting = new CSettingString(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetOptionsFiller(filler, this);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, int minimum, int step, int maximum,
                                                      int formatLabel /* = -1 */, int minimumLabel /* = -1 */, bool delayed /* = false */,
                                                      bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("string", delayed, minimumLabel, formatLabel));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, int minimum, int step, int maximum,
                                                      const std::string &formatString, int minimumLabel /* = -1 */, bool delayed /* = false */,
                                                      bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("string", delayed, minimumLabel, -1, formatString));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, const StaticIntegerSettingOptions &entries,
                                                      bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || entries.empty() ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetOptions(entries);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, IntegerSettingOptionsFiller filler,
                                                      bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || filler == NULL ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("string", delayed));
  setting->SetOptionsFiller(filler, this);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingNumber* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, float value, float minimum, float step, float maximum,
                                                         int formatLabel /* = -1 */, int minimumLabel /* = -1 */, bool delayed /* = false */,
                                                         bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingNumber *setting = new CSettingNumber(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("number", delayed, minimumLabel, formatLabel));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingNumber* CGUIDialogSettingsManualBase::AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, float value, float minimum, float step, float maximum,
                                                         const std::string &formatString, int minimumLabel /* = -1 */, bool delayed /* = false */,
                                                         bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingNumber *setting = new CSettingNumber(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSpinnerControl("number", delayed, minimumLabel, -1, formatString));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingString* CGUIDialogSettingsManualBase::AddList(CSettingGroup *group, const std::string &id, int label, int level, std::string value,
                                                      StringSettingOptionsFiller filler, int heading, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || filler == NULL ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *setting = new CSettingString(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetListControl("string", false, heading, false));
  setting->SetOptionsFiller(filler, this);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddList(CSettingGroup *group, const std::string &id, int label, int level, int value, const StaticIntegerSettingOptions &entries,
                                                  int heading, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || entries.empty() ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetListControl("integer", false, heading, false));
  setting->SetOptions(entries);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddList(CSettingGroup *group, const std::string &id, int label, int level, int value, IntegerSettingOptionsFiller filler,
                                                   int heading, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || filler == NULL ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetListControl("integer", false, heading, false));
  setting->SetOptionsFiller(filler, this);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingList* CGUIDialogSettingsManualBase::AddList(CSettingGroup *group, const std::string &id, int label, int level, std::vector<std::string> values,
                                                    StringSettingOptionsFiller filler, int heading, int minimumItems /* = 0 */, int maximumItems /* = -1 */,
                                                    bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || filler == NULL ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingString *settingDefinition = new CSettingString(id, m_settingsManager);
  if (settingDefinition == NULL)
    return NULL;
  
  settingDefinition->SetOptionsFiller(filler, this);

  CSettingList *setting = new CSettingList(id, settingDefinition, label, m_settingsManager);
  if (setting == NULL)
  {
    delete settingDefinition;
    return NULL;
  }

  std::vector<CVariant> valueList;
  for (std::vector<std::string>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
    valueList.push_back(CVariant(*itValue));
  SettingPtrList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
  {
    delete settingDefinition;
    delete setting;
    return NULL;
  }
  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("string", false, heading, true));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingList* CGUIDialogSettingsManualBase::AddList(CSettingGroup *group, const std::string &id, int label, int level, std::vector<int> values,
                                                    const StaticIntegerSettingOptions &entries, int heading, int minimumItems /* = 0 */, int maximumItems /* = -1 */,
                                                    bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || entries.empty() ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *settingDefinition = new CSettingInt(id, m_settingsManager);
  if (settingDefinition == NULL)
    return NULL;
  
  settingDefinition->SetOptions(entries);

  CSettingList *setting = new CSettingList(id, settingDefinition, label, m_settingsManager);
  if (setting == NULL)
  {
    delete settingDefinition;
    return NULL;
  }

  std::vector<CVariant> valueList;
  for (std::vector<int>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
    valueList.push_back(CVariant(*itValue));
  SettingPtrList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
  {
    delete settingDefinition;
    delete setting;
    return NULL;
  }
  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("integer", false, heading, true));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingList* CGUIDialogSettingsManualBase::AddList(CSettingGroup *group, const std::string &id, int label, int level, std::vector<int> values,
                                                    IntegerSettingOptionsFiller filler, int heading, int minimumItems /* = 0 */, int maximumItems /* = -1 */,
                                                    bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 || filler == NULL ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *settingDefinition = new CSettingInt(id, m_settingsManager);
  if (settingDefinition == NULL)
    return NULL;
  
  settingDefinition->SetOptionsFiller(filler, this);

  CSettingList *setting = new CSettingList(id, settingDefinition, label, m_settingsManager);
  if (setting == NULL)
  {
    delete settingDefinition;
    return NULL;
  }

  std::vector<CVariant> valueList;
  for (std::vector<int>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
    valueList.push_back(CVariant(*itValue));
  SettingPtrList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
  {
    delete settingDefinition;
    delete setting;
    return NULL;
  }
  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);

  setting->SetControl(GetListControl("integer", false, heading, true));
  setting->SetMinimumItems(minimumItems);
  setting->SetMaximumItems(maximumItems);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddPercentageSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, int formatLabel,
                                                               int step /* = 1 */, int heading /* = -1 */, bool usePopup /* = false */, bool delayed /* = false */,
                                                               bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSliderControl("percentage", delayed, heading, usePopup, formatLabel));
  setting->SetMinimum(0);
  setting->SetStep(step);
  setting->SetMaximum(100);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddPercentageSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, const std::string &formatString,
                                                               int step /* = 1 */, int heading /* = -1 */, bool usePopup /* = false */, bool delayed /* = false */,
                                                               bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSliderControl("percentage", delayed, heading, usePopup, -1, formatString));
  setting->SetMinimum(0);
  setting->SetStep(step);
  setting->SetMaximum(100);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, int formatLabel, int minimum, int step,
                                                     int maximum, int heading /* = -1 */, bool usePopup /* = false */, bool delayed /* = false */,
                                                     bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSliderControl("integer", delayed, heading, usePopup, formatLabel));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingInt* CGUIDialogSettingsManualBase::AddSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, const std::string &formatString,
                                                     int minimum, int step, int maximum, int heading /* = -1 */, bool usePopup /* = false */, bool delayed /* = false */,
                                                     bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *setting = new CSettingInt(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSliderControl("integer", delayed, heading, usePopup, -1, formatString));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingNumber* CGUIDialogSettingsManualBase::AddSlider(CSettingGroup *group, const std::string &id, int label, int level, float value, int formatLabel, float minimum,
                                                        float step, float maximum, int heading /* = -1 */, bool usePopup /* = false */, bool delayed /* = false */,
                                                        bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingNumber *setting = new CSettingNumber(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSliderControl("number", delayed, heading, usePopup, formatLabel));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingNumber* CGUIDialogSettingsManualBase::AddSlider(CSettingGroup *group, const std::string &id, int label, int level, float value, const std::string &formatString,
                                                        float minimum, float step, float maximum, int heading /* = -1 */, bool usePopup /* = false */,
                                                        bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingNumber *setting = new CSettingNumber(id, label, value, m_settingsManager);
  if (setting == NULL)
    return NULL;

  setting->SetControl(GetSliderControl("number", delayed, heading, usePopup, -1, formatString));
  setting->SetMinimum(minimum);
  setting->SetStep(step);
  setting->SetMaximum(maximum);
  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingList* CGUIDialogSettingsManualBase::AddPercentageRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper,
                                                               int valueFormatLabel, int step /* = 1 */, int formatLabel /* = 21469 */, bool delayed /* = false */,
                                                               bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, 0, step, 100, "percentage", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddPercentageRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper,
                                                               const std::string &valueFormatString /* = "%i %%" */, int step /* = 1 */, int formatLabel /* = 21469 */,
                                                               bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, 0, step, 100, "percentage", formatLabel, -1, valueFormatString, delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                     int step, int maximum, int valueFormatLabel, int formatLabel /* = 21469 */, bool delayed /* = false */,
                                                     bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "integer", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                     int step, int maximum, const std::string &valueFormatString /* = "%d" */, int formatLabel /* = 21469 */,
                                                     bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "integer", formatLabel, -1, valueFormatString, delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddRange(CSettingGroup *group, const std::string &id, int label, int level, float valueLower, float valueUpper, float minimum,
                                                     float step, float maximum, int valueFormatLabel, int formatLabel /* = 21469 */, bool delayed /* = false */,
                                                     bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "number", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddRange(CSettingGroup *group, const std::string &id, int label, int level, float valueLower, float valueUpper, float minimum,
                                                     float step, float maximum, const std::string &valueFormatString /* = "%.1f" */, int formatLabel /* = 21469 */,
                                                     bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "number", formatLabel, -1, valueFormatString, delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddDateRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                         int step, int maximum, int valueFormatLabel, int formatLabel /* = 21469 */, bool delayed /* = false */,
                                                         bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "date", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddDateRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                         int step, int maximum, const std::string &valueFormatString /* = "" */, int formatLabel /* = 21469 */,
                                                         bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "date", formatLabel, -1, valueFormatString, delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddTimeRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                         int step, int maximum, int valueFormatLabel, int formatLabel /* = 21469 */, bool delayed /* = false */,
                                                         bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "time", formatLabel, valueFormatLabel, "", delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddTimeRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                         int step, int maximum, const std::string &valueFormatString /* = "mm:ss" */, int formatLabel /* = 21469 */,
                                                         bool delayed /* = false */, bool visible /* = true */, int help /* = -1 */)
{
  return AddRange(group, id, label, level, valueLower, valueUpper, minimum, step, maximum, "time", formatLabel, -1, valueFormatString, delayed, visible, help);
}

CSettingList* CGUIDialogSettingsManualBase::AddRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum,
                                                     int step, int maximum, const std::string &format, int formatLabel, int valueFormatLabel,
                                                     const std::string &valueFormatString, bool delayed, bool visible, int help)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingInt *settingDefinition = new CSettingInt(id, m_settingsManager);
  if (settingDefinition == NULL)
    return NULL;
  
  settingDefinition->SetMinimum(minimum);
  settingDefinition->SetStep(step);
  settingDefinition->SetMaximum(maximum);

  CSettingList *setting = new CSettingList(id, settingDefinition, label, m_settingsManager);
  if (setting == NULL)
  {
    delete settingDefinition;
    return NULL;
  }

  std::vector<CVariant> valueList;
  valueList.push_back(valueLower);
  valueList.push_back(valueUpper);
  SettingPtrList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
  {
    delete settingDefinition;
    delete setting;
    return NULL;
  }
  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);
  
  setting->SetControl(GetRangeControl(format, delayed, formatLabel, valueFormatLabel, valueFormatString));
  setting->SetMinimumItems(2);
  setting->SetMaximumItems(2);

  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

CSettingList* CGUIDialogSettingsManualBase::AddRange(CSettingGroup *group, const std::string &id, int label, int level, float valueLower, float valueUpper, float minimum,
                                                     float step, float maximum, const std::string &format, int formatLabel, int valueFormatLabel,
                                                     const std::string &valueFormatString, bool delayed, bool visible, int help)
{
  if (group == NULL || id.empty() || label < 0 ||
      GetSetting(id) != NULL)
    return NULL;

  CSettingNumber *settingDefinition = new CSettingNumber(id, m_settingsManager);
  if (settingDefinition == NULL)
    return NULL;
  
  settingDefinition->SetMinimum(minimum);
  settingDefinition->SetStep(step);
  settingDefinition->SetMaximum(maximum);

  CSettingList *setting = new CSettingList(id, settingDefinition, label, m_settingsManager);
  if (setting == NULL)
  {
    delete settingDefinition;
    return NULL;
  }

  std::vector<CVariant> valueList;
  valueList.push_back(valueLower);
  valueList.push_back(valueUpper);
  SettingPtrList settingValues;
  if (!CSettingUtils::ValuesToList(setting, valueList, settingValues))
  {
    delete settingDefinition;
    delete setting;
    return NULL;
  }
  // setting the default will also set the actual value on an unchanged setting
  setting->SetDefault(settingValues);
  
  setting->SetControl(GetRangeControl(format, delayed, formatLabel, valueFormatLabel, valueFormatString));
  setting->SetMinimumItems(2);
  setting->SetMaximumItems(2);

  setSettingDetails(setting, level, visible, help);

  group->AddSetting(setting);
  return setting;
}

void CGUIDialogSettingsManualBase::setSettingDetails(CSetting *setting, int level, bool visible, int help)
{
  if (setting == NULL)
    return;

  if (level < 0)
    level = SettingLevelBasic;
  else if (level > SettingLevelExpert)
    level = SettingLevelExpert;

  setting->SetLevel(static_cast<SettingLevel>(level));
  setting->SetVisible(visible);
  if (help >= 0)
    setting->SetHelp(help);
}

ISettingControl* CGUIDialogSettingsManualBase::GetCheckmarkControl(bool delayed /* = false */)
{
  CSettingControlCheckmark *control = new CSettingControlCheckmark();
  control->SetDelayed(delayed);

  return control;
}

ISettingControl* CGUIDialogSettingsManualBase::GetTitleControl(bool separatorBelowLabel /* = true */, bool hideSeparator /* = false */)
{
  CSettingControlTitle *control = new CSettingControlTitle();
  control->SetSeparatorBelowLabel(separatorBelowLabel);
  control->SetSeparatorHidden(hideSeparator);

  return control;
}

ISettingControl* CGUIDialogSettingsManualBase::GetEditControl(const std::string &format, bool delayed /* = false */, bool hidden /* = false */, bool verifyNewValue /* = false */, int heading /* = -1 */)
{
  CSettingControlEdit *control = new CSettingControlEdit();
  if (!control->SetFormat(format))
  {
    delete control;
    return NULL;
  }
  
  control->SetDelayed(delayed);
  control->SetHidden(hidden);
  control->SetVerifyNewValue(verifyNewValue);
  control->SetHeading(heading);

  return control;
}

ISettingControl* CGUIDialogSettingsManualBase::GetButtonControl(const std::string &format, bool delayed /* = false */, int heading /* = -1 */, bool hideValue /* = false */,
                                                                bool showInstalledAddons /* = true */, bool showInstallableAddons /* = false */, bool showMoreAddons /* = true */)
{
  CSettingControlButton *control = new CSettingControlButton();
  if (!control->SetFormat(format))
  {
    delete control;
    return NULL;
  }
  
  control->SetDelayed(delayed);
  control->SetHeading(heading);
  control->SetHideValue(hideValue);
  control->SetShowInstalledAddons(showInstalledAddons);
  control->SetShowInstallableAddons(showInstallableAddons);
  control->SetShowMoreAddons(showMoreAddons);

  return control;
}

ISettingControl* CGUIDialogSettingsManualBase::GetSpinnerControl(const std::string &format, bool delayed /* = false */, int minimumLabel /* = -1 */, int formatLabel /* = -1 */, const std::string &formatString /* = "" */)
{
  CSettingControlSpinner *control = new CSettingControlSpinner();
  if (!control->SetFormat(format))
  {
    delete control;
    return NULL;
  }
  
  control->SetDelayed(delayed);
  if (formatLabel >= 0)
    control->SetFormatLabel(formatLabel);
  if (!formatString.empty())
    control->SetFormatString(formatString);
  if (minimumLabel >= 0)
    control->SetMinimumLabel(minimumLabel);

  return control;
}

ISettingControl* CGUIDialogSettingsManualBase::GetListControl(const std::string &format, bool delayed /* = false */, int heading /* = -1 */, bool multiselect /* = false */)
{
  CSettingControlList *control = new CSettingControlList();
  if (!control->SetFormat(format))
  {
    delete control;
    return NULL;
  }

  control->SetDelayed(delayed);
  control->SetHeading(heading);
  control->SetMultiSelect(multiselect);

  return control;
}

ISettingControl* CGUIDialogSettingsManualBase::GetSliderControl(const std::string &format, bool delayed /* = false */, int heading /* = -1 */, bool usePopup /* = false */,
                                                                int formatLabel /* = -1 */, const std::string &formatString /* = "" */)
{
  CSettingControlSlider *control = new CSettingControlSlider();
  if (!control->SetFormat(format))
  {
    delete control;
    return NULL;
  }

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

ISettingControl* CGUIDialogSettingsManualBase::GetRangeControl(const std::string &format, bool delayed /* = false */, int formatLabel /* = -1 */,
                                                               int valueFormatLabel /* = -1 */, const std::string &valueFormatString /* = "" */)
{
  CSettingControlRange *control = new CSettingControlRange();
  if (!control->SetFormat(format))
  {
    delete control;
    return NULL;
  }

  control->SetDelayed(delayed);
  if (formatLabel >= 0)
    control->SetFormatLabel(formatLabel);
  if (valueFormatLabel >= 0)
    control->SetValueFormatLabel(valueFormatLabel);
  if (!valueFormatString.empty())
    control->SetValueFormat(valueFormatString);

  return control;
}
