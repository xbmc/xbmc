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

#include "GUIWindowSettingsCategory.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIToggleButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "view/ViewStateSettings.h"
#include "GUIPassword.h"

using namespace std;

#if defined(TARGET_WINDOWS) // disable 4355: 'this' used in base member initializer list
#pragma warning(push)
#pragma warning(disable: 4355)
#endif // defined(TARGET_WINDOWS)

#define SETTINGS_PICTURES               WINDOW_SETTINGS_MYPICTURES - WINDOW_SETTINGS_START
#define SETTINGS_PROGRAMS               WINDOW_SETTINGS_MYPROGRAMS - WINDOW_SETTINGS_START
#define SETTINGS_WEATHER                WINDOW_SETTINGS_MYWEATHER - WINDOW_SETTINGS_START
#define SETTINGS_MUSIC                  WINDOW_SETTINGS_MYMUSIC - WINDOW_SETTINGS_START
#define SETTINGS_SYSTEM                 WINDOW_SETTINGS_SYSTEM - WINDOW_SETTINGS_START
#define SETTINGS_VIDEOS                 WINDOW_SETTINGS_MYVIDEOS - WINDOW_SETTINGS_START
#define SETTINGS_SERVICE                WINDOW_SETTINGS_SERVICE - WINDOW_SETTINGS_START
#define SETTINGS_APPEARANCE             WINDOW_SETTINGS_APPEARANCE - WINDOW_SETTINGS_START
#define SETTINGS_PVR                    WINDOW_SETTINGS_MYPVR - WINDOW_SETTINGS_START
#define SETTINGS_GAMES                  WINDOW_SETTINGS_MYGAMES - WINDOW_SETTINGS_START

#define SETTING_DELAY                   1500

#define CONTROL_SETTINGS_LABEL          2
#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5
#define CONTROL_DESCRIPTION             6
#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_DEFAULT_EDIT            12
#define CONTROL_START_BUTTONS           -100
#define CONTROL_START_CONTROL           -80
#define CONTRL_BTN_LEVELS               20

typedef struct {
  int id;
  string name;
} SettingGroup;

static const SettingGroup s_settingGroupMap[] = { { SETTINGS_PICTURES,    "pictures" },
                                                  { SETTINGS_PROGRAMS,    "programs" },
                                                  { SETTINGS_WEATHER,     "weather" },
                                                  { SETTINGS_MUSIC,       "music" },
                                                  { SETTINGS_SYSTEM,      "system" },
                                                  { SETTINGS_VIDEOS,      "videos" },
                                                  { SETTINGS_SERVICE,     "services" },
                                                  { SETTINGS_APPEARANCE,  "appearance" },
                                                  { SETTINGS_PVR,         "pvr" },
                                                  { SETTINGS_GAMES,       "games" } };

