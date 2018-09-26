/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"

#include <string>

class CAdvancedSettings;

class CAppParamParser
{
public:
  CAppParamParser();
  void Parse(const char* const* argv, int nArgs);
  void SetAdvancedSettings(CAdvancedSettings& advancedSettings) const;

  const CFileItemList &Playlist() const { return m_playlist; }

  int m_logLevel;
  bool m_startFullScreen = false;

private:
  void ParseArg(const std::string &arg);
  void DisplayHelp();
  void DisplayVersion();

  bool m_testmode = false;
  bool m_standAlone = false;
  std::string m_settingsFile;
  CFileItemList m_playlist;
};
