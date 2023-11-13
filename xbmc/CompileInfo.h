/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace ADDON
{
struct RepoInfo;
}

class CCompileInfo
{
public:
  static int GetMajor();
  static int GetMinor();
  static const char* GetPackage();
  static const char* GetClass();
  static const char* GetAppName();
  static const char* GetSuffix(); // Git "Tag", e.g. alpha1
  static const char* GetSCMID(); // Git Revision
  static std::string GetSharedLibrarySuffix();
  static const char* GetCopyrightYears();
  static std::string GetBuildDate();
  static const char* GetVersionCode();
  static std::vector<std::string> GetAvailableWindowSystems();
  static std::vector<std::string> GetAvailableAudioBackends();
  static std::vector<std::string> GetAvailableGlInterfaces();
  static std::vector<ADDON::RepoInfo> LoadOfficialRepoInfos();
  static std::string GetPythonVersion();
  static std::vector<std::string> GetWebserverExtraWhitelist();
};