#define SettingGroupSize sizeof(s_settingGroupMap) / sizeof(SettingGroup)

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml"),
      m_settings(CSettings::Get()),
      m_iSetting(0), m_iCategory(0), m_iSection(0),
      m_pOriginalSpin(NULL),
      m_pOriginalRadioButton(NULL),
      m_pOriginalCategoryButton(NULL),
      m_pOriginalButton(NULL),
      m_pOriginalEdit(NULL),
      m_pOriginalImage(NULL),
      m_delayedTimer(this),
      m_returningFromSkinLoad(false)
{
  m_loadType = KEEP_IN_MEMORY;

  newOriginalEdit = false;

  // set the correct ID range...
  m_idRange.clear();
  m_idRange.push_back(WINDOW_SETTINGS_MYPICTURES);
  m_idRange.push_back(WINDOW_SETTINGS_MYPROGRAMS);
  m_idRange.push_back(WINDOW_SETTINGS_MYWEATHER);
  m_idRange.push_back(WINDOW_SETTINGS_MYMUSIC);
  m_idRange.push_back(WINDOW_SETTINGS_SYSTEM);
  m_idRange.push_back(WINDOW_SETTINGS_MYVIDEOS);
  m_idRange.push_back(WINDOW_SETTINGS_SERVICE);
  m_idRange.push_back(WINDOW_SETTINGS_APPEARANCE);
  m_idRange.push_back(WINDOW_SETTINGS_MYPVR);
  m_idRange.push_back(WINDOW_SETTINGS_MYGAMES);
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{
  FreeControls();
  if (newOriginalEdit)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = NULL;
  }
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_delayedSetting.reset();
      if (message.GetParam1() != WINDOW_INVALID && !m_returningFromSkinLoad)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iCategory = 0;
        ResetControlStates();
      }
      
      m_iSection = (int)message.GetParam2() - (int)CGUIWindow::GetID();
      CGUIWindow::OnMessage(message);
      m_returningFromSkinLoad = false;
      return true;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      // cancel any delayed changes
      if (m_delayedSetting != NULL)
      {
        m_delayedTimer.Stop();
        CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), GetID());
        OnMessage(message);
      }
      
      CGUIWindow::OnMessage(message);
      FreeControls();
      return true;
    }
    
    case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      if (!m_returningFromSkinLoad)
      {
        int focusedControl = GetFocusedControlID();

        // cancel any delayed changes if the focused control has changed
        if (m_delayedSetting != NULL && m_delayedSetting->GetID() != focusedControl)
        {
          m_delayedTimer.Stop();
          CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID(), 1); // param1 = 1 for "reset the control if it's invalid"
          g_windowManager.SendThreadMessage(message, GetID());
        }
        // update the value of the previous setting (in case it was invalid)
        else if (m_iSetting >= CONTROL_START_CONTROL && m_iSetting < (int)(CONTROL_START_CONTROL + m_settingControls.size()))
        {
          CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_iSetting, 1); // param1 = 1 for "reset the control if it's invalid"
          g_windowManager.SendThreadMessage(message, GetID());
        }

        // check if we have changed the category and need to create new setting controls
        if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < (int)(CONTROL_START_BUTTONS + m_categories.size()))
        {
          int categoryIndex = focusedControl - CONTROL_START_BUTTONS;
          if (categoryIndex != m_iCategory)
          {
            if (!m_categories[categoryIndex]->CanAccess())
            {
              // unable to go to this category - focus the previous one
              SET_CONTROL_FOCUS(CONTROL_START_BUTTONS + m_iCategory, 0);
              return false;
            }

            m_iCategory = categoryIndex;
            CreateSettings();
          }
        }
        else if (focusedControl >= CONTROL_START_CONTROL && focusedControl < (int)(CONTROL_START_CONTROL + m_settingControls.size()))
        {
          m_iSetting = focusedControl;
          CSetting *setting = GetSettingControl(focusedControl)->GetSetting();
          if (setting != NULL)
            SetDescription(setting->GetHelp());
        }
      }
      return true;
    }

    case GUI_MSG_CLICKED:
    {
      BaseSettingControlPtr control = GetSettingControl(message.GetSenderId());
      if (control != NULL)
        OnClick(control);

      break;
    }

    case GUI_MSG_LOAD_SKIN:
    {
      if (IsActive())
        m_returningFromSkinLoad = true;
      break;
    }
    
    case GUI_MSG_UPDATE_ITEM:
    {
      if (m_delayedSetting != NULL)
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
      else if (message.GetControlId() >= CONTROL_START_CONTROL && message.GetControlId() < (int)(CONTROL_START_CONTROL + m_settingControls.size()))
      {
        BaseSettingControlPtr settingControl = GetSettingControl(message.GetControlId());
        if (settingControl.get() != NULL && settingControl->GetSetting() != NULL)
        {
          OnSettingChanged(settingControl->GetSetting());
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

    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
      {
        if (IsActive() && CDisplaySettings::Get().GetCurrentResolution() != g_graphicsContext.GetVideoResolution())
        {
          CDisplaySettings::Get().SetCurrentResolution(g_graphicsContext.GetVideoResolution(), true);
          CreateSettings();
        }
      }
      break;
    }
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowSettingsCategory::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_SETTINGS_RESET:
    {
      if (CGUIDialogYesNo::ShowAndGetInput(10041, 0, 10042, 0))
      {
        for(vector<BaseSettingControlPtr>::iterator it = m_settingControls.begin(); it != m_settingControls.end(); ++it)
        {
          CSetting *setting = (*it)->GetSetting();
          if (setting != NULL)
            setting->Reset();
        }
      }
      return true;
    }

    case ACTION_SETTINGS_LEVEL_CHANGE:
    {
      //Test if we can access the new level
      if (!g_passwordManager.CheckSettingLevelLock(CViewStateSettings::Get().GetNextSettingLevel(), true))
        return false;
      
      CViewStateSettings::Get().CycleSettingLevel();
      CSettings::Get().Save();

      // try to keep the current position
      std::string oldCategory;
      if (m_iCategory >= 0 && m_iCategory < (int)m_categories.size())
        oldCategory = m_categories[m_iCategory]->GetId();

      SET_CONTROL_LABEL(CONTRL_BTN_LEVELS, 10036 + (int)CViewStateSettings::Get().GetSettingLevel());
      // only re-create the categories, the settings will be created later
      SetupControls(false);

      m_iCategory = 0;
      // try to find the category that was previously selected
      if (!oldCategory.empty())
      {
        for (int i = 0; i < (int)m_categories.size(); i++)
        {
          if (m_categories[i]->GetId() == oldCategory)
          {
            m_iCategory = i;
            break;
          }
        }
      }

      CreateSettings();
      return true;
    }

    default:
      break;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnBack(int actionID)
{
  m_settings.Save();
  m_lastControlID = 0; // don't save the control as we go to a different window each time
  
  return CGUIWindow::OnBack(actionID);
}

void CGUIWindowSettingsCategory::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_iCategory);
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
  CGUIWindow::DoProcess(currentTime, dirtyregions);
  if (control && bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
}

void CGUIWindowSettingsCategory::OnInitWindow()
{
  SetupControls();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowSettingsCategory::OnWindowLoaded()
{
  SET_CONTROL_LABEL(CONTRL_BTN_LEVELS, 10036 + (int)CViewStateSettings::Get().GetSettingLevel());
}

void CGUIWindowSettingsCategory::SetupControls(bool createSettings /* = true */)
{
  // cleanup first, if necessary
  FreeControls();

  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalCategoryButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON);
  m_pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalCategoryButton || !m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton)
    return ;
  m_pOriginalEdit = (CGUIEditControl *)GetControl(CONTROL_DEFAULT_EDIT);
  if (!m_pOriginalEdit || m_pOriginalEdit->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
    newOriginalEdit = true;
  }

  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalCategoryButton->SetVisible(false);
  m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);

  // setup our control groups...
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!group)
    return;

  CSettingSection *section = GetSection(m_iSection);
  if (section == NULL)
    return;
  
  // update the screen string
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, section->GetLabel());
  
  SET_CONTROL_LABEL(CONTRL_BTN_LEVELS, 10036 + (int)CViewStateSettings::Get().GetSettingLevel());

  // get the categories we need
  m_categories = section->GetCategories(CViewStateSettings::Get().GetSettingLevel());

  // go through the categories and create the necessary buttons
  int buttonIdOffset = 0;
  for (SettingCategoryList::const_iterator category = m_categories.begin(); category != m_categories.end(); ++category)
  {
    CGUIButtonControl *pButton = NULL;
    if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
      pButton = new CGUIToggleButtonControl(*(CGUIToggleButtonControl *)m_pOriginalCategoryButton);
    else
      pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
    pButton->SetLabel(g_localizeStrings.Get((*category)->GetLabel()));
    pButton->SetID(CONTROL_START_BUTTONS + buttonIdOffset);
    pButton->SetVisible(true);
    pButton->AllocResources();

    group->AddControl(pButton);
    buttonIdOffset++;
  }

  if (createSettings)
    CreateSettings();

  // set focus correctly
  m_defaultControl = CONTROL_START_BUTTONS;
}

