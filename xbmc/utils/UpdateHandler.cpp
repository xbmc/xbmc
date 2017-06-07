/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 
#include "UpdateHandler.h"
#include "IUpdater.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "ServiceBroker.h"

#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/UpdaterOsx.h"
#elif defined(TARGET_WINDOWS)
#include "platform/win32/UpdaterWindows.h"
#endif

IUpdater *CUpdateHandler::impl = NULL;
CUpdateHandler CUpdateHandler::sUpdateHandler;

CUpdateHandler::CUpdateHandler()
{
  if (impl == NULL)
  {
#if defined(TARGET_DARWIN_OSX)
    impl = new CUpdaterOsx();
#elif defined(TARGET_WINDOWS)
    impl = new CUpdaterWindows();
#endif
  }
}

CUpdateHandler::~CUpdateHandler()
{
  if (impl)
  {
    delete impl;
  }
}

CUpdateHandler &CUpdateHandler::GetInstance()
{
  return sUpdateHandler;
}

void CUpdateHandler::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "updates.checkforupdates")
    CheckForUpdate();
}

void CUpdateHandler::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "updates.enableautoupdate")
  {
    SetAutoUpdateEnabled(std::static_pointer_cast<const CSettingBool>(setting)->GetValue());
  }
}

void CUpdateHandler::OnSettingsLoaded()
{
  if (HasExternalSettingsStorage())
    CServiceBroker::GetSettings().SetBool("updates.enableautoupdate", GetAutoUpdateEnabled());
}
 
void CUpdateHandler::Init()
{
  if (impl != NULL)
  {
    impl->Init();
  }
}

void CUpdateHandler::Deinit()
{
  if (impl != NULL)
  {
    impl->Deinit();
  }
}

void CUpdateHandler::SetAutoUpdateEnabled(bool enabled)
{
  if (impl != NULL)
  {
    impl->SetAutoUpdateEnabled(enabled);
  }
}

bool CUpdateHandler::GetAutoUpdateEnabled()
{
  if (impl != NULL)
  {
    return impl->GetAutoUpdateEnabled();
  }
  return false;
}

void CUpdateHandler::CheckForUpdate()
{
  if (impl != NULL)
  {
    impl->CheckForUpdate();
  }
}

bool CUpdateHandler::UpdateSupported()
{
  if (impl != NULL)
  {
    return impl->UpdateSupported();
  }
  return false;
}

bool CUpdateHandler::HasExternalSettingsStorage()
{
  if (impl != NULL)
  {
    return impl->HasExternalSettingsStorage();
  }
  return false;
}
