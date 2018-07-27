/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

// We forward declare CFStringRef in order to avoid
// pulling in tons of Objective-C headers.
struct __CFString;
typedef const struct __CFString * CFStringRef;

class CDarwinUtils
{
public:
  static const char *getIosPlatformString(void);
  static bool        DeviceHasRetina(double &scale);
  static const char *GetOSReleaseString(void);
  static const char *GetOSVersionString(void);
  static float       GetIOSVersion(void);
  static const char *GetIOSVersionString(void);
  static const char *GetOSXVersionString(void);
  static int         GetFrameworkPath(bool forPython, char* path, size_t *pathsize);
  static int         GetExecutablePath(char* path, size_t *pathsize);
  static const char *GetAppRootFolder(void);
  static bool        IsIosSandboxed(void);
  static bool        HasVideoToolboxDecoder(void);
  static int         BatteryLevel(void);
  static void        EnableOSScreenSaver(bool enable);
  static void        ResetSystemIdleTimer();
  static void        SetScheduling(bool realtime);
  static void        PrintDebugString(std::string debugString);
  static bool        CFStringRefToString(CFStringRef source, std::string& destination);
  static bool        CFStringRefToUTF8String(CFStringRef source, std::string& destination);
  static const std::string&  GetManufacturer(void);
  static bool        IsAliasShortcut(const std::string& path, bool isdirectory);
  static void        TranslateAliasShortcut(std::string& path);
  static bool        CreateAliasShortcut(const std::string& fromPath, const std::string& toPath);
};

