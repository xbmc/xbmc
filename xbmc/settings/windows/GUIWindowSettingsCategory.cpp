/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSettingsCategory.h"

#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingSection.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "view/ViewStateSettings.h"

#include <string>

#define SETTINGS_SYSTEM                 WINDOW_SETTINGS_SYSTEM - WINDOW_SETTINGS_START
#define SETTINGS_SERVICE                WINDOW_SETTINGS_SERVICE - WINDOW_SETTINGS_START
#define SETTINGS_PVR                    WINDOW_SETTINGS_MYPVR - WINDOW_SETTINGS_START
#define SETTINGS_PLAYER                 WINDOW_SETTINGS_PLAYER - WINDOW_SETTINGS_START
#define SETTINGS_MEDIA                  WINDOW_SETTINGS_MEDIA - WINDOW_SETTINGS_START
#define SETTINGS_INTERFACE              WINDOW_SETTINGS_INTERFACE - WINDOW_SETTINGS_START
#define SETTINGS_GAMES                  WINDOW_SETTINGS_MYGAMES - WINDOW_SETTINGS_START

#define CONTROL_BTN_LEVELS               20

typedef struct {
  int id;
  std::string name;
} SettingGroup;

static const SettingGroup s_settingGroupMap[] = { { SETTINGS_SYSTEM,      "system" },
                                                  { SETTINGS_SERVICE,     "services" },
                                                  { SETTINGS_PVR,         "pvr" },
                                                  { SETTINGS_PLAYER,      "player" },
                                                  { SETTINGS_MEDIA,       "media" },
                                                  { SETTINGS_INTERFACE,   "interface" },
                                                  { SETTINGS_GAMES,       "games" } };

#define SettingGroupSize sizeof(s_settingGroupMap) / sizeof(SettingGroup)

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory()
    : CGUIDialogSettingsManagerBase(WINDOW_SETTINGS_SYSTEM, "SettingsCategory.xml"),
      m_settings(CServiceBroker::GetSettingsComponent()->GetSettings())
{
  // set the correct ID range...
  m_idRange.clear();
  m_idRange.push_back(WINDOW_SETTINGS_SYSTEM);
  m_idRange.push_back(WINDOW_SETTINGS_SERVICE);
  m_idRange.push_back(WINDOW_SETTINGS_MYPVR);
  m_idRange.push_back(WINDOW_SETTINGS_PLAYER);
  m_idRange.push_back(WINDOW_SETTINGS_MEDIA);
  m_idRange.push_back(WINDOW_SETTINGS_INTERFACE);
  m_idRange.push_back(WINDOW_SETTINGS_MYGAMES);
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory() = default;

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_iSection = message.GetParam2() - CGUIDialogSettingsManagerBase::GetID();
      CGUIDialogSettingsManagerBase::OnMessage(message);
      m_returningFromSkinLoad = false;

      if (!message.GetStringParam(0).empty())
        FocusElement(message.GetStringParam(0));

      return true;
    }

    case GUI_MSG_FOCUSED:
    {
      if (!m_returningFromSkinLoad)
        CGUIDialogSettingsManagerBase::OnMessage(message);
      return true;
    }

    case GUI_MSG_LOAD_SKIN:
    {
      if (IsActive())
        m_returningFromSkinLoad = true;
      break;
    }

    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
      {
        if (IsActive() && CDisplaySettings::GetInstance().GetCurrentResolution() != CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution())
        {
          CDisplaySettings::GetInstance().SetCurrentResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), true);
          CreateSettings();
        }
      }
      break;
    }

    case GUI_MSG_PLAYBACK_STARTED:
    case GUI_MSG_PLAYBACK_ENDED:
    case GUI_MSG_PLAYBACK_STOPPED:
    {
      if (IsActive())
      {
        UpdateSettings();
      }
      break;
    }
  }

  return CGUIDialogSettingsManagerBase::OnMessage(message);
}

bool CGUIWindowSettingsCategory::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_SETTINGS_LEVEL_CHANGE:
    {
      //Test if we can access the new level
      if (!g_passwordManager.CheckSettingLevelLock(CViewStateSettings::GetInstance().GetNextSettingLevel(), true))
        return false;

      CViewStateSettings::GetInstance().CycleSettingLevel();
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      // try to keep the current position
      std::string oldCategory;
      if (m_iCategory >= 0 && m_iCategory < (int)m_categories.size())
        oldCategory = m_categories[m_iCategory]->GetId();

      SET_CONTROL_LABEL(CONTROL_BTN_LEVELS, 10036 + (int)CViewStateSettings::GetInstance().GetSettingLevel());
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

  return CGUIDialogSettingsManagerBase::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnBack(int actionID)
{
  Save();
  return CGUIDialogSettingsManagerBase::OnBack(actionID);
}

void CGUIWindowSettingsCategory::OnWindowLoaded()
{
  SET_CONTROL_LABEL(CONTROL_BTN_LEVELS, 10036 + (int)CViewStateSettings::GetInstance().GetSettingLevel());
  CGUIDialogSettingsManagerBase::OnWindowLoaded();
}

int CGUIWindowSettingsCategory::GetSettingLevel() const
{
  return (int)CViewStateSettings::GetInstance().GetSettingLevel();
}

SettingSectionPtr CGUIWindowSettingsCategory::GetSection()
{
  for (const SettingGroup& settingGroup : s_settingGroupMap)
  {
    if (settingGroup.id == m_iSection)
      return m_settings->GetSection(settingGroup.name);
  }

  return NULL;
}

bool CGUIWindowSettingsCategory::Save()
{
  m_settings->Save();

  return true;
}

CSettingsManager* CGUIWindowSettingsCategory::GetSettingsManager() const
{
  return m_settings->GetSettingsManager();
}

void CGUIWindowSettingsCategory::FocusElement(const std::string& elementId)
{
  for (size_t i = 0; i < m_categories.size(); ++i)
  {
    if (m_categories[i]->GetId() == elementId)
    {
      SET_CONTROL_FOCUS(CONTROL_SETTINGS_START_BUTTONS + i, 0);
      return;
    }
    for (const auto& group: m_categories[i]->GetGroups())
    {
      for (const auto& setting : group->GetSettings())
      {
        if (setting->GetId() == elementId)
        {
          SET_CONTROL_FOCUS(CONTROL_SETTINGS_START_BUTTONS + i, 0);

          auto control = GetSettingControl(elementId);
          if (control)
            SET_CONTROL_FOCUS(control->GetID(), 0);
          else
            CLog::Log(LOGERROR,
                      "CGUIWindowSettingsCategory: failed to get control for setting '{}'.",
                      elementId);
          return;
        }
      }
    }
  }
  CLog::Log(LOGERROR,
            "CGUIWindowSettingsCategory: failed to set focus. unknown category/setting id '{}'.",
            elementId);
}
