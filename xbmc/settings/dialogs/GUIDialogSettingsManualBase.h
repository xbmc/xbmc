#pragma once

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

#include <map>

#include "addons/IAddon.h"
#include "settings/dialogs/GUIDialogSettingsManagerBase.h"
#include "settings/lib/SettingDefinitions.h"

class CSetting;
class CSettingAction;
class CSettingAddon;
class CSettingBool;
class CSettingCategory;
class CSettingGroup;
class CSettingInt;
class CSettingList;
class CSettingNumber;
class CSettingPath;
class CSettingSection;
class CSettingString;

class CGUIDialogSettingsManualBase : public CGUIDialogSettingsManagerBase
{
public:
  CGUIDialogSettingsManualBase(int windowId, const std::string &xmlFile);
  virtual ~CGUIDialogSettingsManualBase();

protected:
  // implementation of CGUIDialogSettingsBase
  virtual CSettingSection* GetSection() { return m_section; }
  virtual void OnOkay();
  virtual void SetupView();

  virtual void InitializeSettings();

  CSettingCategory* AddCategory(const std::string &id, int label, int help = -1);
  CSettingGroup* AddGroup(CSettingCategory *category, int label = -1, int help = -1, bool separatorBelowLabel = true, bool hideSeparator = false);
  // checkmark control
  CSettingBool* AddToggle(CSettingGroup *group, const std::string &id, int label, int level, bool value, bool delayed = false, bool visible = true, int help = -1);
  // edit controls
  CSettingInt* AddEdit(CSettingGroup *group, const std::string &id, int label, int level, int value, int minimum = 0, int step = 1, int maximum = 0,
                       bool verifyNewValue = false, int heading = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingNumber* AddEdit(CSettingGroup *group, const std::string &id, int label, int level, float value, float minimum = 0.0f, float step = 1.0f, float maximum = 0.0f,
                          bool verifyNewValue = false, int heading = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingString* AddEdit(CSettingGroup *group, const std::string &id, int label, int level, std::string value, bool allowEmpty = false,
                          bool hidden = false, int heading = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingString* AddIp(CSettingGroup *group, const std::string &id, int label, int level, std::string value, bool allowEmpty = false,
                        int heading = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingString* AddPasswordMd5(CSettingGroup *group, const std::string &id, int label, int level, std::string value, bool allowEmpty = false,
                                 int heading = -1, bool delayed = false, bool visible = true, int help = -1);
  // button controls
  CSettingAction* AddButton(CSettingGroup *group, const std::string &id, int label, int level, bool delayed = false, bool visible = true, int help = -1);
  CSettingString* AddInfoLabelButton(CSettingGroup *group, const std::string &id, int label, int level, std::string info, bool visible = true, int help = -1);
  CSettingAddon* AddAddon(CSettingGroup *group, const std::string &id, int label, int level, std::string value, ADDON::TYPE addonType,
                          bool allowEmpty = false, int heading = -1, bool hideValue = false, bool showInstalledAddons = true, bool showInstallableAddons = false,
                          bool showMoreAddons = true, bool delayed = false, bool visible = true, int help = -1);
  CSettingPath* AddPath(CSettingGroup *group, const std::string &id, int label, int level, std::string value, bool writable = true,
                        const std::vector<std::string> &sources = std::vector<std::string>(), bool allowEmpty = false, int heading = -1, bool hideValue = false,
                        bool delayed = false, bool visible = true, int help = -1);

  // spinner controls
  CSettingString* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, std::string value, StringSettingOptionsFiller filler,
                             bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, int minimum, int step, int maximum, int formatLabel = -1,
                          int minimumLabel = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, int minimum, int step, int maximum, const std::string &formatString,
                          int minimumLabel = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, const StaticIntegerSettingOptions &entries,
                          bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, int value, IntegerSettingOptionsFiller filler,
                          bool delayed = false, bool visible = true, int help = -1);
  CSettingNumber* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, float value, float minimum, float step, float maximum,
                             int formatLabel = -1, int minimumLabel = -1, bool delayed = false, bool visible = true, int help = -1);
  CSettingNumber* AddSpinner(CSettingGroup *group, const std::string &id, int label, int level, float value, float minimum, float step, float maximum,
                             const std::string &formatString, int minimumLabel = -1, bool delayed = false, bool visible = true, int help = -1);

  // list controls
  CSettingString* AddList(CSettingGroup *group, const std::string &id, int label, int level, std::string value, StringSettingOptionsFiller filler,
                          int heading, bool visible = true, int help = -1);
  CSettingInt* AddList(CSettingGroup *group, const std::string &id, int label, int level, int value, const StaticIntegerSettingOptions &entries,
                       int heading, bool visible = true, int help = -1);
  CSettingInt* AddList(CSettingGroup *group, const std::string &id, int label, int level, int value, IntegerSettingOptionsFiller filler,
                       int heading, bool visible = true, int help = -1);
  CSettingList* AddList(CSettingGroup *group, const std::string &id, int label, int level, std::vector<std::string> values, StringSettingOptionsFiller filler,
                        int heading, int minimumItems = 0, int maximumItems = -1, bool visible = true, int help = -1);
  CSettingList* AddList(CSettingGroup *group, const std::string &id, int label, int level, std::vector<int> values, const StaticIntegerSettingOptions &entries,
                        int heading, int minimumItems = 0, int maximumItems = -1, bool visible = true, int help = -1);
  CSettingList* AddList(CSettingGroup *group, const std::string &id, int label, int level, std::vector<int> values, IntegerSettingOptionsFiller filler,
                        int heading, int minimumItems = 0, int maximumItems = -1, bool visible = true, int help = -1);

  // slider controls
  CSettingInt* AddPercentageSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, int formatLabel, int step = 1, int heading = -1,
                                   bool usePopup = false, bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddPercentageSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, const std::string &formatString, int step = 1,
                                   int heading = -1, bool usePopup = false, bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, int formatLabel, int minimum, int step, int maximum,
                         int heading = -1, bool usePopup = false, bool delayed = false, bool visible = true, int help = -1);
  CSettingInt* AddSlider(CSettingGroup *group, const std::string &id, int label, int level, int value, const std::string &formatString, int minimum, int step, int maximum,
                         int heading = -1, bool usePopup = false, bool delayed = false, bool visible = true, int help = -1);
  CSettingNumber* AddSlider(CSettingGroup *group, const std::string &id, int label, int level, float value, int formatLabel, float minimum, float step, float maximum,
                            int heading = -1, bool usePopup = false, bool delayed = false, bool visible = true, int help = -1);
  CSettingNumber* AddSlider(CSettingGroup *group, const std::string &id, int label, int level, float value, const std::string &formatString, float minimum, float step,
                            float maximum, int heading = -1, bool usePopup = false, bool delayed = false, bool visible = true, int help = -1);

  // range controls
  CSettingList* AddPercentageRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int valueFormatLabel, int step = 1,
                                   int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddPercentageRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper,
                                   const std::string &valueFormatString = "%i %%", int step = 1, int formatLabel = 21469,
                                   bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                         int valueFormatLabel, int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                         const std::string &valueFormatString = "%d", int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddRange(CSettingGroup *group, const std::string &id, int label, int level, float valueLower, float valueUpper, float minimum, float step, float maximum,
                         int valueFormatLabel, int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddRange(CSettingGroup *group, const std::string &id, int label, int level, float valueLower, float valueUpper, float minimum, float step, float maximum,
                         const std::string &valueFormatString = "%.1f", int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddDateRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                             int valueFormatLabel, int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddDateRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                             const std::string &valueFormatString = "", int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddTimeRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                             int valueFormatLabel, int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);
  CSettingList* AddTimeRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                             const std::string &valueFormatString = "mm:ss", int formatLabel = 21469, bool delayed = false, bool visible = true, int help = -1);

  ISettingControl* GetTitleControl(bool separatorBelowLabel = true, bool hideSeparator = false);
  ISettingControl* GetCheckmarkControl(bool delayed = false);
  ISettingControl* GetEditControl(const std::string &format, bool delayed = false, bool hidden = false, bool verifyNewValue = false, int heading = -1);
  ISettingControl* GetButtonControl(const std::string &format, bool delayed = false, int heading = -1, bool hideValue = false, bool showInstalledAddons = true,
                                    bool showInstallableAddons = false, bool showMoreAddons = true);
  ISettingControl* GetSpinnerControl(const std::string &format, bool delayed = false, int minimumLabel = -1, int formatLabel = -1, const std::string &formatString = "");
  ISettingControl* GetListControl(const std::string &format, bool delayed = false, int heading = -1, bool multiselect = false);
  ISettingControl* GetSliderControl(const std::string &format, bool delayed = false, int heading = -1, bool usePopup = false, int formatLabel = -1, const std::string &formatString = "");
  ISettingControl* GetRangeControl(const std::string &format, bool delayed = false, int formatLabel = -1, int valueFormatLabel = -1, const std::string &valueFormatString = "");

private:
  CSettingList* AddRange(CSettingGroup *group, const std::string &id, int label, int level, int valueLower, int valueUpper, int minimum, int step, int maximum,
                         const std::string &format, int formatLabel, int valueFormatLabel, const std::string &valueFormatString, bool delayed, bool visible, int help);
  CSettingList* AddRange(CSettingGroup *group, const std::string &id, int label, int level, float valueLower, float valueUpper, float minimum, float step, float maximum,
                         const std::string &format, int formatLabel, int valueFormatLabel, const std::string &valueFormatString, bool delayed, bool visible, int help);

  void setSettingDetails(CSetting *setting, int level, bool visible, int help);

  CSettingSection *m_section;
};
