/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSettingsBase.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIColorButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIToggleButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/SettingControl.h"
#include "settings/lib/SettingSection.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#if defined(TARGET_WINDOWS) // disable 4355: 'this' used in base member initializer list
#pragma warning(push)
#pragma warning(disable : 4355)
#endif // defined(TARGET_WINDOWS)

namespace
{
constexpr int CATEGORY_GROUP_ID = 3;
constexpr int SETTINGS_GROUP_ID = 5;

constexpr int CONTROL_DEFAULT_BUTTON = 7;
constexpr int CONTROL_DEFAULT_RADIOBUTTON = 8;
constexpr int CONTROL_DEFAULT_SPIN = 9;
constexpr int CONTROL_DEFAULT_CATEGORY_BUTTON = 10;
constexpr int CONTROL_DEFAULT_SEPARATOR = 11;
constexpr int CONTROL_DEFAULT_EDIT = 12;
constexpr int CONTROL_DEFAULT_SLIDER = 13;
constexpr int CONTROL_DEFAULT_SETTING_LABEL = 14;
constexpr int CONTROL_DEFAULT_COLORBUTTON = 15;
} //unnamed namespace

CGUIDialogSettingsBase::CGUIDialogSettingsBase(int windowId, const std::string& xmlFile)
  : CGUIDialog(windowId, xmlFile), m_delayedTimer(this)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSettingsBase::~CGUIDialogSettingsBase()
{
  FreeControls();
  DeleteControls();
}

