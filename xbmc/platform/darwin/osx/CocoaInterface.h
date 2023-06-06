/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#ifdef __cplusplus
extern "C"
{
#endif
  // DisplayLink
  //
  bool Cocoa_CVDisplayLinkCreate(void *displayLinkcallback, void *displayLinkContext);
  void Cocoa_CVDisplayLinkRelease(void);
  void Cocoa_CVDisplayLinkUpdate(void);

  // AppleScript
  //
  void Cocoa_DoAppleScript(const char* scriptSource);
  void Cocoa_DoAppleScriptFile(const char* filePath);

  // Devices
  //
  char* Cocoa_MountPoint2DeviceName(char *path);
  bool Cocoa_GetVolumeNameFromMountPoint(const std::string &mountPoint, std::string &volumeName);

  const char *Cocoa_Paste() ;

#ifdef __cplusplus
}
#endif

