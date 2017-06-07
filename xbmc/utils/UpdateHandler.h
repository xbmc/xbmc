#pragma once
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
 
#include "IUpdater.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"

class CSetting;

class CUpdateHandler : public IUpdater, public ISettingCallback, public ISettingsHandler
{
public:
  static CUpdateHandler &GetInstance();

  // IUpdater
  virtual void Init() override;
  virtual void Deinit() override;
  virtual void SetAutoUpdateEnabled(bool enabled) override;
  virtual bool GetAutoUpdateEnabled() override;
  virtual void CheckForUpdate() override;
  virtual bool UpdateSupported() override;
  virtual bool HasExternalSettingsStorage() override;
  
  // ISettingCallback
  virtual void OnSettingAction(std::shared_ptr<const CSetting> setting) override;
  virtual void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  // ISettingsHandler
  virtual void OnSettingsLoaded() override;
  
protected:
  CUpdateHandler();
  ~CUpdateHandler();

private:
  static IUpdater *impl;
  static CUpdateHandler sUpdateHandler;
};
 