bool CGUIDialogSettingsBase::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_delayedSetting.reset();
      if (message.GetParam1() != WINDOW_INVALID)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iCategory = 0;
        ResetControlStates();
      }

      if (AllowResettingSettings())
      {
        m_resetSetting = std::make_shared<CSettingAction>(SETTINGS_RESET_SETTING_ID);
        m_resetSetting->SetLabel(10041);
        m_resetSetting->SetHelp(10045);
        m_resetSetting->SetControl(CreateControl("button"));
      }

      m_dummyCategory = std::make_shared<CSettingCategory>(SETTINGS_EMPTY_CATEGORY_ID);
      m_dummyCategory->SetLabel(10046);
      m_dummyCategory->SetHelp(10047);
      break;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      // cancel any delayed changes
      if (m_delayedSetting)
      {
        m_delayedTimer.Stop();
        CGUIMessage msg(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID());
        OnMessage(msg);
      }

      CGUIDialog::OnMessage(message);
      FreeControls();
      return true;
    }

    case GUI_MSG_FOCUSED:
    {
      CGUIDialog::OnMessage(message);
      m_focusedControlID = GetFocusedControlID();

      // cancel any delayed changes
      if (m_delayedSetting && m_delayedSetting->GetID() != m_focusedControlID)
      {
        m_delayedTimer.Stop();
        // param1 = 1 for "reset the control if it's invalid"
        CGUIMessage msg(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID(), 1);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
      }
      // update the value of the previous setting (in case it was invalid)
      else if (m_iSetting >= CONTROL_SETTINGS_START_CONTROL &&
               m_iSetting <
                   static_cast<int>(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr control = GetSettingControl(m_iSetting);
        if (control && control->GetSetting() && !control->IsValid())
        {
          // param1 = 1 for "reset the control if it's invalid"
          // param2 = 1 for "only update the current value"
          CGUIMessage msg(GUI_MSG_UPDATE_ITEM, GetID(), m_iSetting, 1, 1);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
        }
      }

      CVariant description;

      // check if we have changed the category and need to create new setting controls
      if (m_focusedControlID >= CONTROL_SETTINGS_START_BUTTONS &&
          m_focusedControlID <
              static_cast<int>(CONTROL_SETTINGS_START_BUTTONS + m_categories.size()))
      {
        int categoryIndex = m_focusedControlID - CONTROL_SETTINGS_START_BUTTONS;
        SettingCategoryPtr category = m_categories.at(categoryIndex);
        if (categoryIndex != m_iCategory)
        {
          if (!category->CanAccess())
          {
            // unable to go to this category - focus the previous one
            SET_CONTROL_FOCUS(CONTROL_SETTINGS_START_BUTTONS + m_iCategory, 0);
            return false;
          }

          m_iCategory = categoryIndex;
          CreateSettings();
        }

        description = category->GetHelp();
      }
      else if (m_focusedControlID >= CONTROL_SETTINGS_START_CONTROL &&
               m_focusedControlID <
                   static_cast<int>(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        m_iSetting = m_focusedControlID;
        std::shared_ptr<CSetting> setting = GetSettingControl(m_focusedControlID)->GetSetting();
        if (setting)
          description = setting->GetHelp();
      }

      // set the description of the currently focused category/setting
      if (description.isInteger() || (description.isString() && !description.empty()))
        SetDescription(description);

      return true;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_SETTINGS_OKAY_BUTTON)
      {
        if (OnOkay())
        {
          Close();
          return true;
        }

        return false;
      }

      if (iControl == CONTROL_SETTINGS_CANCEL_BUTTON)
      {
        OnCancel();
        Close();
        return true;
      }

      BaseSettingControlPtr control = GetSettingControl(iControl);
      if (control)
        OnClick(control);

      break;
    }

    case GUI_MSG_UPDATE_ITEM:
    {
      if (m_delayedSetting && m_delayedSetting->GetID() == message.GetControlId())
      {
        // first get the delayed setting and reset its member variable
        // to avoid handling the delayed setting twice in case the OnClick()
        // performed later causes the window to be deinitialized (e.g. when
        // changing the language)
        BaseSettingControlPtr delayedSetting = m_delayedSetting;
        m_delayedSetting.reset();

        // if updating the setting fails and param1 has been specifically set
        // we need to call OnSettingChanged() to restore a valid value in the
        // setting control
        if (!delayedSetting->OnClick() && message.GetParam1() != 0)
          OnSettingChanged(delayedSetting->GetSetting());
        return true;
      }

      if (message.GetControlId() >= CONTROL_SETTINGS_START_CONTROL &&
          message.GetControlId() <
              static_cast<int>(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr settingControl = GetSettingControl(message.GetControlId());
        if (settingControl && settingControl->GetSetting())
        {
          settingControl->UpdateFromSetting(message.GetParam2() != 0);
          return true;
        }
      }
      break;
    }

    case GUI_MSG_UPDATE:
    {
      if (IsActive() && HasID(message.GetSenderId()))
      {
        int focusedControl = GetFocusedControlID();
        CreateSettings();
        SET_CONTROL_FOCUS(focusedControl, 0);
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogSettingsBase::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SETTINGS_RESET:
    {
      OnResetSettings();
      return true;
    }

    case ACTION_DELETE_ITEM:
    {
      if (m_iSetting >= CONTROL_SETTINGS_START_CONTROL &&
          m_iSetting < static_cast<int>(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        const BaseSettingControlPtr settingControl = GetSettingControl(m_iSetting);
        if (settingControl)
        {
          std::shared_ptr<CSetting> setting = settingControl->GetSetting();
          if (setting)
          {
            setting->Reset();
            return true;
          }
        }
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSettingsBase::OnBack(int actionID)
{
  m_lastControlID = 0; // don't save the control as we go to a different window each time

  // if the setting dialog is not a window but a dialog we need to close differently
  if (!IsDialog())
    return CGUIWindow::OnBack(actionID);

  return CGUIDialog::OnBack(actionID);
}

void CGUIDialogSettingsBase::DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // update alpha status of current button
  CGUIControl* control = GetFirstFocusableControl(CONTROL_SETTINGS_START_BUTTONS + m_iCategory);
  if (control)
  {
    if (m_fadedControlID &&
        (m_fadedControlID != control->GetID() || m_fadedControlID == m_focusedControlID))
    {
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
        static_cast<CGUIButtonControl*>(control)->SetAlpha(0xFF);
      else
        static_cast<CGUIButtonControl*>(control)->SetSelected(false);
      m_fadedControlID = 0;
    }

    if (!control->HasFocus())
    {
      m_fadedControlID = control->GetID();
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
        static_cast<CGUIButtonControl*>(control)->SetAlpha(0x80);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
        static_cast<CGUIButtonControl*>(control)->SetSelected(true);
      else
        m_fadedControlID = 0;

      if (m_fadedControlID)
        control->SetFocus(true);
    }
  }
  CGUIDialog::DoProcess(currentTime, dirtyregions);
}

void CGUIDialogSettingsBase::OnInitWindow()
{
  m_confirmed = false;
  SetupView();
  CGUIDialog::OnInitWindow();
}

void CGUIDialogSettingsBase::SetupControls(bool createSettings /* = true */)
{
  // cleanup first, if necessary
  FreeControls();

  // get all controls
  m_pOriginalSpin = dynamic_cast<CGUISpinControlEx*>(GetControl(CONTROL_DEFAULT_SPIN));
  m_pOriginalSlider = dynamic_cast<CGUISettingsSliderControl*>(GetControl(CONTROL_DEFAULT_SLIDER));
  m_pOriginalRadioButton =
      dynamic_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_DEFAULT_RADIOBUTTON));
  m_pOriginalCategoryButton =
      dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON));
  m_pOriginalButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_DEFAULT_BUTTON));
  m_pOriginalImage = dynamic_cast<CGUIImage*>(GetControl(CONTROL_DEFAULT_SEPARATOR));
  m_pOriginalEdit = dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_DEFAULT_EDIT));
  m_pOriginalGroupTitle =
      dynamic_cast<CGUILabelControl*>(GetControl(CONTROL_DEFAULT_SETTING_LABEL));
  m_pOriginalColorButton =
      dynamic_cast<CGUIColorButtonControl*>(GetControl(CONTROL_DEFAULT_COLORBUTTON));

  // if there's no edit control but there's a button control use that instead
  if (!m_pOriginalEdit && m_pOriginalButton)
  {
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
    m_newOriginalEdit = true;
  }

  // hide all default controls by default
  if (m_pOriginalSpin)
    m_pOriginalSpin->SetVisible(false);
  if (m_pOriginalSlider)
    m_pOriginalSlider->SetVisible(false);
  if (m_pOriginalRadioButton)
    m_pOriginalRadioButton->SetVisible(false);
  if (m_pOriginalButton)
    m_pOriginalButton->SetVisible(false);
  if (m_pOriginalCategoryButton)
    m_pOriginalCategoryButton->SetVisible(false);
  if (m_pOriginalEdit)
    m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage)
    m_pOriginalImage->SetVisible(false);
  if (m_pOriginalGroupTitle)
    m_pOriginalGroupTitle->SetVisible(false);
  if (m_pOriginalColorButton)
    m_pOriginalColorButton->SetVisible(false);

  // get the section
  SettingSectionPtr section = GetSection();
  if (!section)
    return;

  // update the screen string
  if (section->GetLabel() >= 0)
    SetHeading(section->GetLabel());

  // get the categories we need
  m_categories = section->GetCategories((SettingLevel)GetSettingLevel());
  if (m_categories.empty())
    m_categories.push_back(m_dummyCategory);

  if (m_pOriginalCategoryButton)
  {
    // setup our control groups...
    auto* group = dynamic_cast<CGUIControlGroupList*>(GetControl(CATEGORY_GROUP_ID));
    if (!group)
      return;

    // go through the categories and create the necessary buttons
    int buttonIdOffset = 0;
    for (SettingCategoryList::const_iterator category = m_categories.begin();
         category != m_categories.end(); ++category)
    {
      CGUIButtonControl* pButton = nullptr;
      if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
        pButton = new CGUIToggleButtonControl(
            *static_cast<CGUIToggleButtonControl*>(m_pOriginalCategoryButton));
      else if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_COLORBUTTON)
        pButton = new CGUIColorButtonControl(
            *static_cast<CGUIColorButtonControl*>(m_pOriginalCategoryButton));
      else
        pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
      pButton->SetLabel(GetSettingsLabel(*category));
      pButton->SetID(CONTROL_SETTINGS_START_BUTTONS + buttonIdOffset);
      pButton->SetVisible(true);
      pButton->AllocResources();

      group->AddControl(pButton);
      buttonIdOffset++;
    }
  }

  if (createSettings)
    CreateSettings();

  // set focus correctly depending on whether there are categories visible or not
  if (!m_pOriginalCategoryButton &&
      (m_defaultControl <= 0 || m_defaultControl == CATEGORY_GROUP_ID))
    m_defaultControl = SETTINGS_GROUP_ID;
  else if (m_pOriginalCategoryButton && m_defaultControl <= 0)
    m_defaultControl = CATEGORY_GROUP_ID;
}

