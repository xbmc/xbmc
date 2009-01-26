//
//  Utils.h
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#ifndef _OSX_UTILS_H_
#define _OSX_UTILS_H_

#ifdef __cplusplus
extern "C" 
{
#endif
  // Initialization.
  //
  void Cocoa_Initialize(void* pApplication);

  // Pools.
  //
  void* InitializeAutoReleasePool();
  void DestroyAutoReleasePool(void* pool);
  
  // Graphics.
  //
  int Cocoa_GetScreenIndex(void);
  void Cocoa_GetScreenResolution(int* w, int* h);
  double Cocoa_GetScreenRefreshRate(int screen_id);
  void Cocoa_GetScreenResolutionOfAnotherScreen(int display, int* w, int* h);
  int  Cocoa_GetNumDisplays();
  int  Cocoa_GetDisplayID(int screen);
  
  // Open GL.
  //
  void  Cocoa_GL_MakeCurrentContext(void* theContext);
  void  Cocoa_GL_ReleaseContext(void* context);
  void  Cocoa_GL_SwapBuffers(void* theContext);
  void* Cocoa_GL_GetWindowPixelFormat();
  void* Cocoa_GL_GetFullScreenPixelFormat(int screen);
  void* Cocoa_GL_GetCurrentContext();
  void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx);
  void* Cocoa_GL_ResizeWindow(void *theContext, int w, int h, void* sdlView);
  void  Cocoa_GL_SetFullScreen(int width, int height, bool fs, bool blankOtherDisplay, bool GL_FullScreen);
  void  Cocoa_GL_EnableVSync(bool enable);

  // Blanking.
  //
  void Cocoa_GL_UnblankOtherDisplays(int screen);
  void Cocoa_GL_BlankOtherDisplays(int screen);

  // SDL Hack
  //
  void* Cocoa_GL_ReplaceSDLWindowContext();
  
  // Power and Screen
  //
  int  Cocoa_IdleDisplays();
  void Cocoa_UpdateSystemActivity();
  int  Cocoa_SleepSystem();

#ifdef __cplusplus
}
#endif

#endif
