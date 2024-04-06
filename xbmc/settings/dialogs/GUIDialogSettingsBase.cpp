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

#define CATEGORY_GROUP_ID 3
#define SETTINGS_GROUP_ID 5

#define CONTROL_DEFAULT_BUTTON 7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN 9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR 11
#define CONTROL_DEFAULT_EDIT 12
#define CONTROL_DEFAULT_SLIDER 13
#define CONTROL_DEFAULT_SETTING_LABEL 14
#define CONTROL_DEFAULT_COLORBUTTON 15

CGUIDialogSettingsBase::CGUIDialogSettingsBase(int windowId, const std::string& xmlFile)
  : CGUIDialog(windowId, xmlFile),
    m_resetSetting(NULL),
    m_dummyCategory(NULL),
    m_pOriginalSpin(NULL),
    m_pOriginalSlider(NULL),
    m_pOriginalRadioButton(NULL),
    m_pOriginalCategoryButton(NULL),
    m_pOriginalButton(NULL),
    m_pOriginalEdit(NULL),
    m_pOriginalImage(NULL),
    m_pOriginalGroupTitle(NULL),
    m_delayedTimer(this)
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
      if (m_delayedSetting != NULL)
      {
        m_delayedTimer.Stop();
        CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID());
        OnMessage(message);
      }

      CGUIDialog::OnMessage(message);
      FreeControls();
      return true;
    }

    case GUI_MSG_FOCUSED:
    {
      CGUIDialog::OnMessage(message);
      m_focusedControl = GetFocusedControlID();

      // cancel any delayed changes
      if (m_delayedSetting != NULL && m_delayedSetting->GetID() != m_focusedControl)
      {
        m_delayedTimer.Stop();
        // param1 = 1 for "reset the control if it's invalid"
        CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID(), 1);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message, GetID());
      }
      // update the value of the previous setting (in case it was invalid)
      else if (m_iSetting >= CONTROL_SETTINGS_START_CONTROL &&
               m_iSetting < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr control = GetSettingControl(m_iSetting);
        if (control != NULL && control->GetSetting() != NULL && !control->IsValid())
        {
          // param1 = 1 for "reset the control if it's invalid"
          // param2 = 1 for "only update the current value"
          CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_iSetting, 1, 1);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message, GetID());
        }
      }

      CVariant description;

      // check if we have changed the category and need to create new setting controls
      if (m_focusedControl >= CONTROL_SETTINGS_START_BUTTONS &&
          m_focusedControl < (int)(CONTROL_SETTINGS_START_BUTTONS + m_categories.size()))
      {
        int categoryIndex = m_focusedControl - CONTROL_SETTINGS_START_BUTTONS;
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
      else if (m_focusedControl >= CONTROL_SETTINGS_START_CONTROL &&
               m_focusedControl < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        m_iSetting = m_focusedControl;
        std::shared_ptr<CSetting> setting = GetSettingControl(m_focusedControl)->GetSetting();
        if (setting != NULL)
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
      if (control != NULL)
        OnClick(control);

      break;
    }

    case GUI_MSG_UPDATE_ITEM:
    {
      if (m_delayedSetting != NULL && m_delayedSetting->GetID() == message.GetControlId())
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
          message.GetControlId() < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr settingControl = GetSettingControl(message.GetControlId());
        if (settingControl.get() != NULL && settingControl->GetSetting() != NULL)
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
          m_iSetting < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        auto settingControl = GetSettingControl(m_iSetting);
        if (settingControl != nullptr)
        {
          std::shared_ptr<CSetting> setting = settingControl->GetSetting();
          if (setting != nullptr)
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
    if (m_fadedControl &&
        (m_fadedControl != control->GetID() || m_fadedControl == m_focusedControl))
    {
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
        static_cast<CGUIButtonControl*>(control)->SetAlpha(0xFF);
      else
        static_cast<CGUIButtonControl*>(control)->SetSelected(false);
      m_fadedControl = 0;
    }

    if (!control->HasFocus())
    {
      m_fadedControl = control->GetID();
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
        static_cast<CGUIButtonControl*>(control)->SetAlpha(0x80);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
        static_cast<CGUIButtonControl*>(control)->SetSelected(true);
      else
        m_fadedControl = 0;

      if (m_fadedControl)
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
  if (m_pOriginalEdit == nullptr && m_pOriginalButton != nullptr)
  {
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
    m_newOriginalEdit = true;
  }

  // hide all default controls by default
  if (m_pOriginalSpin != nullptr)
    m_pOriginalSpin->SetVisible(false);
  if (m_pOriginalSlider != nullptr)
    m_pOriginalSlider->SetVisible(false);
  if (m_pOriginalRadioButton != nullptr)
    m_pOriginalRadioButton->SetVisible(false);
  if (m_pOriginalButton != nullptr)
    m_pOriginalButton->SetVisible(false);
  if (m_pOriginalCategoryButton != nullptr)
    m_pOriginalCategoryButton->SetVisible(false);
  if (m_pOriginalEdit != nullptr)
    m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage != nullptr)
    m_pOriginalImage->SetVisible(false);
  if (m_pOriginalGroupTitle != nullptr)
    m_pOriginalGroupTitle->SetVisible(false);
  if (m_pOriginalColorButton != nullptr)
    m_pOriginalColorButton->SetVisible(false);

  // get the section
  SettingSectionPtr section = GetSection();
  if (section == NULL)
    return;

  // update the screen string
  if (section->GetLabel() >= 0)
    SetHeading(section->GetLabel());

  // get the categories we need
  m_categories = section->GetCategories((SettingLevel)GetSettingLevel());
  if (m_categories.empty())
    m_categories.push_back(m_dummyCategory);

  if (m_pOriginalCategoryButton != NULL)
  {
    // setup our control groups...
    CGUIControlGroupList* group =
        dynamic_cast<CGUIControlGroupList*>(GetControl(CATEGORY_GROUP_ID));
    if (!group)
      return;

    // go through the categories and create the necessary buttons
    int buttonIdOffset = 0;
    for (SettingCategoryList::const_iterator category = m_categories.begin();
         category != m_categories.end(); ++category)
    {
      CGUIButtonControl* pButton = NULL;
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
  if (m_pOriginalCategoryButton == NULL &&
      (m_defaultControl <= 0 || m_defaultControl == CATEGORY_GROUP_ID))
    m_defaultControl = SETTINGS_GROUP_ID;
  else if (m_pOriginalCategoryButton != NULL && m_defaultControl <= 0)
    m_defaultControl = CATEGORY_GROUP_ID;
}

void CGUIDialogSettingsBase::FreeControls()
{
  // clear the category group
  CGUIControlGroupList* control =
      dynamic_cast<CGUIControlGroupList*>(GetControl(CATEGORY_GROUP_ID));
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
  CGUIControlGroupList* control =
      dynamic_cast<CGUIControlGroupList*>(GetControl(SETTINGS_GROUP_ID));
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for (std::vector<BaseSettingControlPtr>::iterator control = m_settingControls.begin();
       control != m_settingControls.end(); ++control)
    (*control)->Clear();

  m_settingControls.clear();
}

void CGUIDialogSettingsBase::OnTimeout()
{
  UpdateSettingControl(m_delayedSetting, true);
}

void CGUIDialogSettingsBase::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL || setting->GetType() == SettingType::Unknown ||
      setting->GetType() == SettingType::Action)
    return;

  UpdateSettingControl(setting->GetId(), true);
}

void CGUIDialogSettingsBase::OnSettingPropertyChanged(
    const std::shared_ptr<const CSetting>& setting, const char* propertyName)
{
  if (setting == NULL || propertyName == NULL)
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

std::set<std::string> CGUIDialogSettingsBase::CreateSettings()
{
  FreeSettingsControls();

  std::set<std::string> settingMap;

  if (m_categories.size() <= 0)
    return settingMap;

  if (m_iCategory < 0 || m_iCategory >= (int)m_categories.size())
    m_iCategory = 0;

  CGUIControlGroupList* group = dynamic_cast<CGUIControlGroupList*>(GetControl(SETTINGS_GROUP_ID));
  if (group == NULL)
    return settingMap;

  SettingCategoryPtr category = m_categories.at(m_iCategory);
  if (category == NULL)
    return settingMap;

  // set the description of the current category
  SetDescription(category->GetHelp());

  const SettingGroupList& groups = category->GetGroups((SettingLevel)GetSettingLevel());
  int iControlID = CONTROL_SETTINGS_START_CONTROL;
  bool first = true;
  for (SettingGroupList::const_iterator groupIt = groups.begin(); groupIt != groups.end();
       ++groupIt)
  {
    if (*groupIt == NULL)
      continue;

    const SettingList& settings = (*groupIt)->GetSettings((SettingLevel)GetSettingLevel());
    if (settings.size() <= 0)
      continue;

    std::shared_ptr<const CSettingControlTitle> title =
        std::dynamic_pointer_cast<const CSettingControlTitle>((*groupIt)->GetControl());
    bool hideSeparator = title ? title->IsSeparatorHidden() : false;
    bool separatorBelowGroupLabel = title ? title->IsSeparatorBelowLabel() : false;
    int groupLabel = (*groupIt)->GetLabel();

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
      AddGroupLabel(*groupIt, group->GetWidth(), iControlID);

    if (separatorBelowGroupLabel && !hideSeparator)
      AddSeparator(group->GetWidth(), iControlID);

    for (SettingList::const_iterator settingIt = settings.begin(); settingIt != settings.end();
         ++settingIt)
    {
      const std::shared_ptr<CSetting>& pSetting = *settingIt;
      settingMap.insert(pSetting->GetId());
      AddSetting(pSetting, group->GetWidth(), iControlID);
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
  for (std::vector<BaseSettingControlPtr>::iterator it = m_settingControls.begin();
       it != m_settingControls.end(); ++it)
  {
    BaseSettingControlPtr pSettingControl = *it;
    std::shared_ptr<CSetting> pSetting = pSettingControl->GetSetting();
    CGUIControl* pControl = pSettingControl->GetControl();
    if (pSetting == NULL || pControl == NULL)
      continue;

    pSettingControl->UpdateFromSetting();
  }
}

CGUIControl* CGUIDialogSettingsBase::AddSetting(const std::shared_ptr<CSetting>& pSetting,
                                                float width,
                                                int& iControlID)
{
  if (pSetting == NULL)
    return NULL;

  BaseSettingControlPtr pSettingControl;
  CGUIControl* pControl = NULL;

  // determine the label and any possible indentation in case of sub settings
  std::string label = GetSettingsLabel(pSetting);
  int parentLevels = 0;
  std::shared_ptr<CSetting> parentSetting = GetSetting(pSetting->GetParent());
  while (parentSetting != NULL)
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
    return NULL;

  std::string controlType = pSetting->GetControl()->GetType();
  if (controlType == "toggle")
  {
    if (m_pOriginalRadioButton != NULL)
      pControl = m_pOriginalRadioButton->Clone();
    if (pControl == NULL)
      return NULL;

    static_cast<CGUIRadioButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlRadioButtonSetting>(
        static_cast<CGUIRadioButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "spinner")
  {
    if (m_pOriginalSpin != NULL)
      pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (pControl == NULL)
      return NULL;

    static_cast<CGUISpinControlEx*>(pControl)->SetText(label);
    pSettingControl = std::make_shared<CGUIControlSpinExSetting>(
        static_cast<CGUISpinControlEx*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "edit")
  {
    if (m_pOriginalEdit != NULL)
      pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (pControl == NULL)
      return NULL;

    static_cast<CGUIEditControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlEditSetting>(
        static_cast<CGUIEditControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "list")
  {
    if (m_pOriginalButton != NULL)
      pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (pControl == NULL)
      return NULL;

    static_cast<CGUIButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlListSetting>(
        static_cast<CGUIButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "button" || controlType == "slider")
  {
    if (controlType == "button" ||
        std::static_pointer_cast<const CSettingControlSlider>(pSetting->GetControl())->UsePopup())
    {
      if (m_pOriginalButton != NULL)
        pControl = new CGUIButtonControl(*m_pOriginalButton);
      if (pControl == NULL)
        return NULL;

      static_cast<CGUIButtonControl*>(pControl)->SetLabel(label);
      pSettingControl = std::make_shared<CGUIControlButtonSetting>(
          static_cast<CGUIButtonControl*>(pControl), iControlID, pSetting, this);
    }
    else
    {
      if (m_pOriginalSlider != NULL)
        pControl = m_pOriginalSlider->Clone();
      if (pControl == NULL)
        return NULL;

      static_cast<CGUISettingsSliderControl*>(pControl)->SetText(label);
      pSettingControl = std::make_shared<CGUIControlSliderSetting>(
          static_cast<CGUISettingsSliderControl*>(pControl), iControlID, pSetting, this);
    }
  }
  else if (controlType == "range")
  {
    if (m_pOriginalSlider != NULL)
      pControl = m_pOriginalSlider->Clone();
    if (pControl == NULL)
      return NULL;

    static_cast<CGUISettingsSliderControl*>(pControl)->SetText(label);
    pSettingControl = std::make_shared<CGUIControlRangeSetting>(
        static_cast<CGUISettingsSliderControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "label")
  {
    if (m_pOriginalButton != NULL)
      pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (pControl == NULL)
      return NULL;

    static_cast<CGUIButtonControl*>(pControl)->SetLabel(label);
    pSettingControl = std::make_shared<CGUIControlLabelSetting>(
        static_cast<CGUIButtonControl*>(pControl), iControlID, pSetting, this);
  }
  else if (controlType == "colorbutton")
  {
    if (m_pOriginalColorButton)
      pControl = m_pOriginalColorButton->Clone();
    if (pControl == nullptr)
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
  if (m_pOriginalImage == NULL)
    return NULL;

  CGUIControl* pControl = new CGUIImage(*m_pOriginalImage);
  if (pControl == NULL)
    return NULL;

  return AddSettingControl(pControl,
                           BaseSettingControlPtr(new CGUIControlSeparatorSetting(
                               static_cast<CGUIImage*>(pControl), iControlID, this)),
                           width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddGroupLabel(const std::shared_ptr<CSettingGroup>& group,
                                                   float width,
                                                   int& iControlID)
{
  if (m_pOriginalGroupTitle == NULL)
    return NULL;

  CGUIControl* pControl = new CGUILabelControl(*m_pOriginalGroupTitle);
  if (pControl == NULL)
    return NULL;

  static_cast<CGUILabelControl*>(pControl)->SetLabel(GetSettingsLabel(group));

  return AddSettingControl(pControl,
                           BaseSettingControlPtr(new CGUIControlGroupTitleSetting(
                               static_cast<CGUILabelControl*>(pControl), iControlID, this)),
                           width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddSettingControl(CGUIControl* pControl,
                                                       BaseSettingControlPtr pSettingControl,
                                                       float width,
                                                       int& iControlID)
{
  if (pControl == NULL)
  {
    pSettingControl.reset();
    return NULL;
  }

  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  pControl->SetWidth(width);

  CGUIControlGroupList* group = dynamic_cast<CGUIControlGroupList*>(GetControl(SETTINGS_GROUP_ID));
  if (group != NULL)
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
    for (std::vector<BaseSettingControlPtr>::iterator it = m_settingControls.begin();
         it != m_settingControls.end(); ++it)
    {
      std::shared_ptr<CSetting> setting = (*it)->GetSetting();
      if (setting != NULL)
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
                                                  bool updateDisplayOnly /* = false */)
{
  if (settingId.empty())
    return;

  return UpdateSettingControl(GetSettingControl(settingId), updateDisplayOnly);
}

void CGUIDialogSettingsBase::UpdateSettingControl(const BaseSettingControlPtr& pSettingControl,
                                                  bool updateDisplayOnly /* = false */)
{
  if (pSettingControl == NULL)
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
  if (GetControl(controlId) == NULL)
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

BaseSettingControlPtr CGUIDialogSettingsBase::GetSettingControl(const std::string& strSetting)
{
  for (std::vector<BaseSettingControlPtr>::iterator control = m_settingControls.begin();
       control != m_settingControls.end(); ++control)
  {
    if ((*control)->GetSetting() != NULL && (*control)->GetSetting()->GetId() == strSetting)
      return *control;
  }

  return BaseSettingControlPtr();
}

BaseSettingControlPtr CGUIDialogSettingsBase::GetSettingControl(int controlId)
{
  if (controlId < CONTROL_SETTINGS_START_CONTROL ||
      controlId >= (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
    return BaseSettingControlPtr();

  return m_settingControls[controlId - CONTROL_SETTINGS_START_CONTROL];
}