void CGUIDialogSettingsBase::FreeControls()
{
  // clear the category group
  auto* control = dynamic_cast<CGUIControlGroupList*>(GetControl(CATEGORY_GROUP_ID));
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_categories.clear();

  // If we created our own edit control instead of borrowing it then clean it up
  if (m_newOriginalEdit)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = nullptr;
    m_newOriginalEdit = false;
  }

  FreeSettingsControls();
}

void CGUIDialogSettingsBase::DeleteControls()
{
  m_resetSetting.reset();
  m_dummyCategory.reset();
}

void CGUIDialogSettingsBase::FreeSettingsControls()
{
  // clear the settings group
  auto* control = dynamic_cast<CGUIControlGroupList*>(GetControl(SETTINGS_GROUP_ID));
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for (const auto& ctrl : m_settingControls)
    ctrl->Clear();

  m_settingControls.clear();
}

void CGUIDialogSettingsBase::OnTimeout()
{
  UpdateSettingControl(m_delayedSetting, true);
}

void CGUIDialogSettingsBase::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting || setting->GetType() == SettingType::Unknown ||
      setting->GetType() == SettingType::Action)
    return;

  UpdateSettingControl(setting->GetId(), true);
}

void CGUIDialogSettingsBase::OnSettingPropertyChanged(
    const std::shared_ptr<const CSetting>& setting, const char* propertyName)
{
  if (!setting || !propertyName)
    return;

  UpdateSettingControl(setting->GetId());
}

