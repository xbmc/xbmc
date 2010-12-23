/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifndef _OSX_INTERFACE_H_
#define _OSX_INTERFACE_H_

#include <string>
#include "StdString.h"
#include <Carbon/Carbon.h>

class CCocoaAutoPool
{
  public:
    CCocoaAutoPool();
    ~CCocoaAutoPool();
  private:
    void *m_opaque_pool;
};

#ifdef __cplusplus
extern "C"
{
#endif
  // Pools.
  //
  void* Cocoa_Create_AutoReleasePool(void);
  void Cocoa_Destroy_AutoReleasePool(void* pool);
  
  // Power and Screen
  //
  void Cocoa_UpdateSystemActivity(void);
  
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
  void Cocoa_MountPoint2DeviceName(char *path);
  bool Cocoa_GetVolumeNameFromMountPoint(const char *mountPoint, CStdString &volumeName);

  // Mouse.
  //
  void Cocoa_HideMouse();
  void Cocoa_ShowMouse();
  void Cocoa_HideDock();

  // Smart folders.
  //
  void Cocoa_GetSmartFolderResults(const char* strFile, void (*)(void* userData, void* userData2, const char* path), void* userData, void* userData2);

  // Version.
  //
  const char* Cocoa_GetAppVersion();
  bool Cocoa_HasVDADecoder();
  bool Cocoa_GPUForDisplayIsNvidiaPureVideo3();
  int Cocoa_GetOSVersion();

  
  void  Cocoa_MakeChildWindow();
  void  Cocoa_DestroyChildWindow();

  const char *Cocoa_Paste() ;  

  // helper from QA 1134
  // http://developer.apple.com/mac/library/qa/qa2001/qa1134.html
  OSStatus SendAppleEventToSystemProcess(AEEventID EventToSend);
#ifdef __cplusplus
}
#endif

#endif
