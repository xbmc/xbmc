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
  //
  // Pools.
  //
  void* InitializeAutoReleasePool();
  void DestroyAutoReleasePool(void* pool);
  
  //
  // Graphics.
  //
  void Cocoa_GetScreenResolution(int* w, int* h);
  int  Cocoa_GetCurrentDisplay();
  
  //
  // Open GL.
  //
  void  Cocoa_GL_MakeCurrentContext(void* theContext);
  void  Cocoa_GL_ReleaseContext(void* context);
  void  Cocoa_GL_SwapBuffers(void* theContext);
  void* Cocoa_GL_GetWindowPixelFormat();
  void* Cocoa_GL_GetFullScreenPixelFormat();
  void* Cocoa_GL_GetCurrentContext();
  void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx);
  void  Cocoa_GL_ResizeWindow(void *theContext, int w, int h);
  void  Cocoa_GL_SetFullScreen(bool fs);
  void  Cocoa_GL_EnableVSync(bool enable);

  //
  // SDL Hack
  //
  void Cocoa_GL_ReplaceSDLWindowContext();

#ifdef __cplusplus
}
#endif

#endif
