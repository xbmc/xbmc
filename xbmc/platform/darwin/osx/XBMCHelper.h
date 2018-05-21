#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#if !defined(__arm__) && !defined(__aarch64__)

#include "settings/lib/ISettingCallback.h"

enum AppleRemoteOptions
{
  APPLE_REMOTE_DISABLED    = 0,
  APPLE_REMOTE_STANDARD,
  APPLE_REMOTE_UNIVERSAL,
  APPLE_REMOTE_MULTIREMOTE
};

class XBMCHelper : public ISettingCallback
{
 public:
  static XBMCHelper& GetInstance();

  virtual bool OnSettingChanging(std::shared_ptr<const CSetting> setting) override;

  void Start();
  void Stop();

  void Configure();

  bool IsRunning();

  bool IsAlwaysOn() const { return m_alwaysOn; }
  int  GetMode() const { return m_mode; }

  bool ErrorStarting() { return m_errorStarting; }

private:
  XBMCHelper();
  XBMCHelper(XBMCHelper const& ) = delete;
  XBMCHelper& operator=(XBMCHelper const&) = delete;

  void HandleLaunchAgent();
  void Install();
  void Uninstall();

  bool IsRemoteBuddyInstalled();
  bool IsSofaControlRunning();

  int GetProcessPid(const char* processName);

  std::string ReadFile(const char* fileName);
  void WriteFile(const char* fileName, const std::string& data);

  bool m_alwaysOn;
  int  m_mode;
  int  m_sequenceDelay;
  int  m_port;
  bool m_errorStarting;

  std::string m_configFile;
  std::string m_launchAgentLocalFile;
  std::string m_launchAgentInstallFile;
  std::string m_homepath;
  std::string m_helperFile;

  static XBMCHelper* smp_instance;
};

#endif