void CGUIWindowSettingsCategory::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_categories.clear();
  FreeSettingsControls();
}

void CGUIWindowSettingsCategory::FreeSettingsControls()
{
  // clear the settings group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for (std::vector<BaseSettingControlPtr>::iterator control = m_settingControls.begin(); control != m_settingControls.end(); ++control)
    (*control)->Clear();

  m_settingControls.clear();
  m_settings.UnregisterCallback(this);
}

void CGUIWindowSettingsCategory::OnTimeout()
{
  if (m_delayedSetting == NULL)
    return;

  // we send a thread message so that it's processed the following frame (some settings won't
  // like being changed during Render())
  CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), m_delayedSetting->GetID());
  g_windowManager.SendThreadMessage(message, GetID());
}

void CGUIWindowSettingsCategory::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL || setting->GetType() == SettingTypeNone ||
      setting->GetType() == SettingTypeAction)
    return;

  BaseSettingControlPtr pControl = GetSettingControl(setting->GetId());
  if (pControl == NULL)
    return;

  pControl->Update();
}

void CGUIWindowSettingsCategory::OnSettingPropertyChanged(const CSetting *setting, const char *propertyName)
{
  if (setting == NULL || propertyName == NULL)
    return;

  BaseSettingControlPtr settingControl = GetSettingControl(setting->GetId());
  if (settingControl == NULL)
    return;

  settingControl->Update();
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();

  if (m_categories.size() <= 0)
    return;

  if (m_iCategory < 0 || m_iCategory >= (int)m_categories.size())
    m_iCategory = 0;

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group == NULL)
    return;

  const CSettingCategory* category = m_categories.at(m_iCategory);
  if (category == NULL)
    return;

  // set the description of the current category
  SetDescription(category->GetHelp());

  std::set<std::string> settingMap;

  const SettingGroupList& groups = category->GetGroups(CViewStateSettings::Get().GetSettingLevel());
  int iControlID = CONTROL_START_CONTROL;
  bool first = true;
  for (SettingGroupList::const_iterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt)
  {
    if (*groupIt == NULL)
      continue;

    const SettingList& settings = (*groupIt)->GetSettings(CViewStateSettings::Get().GetSettingLevel());
    if (settings.size() <= 0)
      continue;

    if (first)
      first = false;
    else
      AddSeparator(group->GetWidth(), iControlID);

    for (SettingList::const_iterator settingIt = settings.begin(); settingIt != settings.end(); ++settingIt)
    {
      CSetting *pSetting = *settingIt;
      settingMap.insert(pSetting->GetId());
      AddSetting(pSetting, group->GetWidth(), iControlID);
    }
  }

  if (!settingMap.empty())
    m_settings.RegisterCallback(this, settingMap);
  
  // update our settings (turns controls on/off as appropriate)
  UpdateSettings();
}

