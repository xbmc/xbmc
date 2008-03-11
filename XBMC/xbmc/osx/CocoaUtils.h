//
//  Utils.h
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#ifndef _OSX_UTILS_H_
#define _OSX_UTILS_H_

#include "AppleRemoteKeys.h"

#ifdef __cplusplus
extern "C" 
{
#endif
  //
  // Initialization.
  //
  void Cocoa_Initialize(void* pApplication);

  //
  // Pools.
  //
  void* InitializeAutoReleasePool();
  void DestroyAutoReleasePool(void* pool);
  
  //
  // Graphics.
  //
  void Cocoa_GetScreenResolution(int* w, int* h);
  double Cocoa_GetScreenRefreshRate(int screen);
  void Cocoa_GetScreenResolutionOfAnotherScreen(int display, int* w, int* h);
  int  Cocoa_GetNumDisplays();
  int  Cocoa_GetDisplay(int screen);
  
  //
  // Open GL.
  //
  void  Cocoa_GL_MakeCurrentContext(void* theContext);
  void  Cocoa_GL_ReleaseContext(void* context);
  void  Cocoa_GL_SwapBuffers(void* theContext);
  void* Cocoa_GL_GetWindowPixelFormat();
  void* Cocoa_GL_GetFullScreenPixelFormat(int screen);
  void* Cocoa_GL_GetCurrentContext();
  void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx);
  void  Cocoa_GL_ResizeWindow(void *theContext, int w, int h);
  void  Cocoa_GL_SetFullScreen(int screen, int width, int height, bool fs);
  void  Cocoa_GL_EnableVSync(bool enable);

  //
  // SDL Hack
  //
  void Cocoa_GL_ReplaceSDLWindowContext();

#ifdef __cplusplus
}
#endif

#endif