std::string CGUIDialogSettingsBase::GetLocalizedString(uint32_t labelId) const
{
  return g_localizeStrings.Get(labelId);
}

void CGUIDialogSettingsBase::SetupView()
{
  SetupControls();
}

SettingsContainer CGUIDialogSettingsBase::CreateSettings()
{
  FreeSettingsControls();

  SettingsContainer settingMap;

  if (m_categories.empty())
    return settingMap;

  if (m_iCategory < 0 || m_iCategory >= static_cast<int>(m_categories.size()))
    m_iCategory = 0;

  auto* group = dynamic_cast<CGUIControlGroupList*>(GetControl(SETTINGS_GROUP_ID));
  if (!group)
    return settingMap;

  SettingCategoryPtr category = m_categories.at(m_iCategory);
  if (!category)
    return settingMap;

  // set the description of the current category
  SetDescription(category->GetHelp());

  const SettingGroupList& groups = category->GetGroups((SettingLevel)GetSettingLevel());
  int iControlID = CONTROL_SETTINGS_START_CONTROL;
  bool first = true;
  for (const auto& grp : groups)
  {
    if (!group)
      continue;

    const SettingList& settings = grp->GetSettings((SettingLevel)GetSettingLevel());
    if (settings.empty())
      continue;

    const auto title = std::dynamic_pointer_cast<const CSettingControlTitle>(grp->GetControl());
    bool hideSeparator = title ? title->IsSeparatorHidden() : false;
    bool separatorBelowGroupLabel = title ? title->IsSeparatorBelowLabel() : false;
    int groupLabel = grp->GetLabel();

    // hide the separator for the first settings grouplist if it
    // is the very first item in the list (also above the label)
    if (first)
    {
      first = false;
      if (groupLabel <= 0)
        hideSeparator = true;
    }
    else if (!separatorBelowGroupLabel && !hideSeparator)
      AddSeparator(group->GetWidth(), iControlID);

    if (groupLabel > 0)
      AddGroupLabel(grp, group->GetWidth(), iControlID);

    if (separatorBelowGroupLabel && !hideSeparator)
      AddSeparator(group->GetWidth(), iControlID);

    for (const auto& setting : settings)
    {
      settingMap.insert(setting->GetId());
      AddSetting(setting, group->GetWidth(), iControlID);
    }
  }

  if (AllowResettingSettings() && !settingMap.empty())
  {
    // add "Reset" control
    AddSeparator(group->GetWidth(), iControlID);
    AddSetting(m_resetSetting, group->GetWidth(), iControlID);
  }

  // update our settings (turns controls on/off as appropriate)
  UpdateSettings();

  group->SetInvalid();

  return settingMap;
}

