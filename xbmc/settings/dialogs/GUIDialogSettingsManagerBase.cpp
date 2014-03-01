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

#include "GUIDialogSettingsManagerBase.h"
#include "settings/lib/SettingsManager.h"

using namespace std;

CGUIDialogSettingsManagerBase::CGUIDialogSettingsManagerBase(int windowId, const std::string &xmlFile)
    : CGUIDialogSettingsBase(windowId, xmlFile),
      m_settingsManager(NULL)
{ }

CGUIDialogSettingsManagerBase::~CGUIDialogSettingsManagerBase()
{
  m_settingsManager = NULL;
}

CSetting* CGUIDialogSettingsManagerBase::GetSetting(const std::string &settingId)
{
  assert(m_settingsManager != NULL);

  return m_settingsManager->GetSetting(settingId);
}

std::set<std::string> CGUIDialogSettingsManagerBase::CreateSettings()
{
  assert(m_settingsManager != NULL);

  std::set<std::string> settings = CGUIDialogSettingsBase::CreateSettings();

  if (!settings.empty())
    m_settingsManager->RegisterCallback(this, settings);

  return settings;
}

void CGUIDialogSettingsManagerBase::FreeSettingsControls()
{
  assert(m_settingsManager != NULL);

  CGUIDialogSettingsBase::FreeSettingsControls();

  m_settingsManager->UnregisterCallback(this);
}

ISettingControl* CGUIDialogSettingsManagerBase::CreateControl(const std::string &controlType)
{
  assert(m_settingsManager != NULL);

  return m_settingsManager->CreateControl(controlType);
}
