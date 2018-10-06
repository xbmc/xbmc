/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CAppParamParser;
class CAdvancedSettings;
class CProfilesManager;
class CSettings;

class CSettingsComponent
{
public:
  CSettingsComponent();
  virtual ~CSettingsComponent();

  void Init(const CAppParamParser &params);
  bool Load();
  void Deinit();

  std::shared_ptr<CSettings> GetSettings();
  std::shared_ptr<CAdvancedSettings> GetAdvancedSettings();
  std::shared_ptr<CProfilesManager> GetProfilesManager();

protected:
  std::shared_ptr<CSettings> m_settings;
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  std::shared_ptr<CProfilesManager> m_profilesManager;

private:
  bool InitDirectoriesLinux(bool bPlatformDirectories);
  bool InitDirectoriesOSX(bool bPlatformDirectories);
  bool InitDirectoriesWin32(bool bPlatformDirectories);
  void CreateUserDirs() const;
};
