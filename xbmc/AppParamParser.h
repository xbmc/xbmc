/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

  int m_logLevel;
  bool m_startFullScreen = false;
  bool m_platformDirectories = true;
  bool m_testmode = false;
  bool m_standAlone = false;
  std::string m_windowing;
  std::string m_logTarget;

private:
  void ParseArg(const std::string &arg);
  void DisplayHelp();
  void DisplayVersion();

  std::string m_settingsFile;
  std::unique_ptr<CFileItemList> m_playlist;
};
