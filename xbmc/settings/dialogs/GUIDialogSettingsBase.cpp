/*
 *      Copyright (C) 2014 Team XBMC
 *      http://www.xbmc.org
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

#include "GUIDialogSettingsBase.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIToggleButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "settings/SettingControl.h"
#include "settings/lib/SettingSection.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"

using namespace std;

#if defined(TARGET_WINDOWS) // disable 4355: 'this' used in base member initializer list
#pragma warning(push)
#pragma warning(disable: 4355)
#endif // defined(TARGET_WINDOWS)

#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5

#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_DEFAULT_EDIT            12
#define CONTROL_DEFAULT_SLIDER          13
#define CONTROL_DEFAULT_SETTING_LABEL   14

CGUIDialogSettingsBase::CGUIDialogSettingsBase(int windowId, const std::string &xmlFile)
    : CGUIDialog(windowId, xmlFile),
      m_iSetting(0), m_iCategory(0),
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
      m_newOriginalEdit(false),
      m_delayedTimer(this),
      m_confirmed(false)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSettingsBase::~CGUIDialogSettingsBase()
{
  FreeControls();
  DeleteControls();
}

bool CGUIDialogSettingsBase::OnMessage(CGUIMessage &message)
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
        m_resetSetting = new CSettingAction(SETTINGS_RESET_SETTING_ID);
        m_resetSetting->SetLabel(10041);
        m_resetSetting->SetHelp(10045);
        m_resetSetting->SetControl(CreateControl("button"));
      }

      m_dummyCategory = new CSettingCategory(SETTINGS_EMPTY_CATEGORY_ID);
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
      int focusedControl = GetFocusedControlID();

      // cancel any delayed changes
      if (m_delayedSetting != NULL && m_delayedSetting->GetID() != focusedControl)
      {
        m_delayedTimer.Stop();
        CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID(), 1); // param1 = 1 for "reset the control if it's invalid"
        g_windowManager.SendThreadMessage(message, GetID());
      }
      // update the value of the previous setting (in case it was invalid)
      else if (m_iSetting >= CONTROL_SETTINGS_START_CONTROL && m_iSetting < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr control = GetSettingControl(m_iSetting);
        if (control != NULL && control->GetSetting() != NULL && !control->IsValid())
        {
          CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_iSetting, 1); // param1 = 1 for "reset the control if it's invalid"
          g_windowManager.SendThreadMessage(message, GetID());
        }
      }

      CVariant description;

      // check if we have changed the category and need to create new setting controls
      if (focusedControl >= CONTROL_SETTINGS_START_BUTTONS && focusedControl < (int)(CONTROL_SETTINGS_START_BUTTONS + m_categories.size()))
      {
        int categoryIndex = focusedControl - CONTROL_SETTINGS_START_BUTTONS;
        const CSettingCategory* category = m_categories.at(categoryIndex);
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
      else if (focusedControl >= CONTROL_SETTINGS_START_CONTROL && focusedControl < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        m_iSetting = focusedControl;
        CSetting *setting = GetSettingControl(focusedControl)->GetSetting();
        if (setting != NULL)
          description = setting->GetHelp();
      }

      // set the description of the currently focused category/setting
      if (description.isInteger() ||
          (description.isString() && !description.empty()))
        SetDescription(description);

      return true;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_SETTINGS_OKAY_BUTTON)
      {
        OnOkay();
        Close();
        return true;
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

      if (message.GetControlId() >= CONTROL_SETTINGS_START_CONTROL && message.GetControlId() < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr settingControl = GetSettingControl(message.GetControlId());
        if (settingControl.get() != NULL && settingControl->GetSetting() != NULL)
        {
          settingControl->Update();
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

bool CGUIDialogSettingsBase::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_SETTINGS_RESET:
    {
      OnResetSettings();
      return true;
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

void CGUIDialogSettingsBase::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_SETTINGS_START_BUTTONS + m_iCategory);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      bAlphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      bAlphaFaded = true;
    }
  }
  CGUIDialog::DoProcess(currentTime, dirtyregions);
  if (control && bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
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

  // get the section
  CSettingSection *section = GetSection();
  if (section == NULL)
    return;
  
  // update the screen string
  SetHeading(section->GetLabel());

  // get the categories we need
  m_categories = section->GetCategories((SettingLevel)GetSettingLevel());
  if (m_categories.empty())
    m_categories.push_back(m_dummyCategory);

  // get all controls
  m_pOriginalSpin = dynamic_cast<CGUISpinControlEx*>(GetControl(CONTROL_DEFAULT_SPIN));
  m_pOriginalSlider = dynamic_cast<CGUISettingsSliderControl*>(GetControl(CONTROL_DEFAULT_SLIDER));
  m_pOriginalRadioButton = dynamic_cast<CGUIRadioButtonControl *>(GetControl(CONTROL_DEFAULT_RADIOBUTTON));
  m_pOriginalCategoryButton = dynamic_cast<CGUIButtonControl *>(GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON));
  m_pOriginalButton = dynamic_cast<CGUIButtonControl *>(GetControl(CONTROL_DEFAULT_BUTTON));
  m_pOriginalImage = dynamic_cast<CGUIImage *>(GetControl(CONTROL_DEFAULT_SEPARATOR));
  m_pOriginalEdit = dynamic_cast<CGUIEditControl *>(GetControl(CONTROL_DEFAULT_EDIT));
  m_pOriginalGroupTitle = dynamic_cast<CGUILabelControl *>(GetControl(CONTROL_DEFAULT_SETTING_LABEL));

  if (!m_pOriginalEdit && m_pOriginalButton)
  {
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
    m_newOriginalEdit = true;
  }

  if (m_pOriginalSpin) m_pOriginalSpin->SetVisible(false);
  if (m_pOriginalSlider) m_pOriginalSlider->SetVisible(false);
  if (m_pOriginalRadioButton) m_pOriginalRadioButton->SetVisible(false);
  if (m_pOriginalButton) m_pOriginalButton->SetVisible(false);
  if (m_pOriginalCategoryButton) m_pOriginalCategoryButton->SetVisible(false);
  if (m_pOriginalEdit) m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  if (m_pOriginalGroupTitle) m_pOriginalGroupTitle->SetVisible(false);

  if (m_pOriginalCategoryButton != NULL)
  {
    // setup our control groups...
    CGUIControlGroupList *group = dynamic_cast<CGUIControlGroupList *>(GetControl(CATEGORY_GROUP_ID));
    if (!group)
      return;

    // go through the categories and create the necessary buttons
    int buttonIdOffset = 0;
    for (SettingCategoryList::const_iterator category = m_categories.begin(); category != m_categories.end(); ++category)
    {
      CGUIButtonControl *pButton = NULL;
      if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
        pButton = new CGUIToggleButtonControl(*(CGUIToggleButtonControl *)m_pOriginalCategoryButton);
      else
        pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
      pButton->SetLabel(GetLocalizedString((*category)->GetLabel()));
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
  CGUIControlGroupList *control = dynamic_cast<CGUIControlGroupList *>(GetControl(CATEGORY_GROUP_ID));
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_categories.clear();
  FreeSettingsControls();
}

void CGUIDialogSettingsBase::DeleteControls()
{
  if (m_newOriginalEdit)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = NULL;
  }

  delete m_resetSetting;
  m_resetSetting = NULL;
  delete m_dummyCategory;
  m_dummyCategory = NULL;
}

void CGUIDialogSettingsBase::FreeSettingsControls()
{
  // clear the settings group
  CGUIControlGroupList *control = dynamic_cast<CGUIControlGroupList *>(GetControl(SETTINGS_GROUP_ID));
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for (std::vector<BaseSettingControlPtr>::iterator control = m_settingControls.begin(); control != m_settingControls.end(); ++control)
    (*control)->Clear();

  m_settingControls.clear();
}

void CGUIDialogSettingsBase::OnTimeout()
{
  UpdateSettingControl(m_delayedSetting);
}

void CGUIDialogSettingsBase::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL || setting->GetType() == SettingTypeNone ||
      setting->GetType() == SettingTypeAction)
    return;

  UpdateSettingControl(setting->GetId());
}

void CGUIDialogSettingsBase::OnSettingPropertyChanged(const CSetting *setting, const char *propertyName)
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

  CGUIControlGroupList *group = dynamic_cast<CGUIControlGroupList *>(GetControl(SETTINGS_GROUP_ID));
  if (group == NULL)
    return settingMap;

  const CSettingCategory* category = m_categories.at(m_iCategory);
  if (category == NULL)
    return settingMap;

  // set the description of the current category
  SetDescription(category->GetHelp());

  const SettingGroupList& groups = category->GetGroups((SettingLevel)GetSettingLevel());
  int iControlID = CONTROL_SETTINGS_START_CONTROL;
  bool first = true;
  for (SettingGroupList::const_iterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt)
  {
    if (*groupIt == NULL)
      continue;

    const SettingList& settings = (*groupIt)->GetSettings((SettingLevel)GetSettingLevel());
    if (settings.size() <= 0)
      continue;

    const CSettingControlTitle *title = dynamic_cast<CSettingControlTitle *>((*groupIt)->GetControl());
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
      AddLabel(group->GetWidth(), iControlID, groupLabel);

    if (separatorBelowGroupLabel && !hideSeparator)
      AddSeparator(group->GetWidth(), iControlID);

    for (SettingList::const_iterator settingIt = settings.begin(); settingIt != settings.end(); ++settingIt)
    {
      CSetting *pSetting = *settingIt;
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

  return settingMap;
}

std::string CGUIDialogSettingsBase::GetSettingsLabel(CSetting *pSetting)
{
  return GetLocalizedString(pSetting->GetLabel());
}

void CGUIDialogSettingsBase::UpdateSettings()
{
  for (vector<BaseSettingControlPtr>::iterator it = m_settingControls.begin(); it != m_settingControls.end(); ++it)
  {
    BaseSettingControlPtr pSettingControl = *it;
    CSetting *pSetting = pSettingControl->GetSetting();
    CGUIControl *pControl = pSettingControl->GetControl();
    if (pSetting == NULL || pControl == NULL)
      continue;

    pSettingControl->Update();
  }
}

CGUIControl* CGUIDialogSettingsBase::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  if (pSetting == NULL)
    return NULL;

  BaseSettingControlPtr pSettingControl;
  CGUIControl *pControl = NULL;

  // determine the label and any possible indentation in case of sub settings
  std::string label = GetSettingsLabel(pSetting);
  int parentLevels = 0;
  CSetting *parentSetting = GetSetting(pSetting->GetParent());
  while (parentSetting != NULL)
  {
    parentLevels++;
    parentSetting = GetSetting(parentSetting->GetParent());
  }

  if (parentLevels > 0)
  {
    // add additional 2 spaces indentation for anything past one level
    string indentation;
    for (int index = 1; index < parentLevels; index++)
      indentation.append("  ");
    label = StringUtils::Format(g_localizeStrings.Get(168).c_str(), indentation.c_str(), label.c_str());
  }

  // create the proper controls
  if (!pSetting->GetControl())
    return NULL;

  std::string controlType = pSetting->GetControl()->GetType();
  if (controlType == "toggle")
  {
    if (m_pOriginalRadioButton != NULL)
      pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (pControl == NULL)
      return NULL;

    ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
    pSettingControl.reset(new CGUIControlRadioButtonSetting((CGUIRadioButtonControl *)pControl, iControlID, pSetting));
  }
  else if (controlType == "spinner")
  {
    if (m_pOriginalSpin != NULL)
      pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (pControl == NULL)
      return NULL;

    ((CGUISpinControlEx *)pControl)->SetText(label);
    pSettingControl.reset(new CGUIControlSpinExSetting((CGUISpinControlEx *)pControl, iControlID, pSetting));
  }
  else if (controlType == "edit")
  {
    if (m_pOriginalEdit != NULL)
      pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (pControl == NULL)
      return NULL;
      
    ((CGUIEditControl *)pControl)->SetLabel(label);
    pSettingControl.reset(new CGUIControlEditSetting((CGUIEditControl *)pControl, iControlID, pSetting));
  }
  else if (controlType == "list")
  {
    if (m_pOriginalButton != NULL)
      pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (pControl == NULL)
      return NULL;

    ((CGUIButtonControl *)pControl)->SetLabel(label);
    pSettingControl.reset(new CGUIControlListSetting((CGUIButtonControl *)pControl, iControlID, pSetting));
  }
  else if (controlType == "button" || controlType == "slider")
  {
    if (controlType == "button" ||
        static_cast<const CSettingControlSlider*>(pSetting->GetControl())->UsePopup())
    {
      if (m_pOriginalButton != NULL)
        pControl = new CGUIButtonControl(*m_pOriginalButton);
      if (pControl == NULL)
        return NULL;
      
      ((CGUIButtonControl *)pControl)->SetLabel(label);
      pSettingControl.reset(new CGUIControlButtonSetting((CGUIButtonControl *)pControl, iControlID, pSetting));
    }
    else
    {
      if (m_pOriginalSlider != NULL)
        pControl = new CGUISettingsSliderControl(*m_pOriginalSlider);
      if (pControl == NULL)
        return NULL;
      
      ((CGUISettingsSliderControl *)pControl)->SetText(label);
      pSettingControl.reset(new CGUIControlSliderSetting((CGUISettingsSliderControl *)pControl, iControlID, pSetting));
    }
  }
  else if (controlType == "range")
  {
    if (m_pOriginalSlider != NULL)
      pControl = new CGUISettingsSliderControl(*m_pOriginalSlider);
    if (pControl == NULL)
      return NULL;

    ((CGUISettingsSliderControl *)pControl)->SetText(label);
    pSettingControl.reset(new CGUIControlRangeSetting((CGUISettingsSliderControl *)pControl, iControlID, pSetting));
  }
  else
    return NULL;

  if (pSetting->GetControl()->GetDelayed())
    pSettingControl->SetDelayed();

  return AddSettingControl(pControl, pSettingControl, width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddSeparator(float width, int &iControlID)
{
  if (m_pOriginalImage == NULL)
    return NULL;

  CGUIControl *pControl = new CGUIImage(*m_pOriginalImage);
  if (pControl == NULL)
    return NULL;

  return AddSettingControl(pControl, BaseSettingControlPtr(new CGUIControlSeparatorSetting((CGUIImage *)pControl, iControlID)), width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddLabel(float width, int &iControlID, int label)
{
  if (m_pOriginalGroupTitle == NULL)
    return NULL;

  CGUIControl *pControl = new CGUILabelControl(*m_pOriginalGroupTitle);
  if (pControl == NULL)
    return NULL;

  ((CGUILabelControl *)pControl)->SetLabel(GetLocalizedString(label));

  return AddSettingControl(pControl, BaseSettingControlPtr(new CGUIControlGroupTitleSetting((CGUILabelControl *)pControl, iControlID)), width, iControlID);
}

CGUIControl* CGUIDialogSettingsBase::AddSettingControl(CGUIControl *pControl, BaseSettingControlPtr pSettingControl, float width, int &iControlID)
{
  if (pControl == NULL)
  {
    pSettingControl.reset();
    return NULL;
  }
  
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  pControl->SetWidth(width);
  
  CGUIControlGroupList *group = dynamic_cast<CGUIControlGroupList *>(GetControl(SETTINGS_GROUP_ID));
  if (group != NULL)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
  }
  m_settingControls.push_back(pSettingControl);
  
  return pControl;
}

void CGUIDialogSettingsBase::SetHeading(const CVariant &label)
{
  SetControlLabel(CONTROL_SETTINGS_LABEL, label);
}

void CGUIDialogSettingsBase::SetDescription(const CVariant &label)
{
  SetControlLabel(CONTROL_SETTINGS_DESCRIPTION, label);
}

void CGUIDialogSettingsBase::OnResetSettings()
{
  if (CGUIDialogYesNo::ShowAndGetInput(10041, 10042))
  {
    for(vector<BaseSettingControlPtr>::iterator it = m_settingControls.begin(); it != m_settingControls.end(); ++it)
    {
      CSetting *setting = (*it)->GetSetting();
      if (setting != NULL)
        setting->Reset();
    }
  }
}

void CGUIDialogSettingsBase::OnClick(BaseSettingControlPtr pSettingControl)
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
    pSettingControl->Update(true);

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
    pSettingControl->Update();
}

void CGUIDialogSettingsBase::UpdateSettingControl(const std::string &settingId)
{
  if (settingId.empty())
    return;

  return UpdateSettingControl(GetSettingControl(settingId));
}

void CGUIDialogSettingsBase::UpdateSettingControl(BaseSettingControlPtr pSettingControl)
{
  if (pSettingControl == NULL)
    return;

  // we send a thread message so that it's processed the following frame (some settings won't
  // like being changed during Render())
  CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), pSettingControl->GetID());
  g_windowManager.SendThreadMessage(message, GetID());
}

void CGUIDialogSettingsBase::SetControlLabel(int controlId, const CVariant &label)
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

BaseSettingControlPtr CGUIDialogSettingsBase::GetSettingControl(const std::string &strSetting)
{
  for (vector<BaseSettingControlPtr>::iterator control = m_settingControls.begin(); control != m_settingControls.end(); ++control)
  {
    if ((*control)->GetSetting() != NULL && (*control)->GetSetting()->GetId() == strSetting)
      return *control;
  }

  return BaseSettingControlPtr();
}

BaseSettingControlPtr CGUIDialogSettingsBase::GetSettingControl(int controlId)
{
  if (controlId < CONTROL_SETTINGS_START_CONTROL || controlId >= (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
    return BaseSettingControlPtr();

  return m_settingControls[controlId - CONTROL_SETTINGS_START_CONTROL];
}
