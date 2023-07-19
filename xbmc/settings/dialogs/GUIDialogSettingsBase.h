/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "settings/SettingControl.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/Timer.h"
#include "utils/ILocalizer.h"

#include <set>
#include <vector>

#define CONTROL_SETTINGS_LABEL 2
#define CONTROL_SETTINGS_DESCRIPTION 6

#define CONTROL_SETTINGS_OKAY_BUTTON 28
#define CONTROL_SETTINGS_CANCEL_BUTTON 29
#define CONTROL_SETTINGS_CUSTOM_BUTTON 30

#define CONTROL_SETTINGS_START_BUTTONS -100
#define CONTROL_SETTINGS_START_CONTROL -80

#define SETTINGS_RESET_SETTING_ID "settings.reset"
#define SETTINGS_EMPTY_CATEGORY_ID "categories.empty"

class CGUIControl;
class CGUIControlBaseSetting;
class CGUIImage;
class CGUISpinControlEx;
class CGUIEditControl;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;
class CGUILabelControl;
class CGUIColorButtonControl;

class CSetting;
class CSettingAction;
class CSettingCategory;
class CSettingGroup;
class CSettingSection;

class CVariant;

class ISetting;

typedef std::shared_ptr<CGUIControlBaseSetting> BaseSettingControlPtr;

class CGUIDialogSettingsBase : public CGUIDialog,
                               public CSettingControlCreator,
                               public ILocalizer,
                               protected ITimerCallback,
                               protected ISettingCallback
{
public:
  CGUIDialogSettingsBase(int windowId, const std::string& xmlFile);
  ~CGUIDialogSettingsBase() override;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  bool OnBack(int actionID) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;

  virtual bool IsConfirmed() const { return m_confirmed; }

  // implementation of ILocalizer
  std::string Localize(std::uint32_t code) const override { return GetLocalizedString(code); }

protected:
  // specializations of CGUIWindow
  void OnInitWindow() override;

  // implementations of ITimerCallback
  void OnTimeout() override;

  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingPropertyChanged(const std::shared_ptr<const CSetting>& setting,
                                const char* propertyName) override;

  // new virtual methods
  virtual bool AllowResettingSettings() const { return true; }
  virtual int GetSettingLevel() const { return 0; }
  virtual std::shared_ptr<CSettingSection> GetSection() = 0;
  virtual std::shared_ptr<CSetting> GetSetting(const std::string& settingId) = 0;
  virtual std::chrono::milliseconds GetDelayMs() const { return std::chrono::milliseconds(1500); }
  virtual std::string GetLocalizedString(uint32_t labelId) const;

  virtual bool OnOkay()
  {
    m_confirmed = true;
    return true;
  }
  virtual void OnCancel() {}

  virtual void SetupView();
  virtual std::set<std::string> CreateSettings();
  virtual void UpdateSettings();

  /*!
    \brief Get the name for the setting entry

    Used as virtual to allow related settings dialog to give a std::string name of the setting.
    If not used on own dialog class it handle the string from int CSetting::GetLabel(),
    This must also be used if on related dialog no special entry is wanted.

    \param pSetting Base settings class which need the name
    \return Name used on settings dialog
   */
  virtual std::string GetSettingsLabel(const std::shared_ptr<ISetting>& pSetting);

  virtual CGUIControl* AddSetting(const std::shared_ptr<CSetting>& pSetting,
                                  float width,
                                  int& iControlID);
  virtual CGUIControl* AddSettingControl(CGUIControl* pControl,
                                         BaseSettingControlPtr pSettingControl,
                                         float width,
                                         int& iControlID);

  virtual void SetupControls(bool createSettings = true);
  virtual void FreeControls();
  virtual void DeleteControls();
  virtual void FreeSettingsControls();

  virtual void SetHeading(const CVariant& label);
  virtual void SetDescription(const CVariant& label);

  virtual void OnResetSettings();

  /*!
    \brief A setting control has been interacted with by the user

    This method is called when the user manually interacts (clicks,
    edits) with a setting control. It contains handling for both
    delayed and undelayed settings and either starts the delay timer
    or triggers the setting change which, on success, results in a
    callback to OnSettingChanged().

    \param pSettingControl Setting control that has been interacted with
   */
  virtual void OnClick(const BaseSettingControlPtr& pSettingControl);

  void UpdateSettingControl(const std::string& settingId, bool updateDisplayOnly = false);
  void UpdateSettingControl(const BaseSettingControlPtr& pSettingControl,
                            bool updateDisplayOnly = false);
  void SetControlLabel(int controlId, const CVariant& label);

  BaseSettingControlPtr GetSettingControl(const std::string& setting);
  BaseSettingControlPtr GetSettingControl(int controlId);

  CGUIControl* AddSeparator(float width, int& iControlID);
  CGUIControl* AddGroupLabel(const std::shared_ptr<CSettingGroup>& group,
                             float width,
                             int& iControlID);

  std::vector<std::shared_ptr<CSettingCategory>> m_categories;
  std::vector<BaseSettingControlPtr> m_settingControls;

  int m_iSetting = 0;
  int m_iCategory = 0;
  std::shared_ptr<CSettingAction> m_resetSetting;
  std::shared_ptr<CSettingCategory> m_dummyCategory;

  CGUISpinControlEx* m_pOriginalSpin;
  CGUISettingsSliderControl* m_pOriginalSlider;
  CGUIRadioButtonControl* m_pOriginalRadioButton;
  CGUIColorButtonControl* m_pOriginalColorButton = nullptr;
  CGUIButtonControl* m_pOriginalCategoryButton;
  CGUIButtonControl* m_pOriginalButton;
  CGUIEditControl* m_pOriginalEdit;
  CGUIImage* m_pOriginalImage;
  CGUILabelControl* m_pOriginalGroupTitle;
  bool m_newOriginalEdit = false;

  BaseSettingControlPtr
      m_delayedSetting; ///< Current delayed setting \sa CBaseSettingControl::SetDelayed()
  CTimer m_delayedTimer; ///< Delayed setting timer

  bool m_confirmed = false;
  int m_focusedControl = 0, m_fadedControl = 0;
};
