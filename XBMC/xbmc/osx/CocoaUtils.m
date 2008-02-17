//
//  CocoaUtils.m
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#include "CocoaUtils.h"

void* InitializeAutoReleasePool()
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  return pool;
}

void DestroyAutoReleasePool(void* aPool)
{
  NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
  [pool release];
}

void Cocoa_GL_MakeCurrentContext(void* theContext)
{
  NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ context makeCurrentContext ];
}

void Cocoa_GL_ReleaseContext(void* theContext)
{
  NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ NSOpenGLContext clearCurrentContext ];
  [ context clearDrawable ];
  [ context release ];
}

void Cocoa_GL_SwapBuffers(void* theContext)
{
  [ (NSOpenGLContext*)theContext flushBuffer ];
}

#define MAX_DISPLAYS 32

int Cocoa_GetCurrentDisplay()
{
  // Figure out the screen size.
  CGDirectDisplayID display_id = kCGDirectMainDisplay;

  const char* strDisplayNum = getenv("SDL_VIDEO_FULLSCREEN_DISPLAY");
  if (strDisplayNum != 0)
  {
    int display_num = atoi(strDisplayNum);
    if (display_num != 0)
    {
      CGDirectDisplayID displayArray[MAX_DISPLAYS];
      CGDisplayCount    numDisplays;

      // Get the list of displays.
      CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
      fprintf(stderr, "There are %d displays, requested was %d.\n", numDisplays, display_num);

      if (display_num <= numDisplays)
      {
        fprintf(stderr, "Replacing display ID %d with %d\n", display_id, displayArray[display_num-1]);
        display_id = displayArray[display_num-1];
      }
    }
  }
  return (int)display_id;
}

void Cocoa_GetScreenResolution(int* w, int* h)
{
  // Figure out the screen size.
  CGDirectDisplayID display_id = Cocoa_GetCurrentDisplay();
  CFDictionaryRef save_mode  = CGDisplayCurrentMode(display_id);

  CFNumberGetValue(CFDictionaryGetValue(save_mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue(CFDictionaryGetValue(save_mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

void Cocoa_GL_ResizeWindow(void *theContext, int w, int h)
{
  if (!theContext)
    return;

  NSOpenGLContext* context = Cocoa_GL_GetCurrentContext();
  NSView* view;
  NSWindow* window;

  view = [context view];
  if (view && w>0 && h>0)
  {
    window = [view window];
    if (window)
    {
      [window setContentSize:NSMakeSize(w,h)];
      [window update];
      [view setFrameSize:NSMakeSize(w, h)];
      [context update];
    }
  }
}

void Cocoa_GL_SetFullScreen(bool fs)
{
  static NSView* lastView = NULL;
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();

  if (!context)
    return;

  if (fs)
  {
    // obtain fullscreen pixel format
    NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetFullScreenPixelFormat();
    if (!pixFmt)
      return;

    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*) pixFmt,
                                                                           (void*)context);

    // release pixelformat
    [pixFmt release];
    pixFmt = nil;

    if (!newContext)
      return;
    
    // save the view
    lastView = [context view];

    // clear the current context
    [NSOpenGLContext clearCurrentContext];

    // capture all displays before going fullscreen
    CGDisplayCapture(Cocoa_GetCurrentDisplay());

    // set fullscreen
    [newContext setFullScreen];
    
    // release old context
    Cocoa_GL_ReleaseContext((void*)context);

    // activate context
    [newContext makeCurrentContext];
  }
  else
  {
    // exit fullscreen
    [context clearDrawable];

    // release display
    CGDisplayRelease(Cocoa_GetCurrentDisplay());

    // obtain windowed pixel format
    NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
    if (!pixFmt)
      return;
    
    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*) pixFmt,
                                                                           (void*)context);
    
    // release pixelformat
    [pixFmt release];
    pixFmt = nil;
      
    if (!newContext)
      return;
    
    // assign view from old context
    [newContext setView:lastView];
    
    // release the fullscreen context
    Cocoa_GL_ReleaseContext((void*)context);
    
    // activate context
    [newContext makeCurrentContext];
  }
}

void Cocoa_GL_EnableVSync(bool enable)
{
  CGLContextObj cglContext;
  cglContext = CGLGetCurrentContext();
  if (cglContext)
  {
    GLint interval;
    if (enable)
    {
      interval = 1;
    }
    else
    {
      interval = 0;
    }
    CGLSetParameter(cglContext, kCGLCPSwapInterval, &interval);
  }
}

void* Cocoa_GL_GetWindowPixelFormat()
{
  NSOpenGLPixelFormatAttribute wattrs[] =
    {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAWindow,
      NSOpenGLPFANoRecovery,
      NSOpenGLPFAAccelerated,
      //NSOpenGLPFAColorSize, 32,
      //NSOpenGLPFAAlphaSize, 8,
      0
    };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetFullScreenPixelFormat()
{
  NSOpenGLPixelFormatAttribute fsattrs[] =
    {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAFullScreen,
      NSOpenGLPFANoRecovery,
      NSOpenGLPFAAccelerated,
      NSOpenGLPFAScreenMask,
      CGDisplayIDToOpenGLDisplayMask((CGDirectDisplayID)Cocoa_GetCurrentDisplay()),
      0
    };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetCurrentContext()
{
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  return (void*)context;
}

void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx)
{
  if (!pixFmt)
    return nil;
  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:pixFmt
                                 shareContext:(NSOpenGLContext*)shareCtx];
  return newContext;
}

void Cocoa_GL_ReplaceSDLWindowContext()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];

  if (!view)
    return;

  // disassociate view from context
  [context clearDrawable];

  // release the context
  Cocoa_GL_ReleaseContext((void*)context);

  // obtain window pixelformat
  NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
  if (!pixFmt)
    return;

  NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*)pixFmt, nil);
  [pixFmt release];

  if (!newContext)
    return;

  // associate with current view
  [newContext setView:view];
  [newContext makeCurrentContext];
}

