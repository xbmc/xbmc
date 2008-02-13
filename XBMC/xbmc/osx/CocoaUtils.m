//
//  CocoaUtils.m
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

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
	NSOpenGLContext* context = (NSOpenGLContext* )theContext;
	[ context flushBuffer ];
}

#define MAX_DISPLAYS 32

void Cocoa_GetScreenResolution(int* w, int* h)
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
		printf("There are %d displays, requested was %d.\n", numDisplays, display_num);
		
		if (display_num <= numDisplays)
		{
			printf("Replacing display ID %d with %d\n", display_id, displayArray[display_num-1]);
			display_id = displayArray[display_num-1];
		}
	}
  }
  
  CFDictionaryRef save_mode  = CGDisplayCurrentMode(display_id);

  CFNumberGetValue(CFDictionaryGetValue(save_mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue(CFDictionaryGetValue(save_mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}