/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _OSX_INTERFACE_H_
#define _OSX_INTERFACE_H_

#include <string>
#include "utils/StdString.h"
#include "AutoPool.h"

#ifdef __cplusplus
extern "C"
{
#endif
  // DisplayLink
  //
  bool Cocoa_CVDisplayLinkCreate(void *displayLinkcallback, void *displayLinkContext);
  void Cocoa_CVDisplayLinkRelease(void);
  void Cocoa_CVDisplayLinkUpdate(void);
  double Cocoa_GetCVDisplayLinkRefreshPeriod(void);

  // AppleScript
  //
  void Cocoa_DoAppleScript(const char* scriptSource);
  void Cocoa_DoAppleScriptFile(const char* filePath);
  
  // Application support
  //
  const char* Cocoa_GetIconFromBundle(const char *_bundlePath, const char *_iconName);
  
  // Devices
  //
  char* Cocoa_MountPoint2DeviceName(char *path);
  bool Cocoa_GetVolumeNameFromMountPoint(const char *mountPoint, CStdString &volumeName);

  // Mouse.
  //
  void Cocoa_HideMouse();
  void Cocoa_ShowMouse();
  void Cocoa_HideDock();

  // Version.
  //
  const char* Cocoa_GetAppVersion();
  bool Cocoa_HasVDADecoder();
  bool Cocoa_GPUForDisplayIsNvidiaPureVideo3();
  int Cocoa_GetOSVersion();

  
  void  Cocoa_MakeChildWindow();
  void  Cocoa_DestroyChildWindow();

  const char *Cocoa_Paste() ;  

#ifdef __cplusplus
}
#endif

#endif
