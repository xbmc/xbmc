/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