void CGUIWindowSettingsCategory::UpdateSettings()
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

void CGUIWindowSettingsCategory::SetDescription(const CVariant &label)
{
  if (GetControl(CONTROL_DESCRIPTION) == NULL)
    return;

  if (label.isString())
    SET_CONTROL_LABEL(CONTROL_DESCRIPTION, label.asString());
  else if (label.isInteger() && label.asInteger() >= 0)
    SET_CONTROL_LABEL(CONTROL_DESCRIPTION, (int)label.asInteger());
  else
    SET_CONTROL_LABEL(CONTROL_DESCRIPTION, "");
}

CGUIControl* CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  if (pSetting == NULL)
    return NULL;

  BaseSettingControlPtr pSettingControl;
  CGUIControl *pControl = NULL;

  // determine the label and any possible indentation in case of sub settings
  string label = g_localizeStrings.Get(pSetting->GetLabel());
  int parentLevels = 0;
  CSetting *parentSetting = m_settings.GetSetting(pSetting->GetParent());
  while (parentSetting != NULL)
  {
    parentLevels++;
    parentSetting = m_settings.GetSetting(parentSetting->GetParent());
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
  switch (pSetting->GetControl().GetType())
  {
    case SettingControlTypeCheckmark:
    {
      pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
      if (pControl == NULL)
        return NULL;

      ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
      pSettingControl.reset(new CGUIControlRadioButtonSetting((CGUIRadioButtonControl *)pControl, iControlID, pSetting));
      break;
    }
    
    case SettingControlTypeSpinner:
    {
      pControl = new CGUISpinControlEx(*m_pOriginalSpin);
      if (pControl == NULL)
        return NULL;

      ((CGUISpinControlEx *)pControl)->SetText(label);
      pSettingControl.reset(new CGUIControlSpinExSetting((CGUISpinControlEx *)pControl, iControlID, pSetting));
      break;
    }
    
    case SettingControlTypeEdit:
    {
      pControl = new CGUIEditControl(*m_pOriginalEdit);
      if (pControl == NULL)
        return NULL;
      
      ((CGUIEditControl *)pControl)->SetLabel(label);
      pSettingControl.reset(new CGUIControlEditSetting((CGUIEditControl *)pControl, iControlID, pSetting));
      break;
    }
    
    case SettingControlTypeList:
    {
      pControl = new CGUIButtonControl(*m_pOriginalButton);
      if (pControl == NULL)
        return NULL;

      ((CGUIButtonControl *)pControl)->SetLabel(label);
      pSettingControl.reset(new CGUIControlListSetting((CGUIButtonControl *)pControl, iControlID, pSetting));
      break;
    }
    
    case SettingControlTypeButton:
    {
      pControl = new CGUIButtonControl(*m_pOriginalButton);
      if (pControl == NULL)
        return NULL;
      
      ((CGUIButtonControl *)pControl)->SetLabel(label);
      pSettingControl.reset(new CGUIControlButtonSetting((CGUIButtonControl *)pControl, iControlID, pSetting));
      break;
    }
    
    case SettingControlTypeNone:
    default:
      return NULL;
  }

  if (pSetting->GetControl().GetDelayed())
    pSettingControl->SetDelayed();

  return AddSettingControl(pControl, pSettingControl, width, iControlID);
}

