/*
 *  Copyright (C) 2005-202 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/ilog.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class CFileItemList;

class CAppParams
{
public:
  CAppParams();
  virtual ~CAppParams() = default;

  int GetLogLevel() const { return m_logLevel; }
  void SetLogLevel(int logLevel) { m_logLevel = logLevel; }

  bool IsStartFullScreen() const { return m_startFullScreen; }
  void SetStartFullScreen(bool startFullScreen) { m_startFullScreen = startFullScreen; }

  bool IsStandAlone() const { return m_standAlone; }
  void SetStandAlone(bool standAlone) { m_standAlone = standAlone; }

  bool HasPlatformDirectories() const { return m_platformDirectories; }
  void SetPlatformDirectories(bool platformDirectories)
  {
    m_platformDirectories = platformDirectories;
  }

  bool IsTestMode() const { return m_testmode; }
  void SetTestMode(bool testMode) { m_testmode = testMode; }

  const std::string& GetSettingsFile() const { return m_settingsFile; }
  void SetSettingsFile(const std::string& settingsFile) { m_settingsFile = settingsFile; }

  const std::string& GetWindowing() const { return m_windowing; }
  void SetWindowing(const std::string& windowing) { m_windowing = windowing; }

  const std::string& GetLogTarget() const { return m_logTarget; }
  void SetLogTarget(const std::string& logTarget) { m_logTarget = logTarget; }

  std::string_view GetAudioBackend() const { return m_audioBackend; }
  void SetAudioBackend(std::string_view audioBackend) { m_audioBackend = audioBackend; }

  std::string_view GetGlInterface() const { return m_glInterface; }
  void SetGlInterface(const std::string& glInterface) { m_glInterface = glInterface; }

  CFileItemList& GetPlaylist() const { return *m_playlist; }

  /*!
   * \brief Get the raw command-line arguments
   *
   * Note: Raw arguments are currently not used by Kodi, but they will be
   * useful if/when ROS 2 support is ever merged.
   *
   * \return The arguments. Note that the leading argument is the executable
   * path name.
   */
  const std::vector<std::string>& GetRawArgs() const { return m_rawArgs; }

  /*!
   * \brief Set the raw command-line arguments
   *
   * \args The arguments. Note that the leading argument is the executable path
   * name.
   */
  void SetRawArgs(std::vector<std::string> args);

private:
  int m_logLevel{LOG_LEVEL_NORMAL};

  bool m_startFullScreen{false};
  bool m_standAlone{false};
  bool m_platformDirectories{true};
  bool m_testmode{false};

  std::string m_settingsFile;
  std::string m_windowing;
  std::string m_logTarget;
  std::string m_audioBackend;
  std::string m_glInterface;

  std::unique_ptr<CFileItemList> m_playlist;

  // The raw command-line arguments
  std::vector<std::string> m_rawArgs;
};
