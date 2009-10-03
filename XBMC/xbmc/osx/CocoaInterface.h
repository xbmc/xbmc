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
  void Cocoa_DisableOSXScreenSaver(void);
  
  // DisplayLink
  //
  bool Cocoa_CVDisplayLinkCreate(void *displayLinkcallback, void *displayLinkContext);
  void Cocoa_CVDisplayLinkRelease(void);
  void Cocoa_CVDisplayLinkUpdate(void);
  double Cocoa_GetCVDisplayLinkRefreshPeriod(void);

  // AppleScript
  //
  void Cocoa_DoAppleScript(const char* scriptSource);
  
  // Devices
  //
  void Cocoa_MountPoint2DeviceName(char* path);
  
  //
  // Mouse.
  //
  void Cocoa_HideMouse();

  //
  // Smart folders.
  //
  void Cocoa_GetSmartFolderResults(const char* strFile, void (*)(void* userData, void* userData2, const char* path), void* userData, void* userData2);

  //
  // Version.
  //
  const char* Cocoa_GetAppVersion();
  
  void  Cocoa_MakeChildWindow();
  void  Cocoa_DestroyChildWindow();

  const char *Cocoa_Paste() ;  

#ifdef __cplusplus
}
#endif

#endif