CGUIControl* CGUIWindowSettingsCategory::AddSeparator(float width, int &iControlID)
{
  if (m_pOriginalImage == NULL)
    return NULL;

  CGUIControl *pControl = new CGUIImage(*m_pOriginalImage);
  if (pControl == NULL)
    return NULL;

  return AddSettingControl(pControl, BaseSettingControlPtr(new CGUIControlSeparatorSetting((CGUIImage *)pControl, iControlID)), width, iControlID);
}

CGUIControl* CGUIWindowSettingsCategory::AddSettingControl(CGUIControl *pControl, BaseSettingControlPtr pSettingControl, float width, int &iControlID)
{
  if (pControl == NULL)
  {
    pSettingControl.reset();
    return NULL;
  }
  
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  pControl->SetWidth(width);
  
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group != NULL)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
  }
  m_settingControls.push_back(pSettingControl);
  
  return pControl;
}

void CGUIWindowSettingsCategory::OnClick(BaseSettingControlPtr pSettingControl)
{
  // we need to first set the delayed setting and then execute OnClick()
  // because OnClick() triggers OnSettingChanged() and there we need to
  // know if the changed setting is delayed or not
  if (pSettingControl->IsDelayed())
  {
    m_delayedSetting = pSettingControl;
    if (m_delayedTimer.IsRunning())
      m_delayedTimer.Restart();
    else
      m_delayedTimer.Start(SETTING_DELAY);

    return;
  }

  // if changing the setting fails
  // we need to restore the proper state
  if (!pSettingControl->OnClick())
    pSettingControl->Update();
}

CSettingSection* CGUIWindowSettingsCategory::GetSection(int windowID) const
{
  for (size_t index = 0; index < SettingGroupSize; index++)
  {
    if (s_settingGroupMap[index].id == windowID)
      return m_settings.GetSection(s_settingGroupMap[index].name);
  }
  
  return NULL;
}

BaseSettingControlPtr CGUIWindowSettingsCategory::GetSettingControl(const std::string &strSetting)
{
  for (vector<BaseSettingControlPtr>::iterator control = m_settingControls.begin(); control != m_settingControls.end(); ++control)
  {
    if ((*control)->GetSetting() != NULL && (*control)->GetSetting()->GetId() == strSetting)
      return *control;
  }

  return BaseSettingControlPtr();
}

BaseSettingControlPtr CGUIWindowSettingsCategory::GetSettingControl(int controlId)
{
  if (controlId < CONTROL_START_CONTROL || controlId >= (int)(CONTROL_START_CONTROL + m_settingControls.size()))
    return BaseSettingControlPtr();

  return m_settingControls[controlId - CONTROL_START_CONTROL];
}
