/*
 *      Copyright (C) 2005-2014 Team XBMC
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
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "input/Key.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/lib/SettingSection.h"
#include "view/ViewStateSettings.h"

using namespace std;

#define SETTINGS_PICTURES               WINDOW_SETTINGS_MYPICTURES - WINDOW_SETTINGS_START
#define SETTINGS_PROGRAMS               WINDOW_SETTINGS_MYPROGRAMS - WINDOW_SETTINGS_START
#define SETTINGS_WEATHER                WINDOW_SETTINGS_MYWEATHER - WINDOW_SETTINGS_START
#define SETTINGS_MUSIC                  WINDOW_SETTINGS_MYMUSIC - WINDOW_SETTINGS_START
#define SETTINGS_SYSTEM                 WINDOW_SETTINGS_SYSTEM - WINDOW_SETTINGS_START
#define SETTINGS_VIDEOS                 WINDOW_SETTINGS_MYVIDEOS - WINDOW_SETTINGS_START
#define SETTINGS_SERVICE                WINDOW_SETTINGS_SERVICE - WINDOW_SETTINGS_START
#define SETTINGS_APPEARANCE             WINDOW_SETTINGS_APPEARANCE - WINDOW_SETTINGS_START
#define SETTINGS_PVR                    WINDOW_SETTINGS_MYPVR - WINDOW_SETTINGS_START

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
                                                  { SETTINGS_PVR,         "pvr" } };
                                                  
#define SettingGroupSize sizeof(s_settingGroupMap) / sizeof(SettingGroup)

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory()
    : CGUIDialogSettingsManagerBase(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml"),
      m_settings(CSettings::Get()),
      m_iSection(0),
      m_returningFromSkinLoad(false)
{
  m_settingsManager = m_settings.GetSettingsManager();

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
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory()
{ }

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_iSection = (int)message.GetParam2() - (int)CGUIDialogSettingsManagerBase::GetID();
      CGUIDialogSettingsManagerBase::OnMessage(message);
      m_returningFromSkinLoad = false;
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
        if (IsActive() && CDisplaySettings::Get().GetCurrentResolution() != g_graphicsContext.GetVideoResolution())
        {
          CDisplaySettings::Get().SetCurrentResolution(g_graphicsContext.GetVideoResolution(), true);
          CreateSettings();
        }
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

  return CGUIDialogSettingsManagerBase::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnBack(int actionID)
{
  Save();
  return CGUIDialogSettingsManagerBase::OnBack(actionID);
}

void CGUIWindowSettingsCategory::OnWindowLoaded()
{
  SET_CONTROL_LABEL(CONTRL_BTN_LEVELS, 10036 + (int)CViewStateSettings::Get().GetSettingLevel());
  CGUIDialogSettingsManagerBase::OnWindowLoaded();
}

int CGUIWindowSettingsCategory::GetSettingLevel() const
{
  return (int)CViewStateSettings::Get().GetSettingLevel();
}

CSettingSection* CGUIWindowSettingsCategory::GetSection()
{
  for (size_t index = 0; index < SettingGroupSize; index++)
  {
    if (s_settingGroupMap[index].id == m_iSection)
      return m_settings.GetSection(s_settingGroupMap[index].name);
  }

  return NULL;
}

void CGUIWindowSettingsCategory::Save()
{
  m_settings.Save();
}
