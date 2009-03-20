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
  // Pools.
  //
  void* Cocoa_Create_AutoReleasePool();
  void Cocoa_Destroy_AutoReleasePool(void* pool);
  
  // Graphics.
  //
  int Cocoa_GetScreenIndex(void);
  void Cocoa_GetScreenResolution(int* w, int* h);
  double Cocoa_GetScreenRefreshRate(int screen_index);
  void Cocoa_GetScreenResolutionOfAnotherScreen(int screen_index, int* w, int* h);
  int  Cocoa_GetNumDisplays();
  int  Cocoa_GetDisplayID(int screen_index);
  
  // Open GL.
  //
  void  Cocoa_GL_MakeCurrentContext(void* theContext);
  void  Cocoa_GL_ReleaseContext(void* context);
  void  Cocoa_GL_SwapBuffers(void* theContext);
  void* Cocoa_GL_GetWindowPixelFormat();
  void* Cocoa_GL_GetFullScreenPixelFormat(int screen_index);
  void* Cocoa_GL_GetCurrentContext();
  void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx);
  void* Cocoa_GL_ResizeWindow(void *theContext, int w, int h);
  void  Cocoa_GL_SetFullScreen(int width, int height, bool fs, bool blankOtherDisplay, bool GL_FullScreen);
  void  Cocoa_GL_EnableVSync(bool enable);

  // SDL Hack
  //
  void* Cocoa_GL_ReplaceSDLWindowContext();
  
  // Power and Screen
  //
  int  Cocoa_IdleDisplays();
  void Cocoa_UpdateSystemActivity();
  void Cocoa_DisableOSXScreenSaver();
  
  // AppleScript
  //
  void Cocoa_DoAppleScript(const char* scriptSource);
#endif