std::string CGUIDialogSettingsBase::GetSettingsLabel(const std::shared_ptr<ISetting>& pSetting)
{
  return GetLocalizedString(pSetting->GetLabel());
}

void CGUIDialogSettingsBase::UpdateSettings()
{
  for (const auto& settingControl : m_settingControls)
  {
    const std::shared_ptr<const CSetting> pSetting = settingControl->GetSetting();
    const CGUIControl* pControl = settingControl->GetControl();
    if (!pSetting || !pControl)
      continue;

    settingControl->UpdateFromSetting();
  }
}

CGUIControl* CGUIDialogSettingsBase::AddSetting(const std::shared_ptr<CSetting>& pSetting,
                                                float width,
                                                int& iControlID)
{
  if (!pSetting)
    return nullptr;

  BaseSettingControlPtr pSettingControl;
  CGUIControl* pControl = nullptr;

  // determine the label and any possible indentation in case of sub settings
  std::string label = GetSettingsLabel(pSetting);
  int parentLevels = 0;
  std::shared_ptr<CSetting> parentSetting = GetSetting(pSetting->GetParent());
  while (parentSetting)
  {
    parentLevels++;
    parentSetting = GetSetting(parentSetting->GetParent());
  }

  if (parentLevels > 0)
  {
    // add additional 2 spaces indentation for anything past one level
    std::string indentation;
    for (int index = 1; index < parentLevels; index++)
      indentation.append("  ");
    label = StringUtils::Format(g_localizeStrings.Get(168), indentation, label);
  }

  // create the proper controls
  if (!pSetting->GetControl())
    return nullptr;

  std::string controlType = pSetting->GetControl()->GetType();
  if (controlType == "toggle")
  {
    if (m_pOriginalRadioButton)
      pControl = m_pOriginalRadioButton->Clone();
    if (!pControl)
      return nullptr;

    static_cast<CGUIRadioButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlRadioButtonSetting>(
        static_cast<CGUIRadioButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "spinner")
  {
    if (m_pOriginalSpin)
      pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl)
      return nullptr;

    static_cast<CGUISpinControlEx*>(pControl)->SetText(label);
    pSettingControl = std::make_shared<CGUIControlSpinExSetting>(
        static_cast<CGUISpinControlEx*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "edit")
  {
    if (m_pOriginalEdit)
      pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (!pControl)
      return nullptr;

    static_cast<CGUIEditControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlEditSetting>(
        static_cast<CGUIEditControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "list")
  {
    if (m_pOriginalButton)
      pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl)
      return nullptr;

    static_cast<CGUIButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlListSetting>(
        static_cast<CGUIButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "button" || controlType == "slider")
  {
    if (controlType == "button" ||
        std::static_pointer_cast<const CSettingControlSlider>(pSetting->GetControl())->UsePopup())
    {
      if (m_pOriginalButton)
        pControl = new CGUIButtonControl(*m_pOriginalButton);
      if (!pControl)
        return nullptr;

      static_cast<CGUIButtonControl*>(pControl)->SetLabel(label);
      pSettingControl = std::make_shared<CGUIControlButtonSetting>(
          static_cast<CGUIButtonControl*>(pControl), iControlID, pSetting, this);
    }
    else
    {
      if (m_pOriginalSlider)
        pControl = m_pOriginalSlider->Clone();
      if (!pControl)
        return nullptr;

      static_cast<CGUISettingsSliderControl*>(pControl)->SetText(label);
      pSettingControl = std::make_shared<CGUIControlSliderSetting>(
          static_cast<CGUISettingsSliderControl*>(pControl), iControlID, pSetting, this);
    }
  }
  else if (controlType == "range")
  {
    if (m_pOriginalSlider)
      pControl = m_pOriginalSlider->Clone();
    if (!pControl)
      return nullptr;

    static_cast<CGUISettingsSliderControl*>(pControl)->SetText(label);
    pSettingControl = std::make_shared<CGUIControlRangeSetting>(
        static_cast<CGUISettingsSliderControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "label")
  {
    if (m_pOriginalButton)
      pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl)
      return nullptr;

    static_cast<CGUIButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlLabelSetting>(
        static_cast<CGUIButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "colorbutton")
  {
    if (m_pOriginalColorButton)
      pControl = m_pOriginalColorButton->Clone();
    if (!pControl)
      return nullptr;

    static_cast<CGUIColorButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlColorButtonSetting>(
        static_cast<CGUIColorButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else
    return nullptr;

  if (pSetting->GetControl()->GetDelayed())
    pSettingControl->SetDelayed();

  return AddSettingControl(pControl, pSettingControl, width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddSeparator(float width, int& iControlID)
{
  if (!m_pOriginalImage)
    return nullptr;

  CGUIControl* pControl = new CGUIImage(*m_pOriginalImage);
  if (!pControl)
    return nullptr;

  return AddSettingControl(pControl,
                           BaseSettingControlPtr(new CGUIControlSeparatorSetting(
                               static_cast<CGUIImage*>(pControl), iControlID, this)),
                           width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddGroupLabel(const std::shared_ptr<CSettingGroup>& group,
                                                   float width,
                                                   int& iControlID)
{
  if (!m_pOriginalGroupTitle)
    return nullptr;

  CGUIControl* pControl = new CGUILabelControl(*m_pOriginalGroupTitle);
  if (!pControl)
    return nullptr;

  static_cast<CGUILabelControl*>(pControl)->SetLabel(GetSettingsLabel(group));

  return AddSettingControl(pControl,
                           std::make_shared<CGUIControlGroupTitleSetting>(
                               static_cast<CGUILabelControl*>(pControl), iControlID, this),
                           width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddSettingControl(CGUIControl* pControl,
                                                       BaseSettingControlPtr pSettingControl,
                                                       float width,
                                                       int& iControlID)
{
  if (!pControl)
  {
    pSettingControl.reset();
    return nullptr;
  }

  pControl->SetID(iControlID);
  iControlID++;
  pControl->SetVisible(true);
  pControl->SetWidth(width);

  auto* group = dynamic_cast<CGUIControlGroupList*>(GetControl(SETTINGS_GROUP_ID));
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
  }
  m_settingControls.push_back(pSettingControl);

  return pControl;
}

void CGUIDialogSettingsBase::SetHeading(const CVariant& label)
{
  SetControlLabel(CONTROL_SETTINGS_LABEL, label);
}

void CGUIDialogSettingsBase::SetDescription(const CVariant& label)
{
  SetControlLabel(CONTROL_SETTINGS_DESCRIPTION, label);
}

void CGUIDialogSettingsBase::OnResetSettings()
{
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{10041}, CVariant{10042}))
  {
    for (const auto& settingControl : m_settingControls)
    {
      const std::shared_ptr<CSetting> setting = settingControl->GetSetting();
      if (setting)
        setting->Reset();
    }
  }
}

void CGUIDialogSettingsBase::OnClick(const BaseSettingControlPtr& pSettingControl)
{
  if (AllowResettingSettings() &&
      pSettingControl->GetSetting()->GetId() == SETTINGS_RESET_SETTING_ID)
  {
    OnAction(CAction(ACTION_SETTINGS_RESET));
    return;
  }

  // we need to first set the delayed setting and then execute OnClick()
  // because OnClick() triggers OnSettingChanged() and there we need to
  // know if the changed setting is delayed or not
  if (pSettingControl->IsDelayed())
  {
    m_delayedSetting = pSettingControl;
    // for some controls we need to update its displayed data/text before
    // OnClick() is called after the delay timer has expired because
    // otherwise the displayed value of the control does not match with
    // the user's interaction
    pSettingControl->UpdateFromControl();

    // either start or restart the delay timer which will result in a call to
    // the control's OnClick() method to update the setting's value
    if (m_delayedTimer.IsRunning())
      m_delayedTimer.Restart();
    else
      m_delayedTimer.Start(GetDelayMs());

    return;
  }

  // if changing the setting fails
  // we need to restore the proper state
  if (!pSettingControl->OnClick())
    pSettingControl->UpdateFromSetting();
}

void CGUIDialogSettingsBase::UpdateSettingControl(const std::string& settingId,
                                                  bool updateDisplayOnly /* = false */) const
{
  if (settingId.empty())
    return;

  return UpdateSettingControl(GetSettingControl(settingId), updateDisplayOnly);
}

void CGUIDialogSettingsBase::UpdateSettingControl(const BaseSettingControlPtr& pSettingControl,
                                                  bool updateDisplayOnly /* = false */) const
{
  if (!pSettingControl)
    return;

  // we send a thread message so that it's processed the following frame (some settings won't
  // like being changed during Render())
  // param2 = 1 for "only update the current value"
  CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), pSettingControl->GetID(), 0,
                      updateDisplayOnly ? 1 : 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message, GetID());
}

void CGUIDialogSettingsBase::SetControlLabel(int controlId, const CVariant& label)
{
  if (!GetControl(controlId))
    return;

  if (label.isString())
    SET_CONTROL_LABEL(controlId, label.asString());
  else if (label.isInteger() && label.asInteger() >= 0)
  {
    int labelId = static_cast<uint32_t>(label.asInteger());
    std::string localizedString = GetLocalizedString(labelId);
    if (!localizedString.empty())
      SET_CONTROL_LABEL(controlId, localizedString);
    else
      SET_CONTROL_LABEL(controlId, labelId);
  }
  else
    SET_CONTROL_LABEL(controlId, "");
}

BaseSettingControlPtr CGUIDialogSettingsBase::GetSettingControl(std::string_view strSetting) const
{
  for (const auto& settingControl : m_settingControls)
  {
    const std::shared_ptr<const CSetting> setting = settingControl->GetSetting();
    if (setting && setting->GetId() == strSetting)
      return settingControl;
  }

  return BaseSettingControlPtr();
}

BaseSettingControlPtr CGUIDialogSettingsBase::GetSettingControl(int controlId)
{
  if (controlId < CONTROL_SETTINGS_START_CONTROL ||
      controlId >= static_cast<int>(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
    return BaseSettingControlPtr();

  return m_settingControls[controlId - CONTROL_SETTINGS_START_CONTROL];
}
