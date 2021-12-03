/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/ilog.h"

#include <memory>
#include <string>

class CAdvancedSettings;
class CFileItemList;

class CAppParamParser
{
public:
  CAppParamParser();
  ~CAppParamParser();

  void Parse(const char* const* argv, int nArgs);
  void SetAdvancedSettings(CAdvancedSettings& advancedSettings) const;

  const CFileItemList& GetPlaylist() const;

  void SetLogLevel(int logLevel) { m_logLevel = logLevel; }
  void SetStartFullScreen(bool startFullScreen) { m_startFullScreen = startFullScreen; }
  void SetPlatformDirectories(bool platformDirectories)
  {
    m_platformDirectories = platformDirectories;
  }
  void SetStandAlone(bool standAlone) { m_standAlone = standAlone; }

  bool HasPlatformDirectories() const { return m_platformDirectories; }
  bool IsTestMode() const { return m_testmode; }
  bool IsStandAlone() const { return m_standAlone; }
  const std::string& GetWindowing() const { return m_windowing; }
  const std::string& GetLogTarget() const { return m_logTarget; }

protected:
  virtual void ParseArg(const std::string& arg);
  virtual void DisplayHelp();

  std::string m_windowing;
  std::string m_logTarget;

private:
  void DisplayVersion();

  int m_logLevel = LOG_LEVEL_NORMAL;
  bool m_startFullScreen = false;
  bool m_platformDirectories = true;
  bool m_testmode = false;
  bool m_standAlone = false;

  std::string m_settingsFile;
  std::unique_ptr<CFileItemList> m_playlist;
};
