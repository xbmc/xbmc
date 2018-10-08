/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

class CProfileManager;

// static class for path translation from our special:// URLs.

/* paths are as follows:

 special://xbmc/          - the main XBMC folder (i.e. where the app resides).
 special://home/          - a writeable version of the main XBMC folder
                             Linux: ~/.kodi/
                             OS X:  ~/Library/Application Support/Kodi/
                             Win32: ~/Application Data/XBMC/
 special://envhome/       - on posix systems this will be equal to the $HOME
 special://userhome/      - a writable version of the user home directory
                             Linux, OS X: ~/.kodi
                             Win32: home directory of user
 special://masterprofile/ - the master users userdata folder - usually special://home/userdata
                             Linux: ~/.kodi/userdata/
                             OS X:  ~/Library/Application Support/Kodi/UserData/
                             Win32: ~/Application Data/XBMC/UserData/
 special://profile/       - the current users userdata folder - usually special://masterprofile/profiles/<current_profile>
                             Linux: ~/.kodi/userdata/profiles/<current_profile>
                             OS X:  ~/Library/Application Support/Kodi/UserData/profiles/<current_profile>
                             Win32: ~/Application Data/XBMC/UserData/profiles/<current_profile>

 special://temp/          - the temporary directory.
                             Linux: ~/.kodi/temp
                             OS X:  ~/
                             Win32: ~/Application Data/XBMC/cache
*/
class CURL;
class CSpecialProtocol
{
public:
  static void RegisterProfileManager(const CProfileManager &profileManager);
  static void UnregisterProfileManager();

  static void SetProfilePath(const std::string &path);
  static void SetXBMCPath(const std::string &path);
  static void SetXBMCBinPath(const std::string &path);
  static void SetXBMCBinAddonPath(const std::string &path);
  static void SetXBMCAltBinAddonPath(const std::string &path);
  static void SetXBMCFrameworksPath(const std::string &path);
  static void SetHomePath(const std::string &path);
  static void SetUserHomePath(const std::string &path);
  static void SetEnvHomePath(const std::string &path);
  static void SetMasterProfilePath(const std::string &path);
  static void SetTempPath(const std::string &path);
  static void SetLogPath(const std::string &dir);

  static bool ComparePath(const std::string &path1, const std::string &path2);
  static void LogPaths();

  static std::string TranslatePath(const std::string &path);
  static std::string TranslatePath(const CURL &url);
  static std::string TranslatePathConvertCase(const std::string& path);

private:
  static const CProfileManager *m_profileManager;

  static void SetPath(const std::string &key, const std::string &path);
  static std::string GetPath(const std::string &key);

  static std::map<std::string, std::string> m_pathMap;
};

#ifdef TARGET_WINDOWS
#define PATH_SEPARATOR_CHAR '\\'
#define PATH_SEPARATOR_STRING "\\"
#else
#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STRING "/"
#endif
