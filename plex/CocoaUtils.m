//
//  CocoaUtils.m
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>

#import <IOKit/graphics/IOGraphicsLib.h>
#import <ApplicationServices/ApplicationServices.h>
#import <SystemConfiguration/SystemConfiguration.h>

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOKitLib.h>
#include <SDL/SDL.h>
#import <QuartzCore/QuartzCore.h>

#ifdef WORKING
#include "CocoaUtils.h"
#include "CocoaToCppThunk.h"

#import "XBMCMain.h" 
#import "BackgroundMusicPlayer.h"
#import "AppleHardwareInfo.h"
#import "SUPlexUpdater.h"

#import "AdvancedSettingsController.h"
#import "PlexApplication.h"
#import "PlexCompatibility.h"

extern int GetProcessPid(const char* processName);
extern void CocoaPlus_Initialize();

#define MAX_DISPLAYS 32
static NSWindow* blankingWindows[MAX_DISPLAYS];
static float blankingBrightness[MAX_DISPLAYS];
static int mainDisplayScreen;

void Cocoa_Initialize(void* pApplication)
{
  // Intialize the Apple remote code.
  [[XBMCMain sharedInstance] setApplication: pApplication];
  
  // Initialize.
  int i;
  for (i=0; i<MAX_DISPLAYS; i++)
  {
    blankingWindows[i] = 0;
    blankingBrightness[i] = -1.0f;
    mainDisplayScreen = 0;
  }

  CocoaPlus_Initialize();
}

void Cocoa_ActivateWindow()
{
  [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

void Cocoa_DisplayError(const char* strError)
{
  NSAlert *alert = [NSAlert alertWithMessageText:@"Fatal Error"
                    defaultButton:@"OK" alternateButton:nil otherButton:nil
                    informativeTextWithFormat:[NSString stringWithUTF8String:strError]];
                    
  [alert runModal];
  [alert release];
}

void InstallCrashReporter() 
{
  if (access("/Library/InputManagers/Smart Crash Reports/Smart Crash Reports.bundle", R_OK) != 0)
    system([[[NSBundle mainBundle] pathForAuxiliaryExecutable:@"CrashReporter"] UTF8String]);
}

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

NSWindow* childWindow = nil;
NSWindow* mainWindow = nil;

void Cocoa_MakeChildWindow()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  NSWindow* window = [view window];

  // Create a child window.
  childWindow = [[NSWindow alloc] initWithContentRect:[window frame]
                                            styleMask:NSBorderlessWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
                                          
  [childWindow setContentSize:[view frame].size];
  [childWindow setBackgroundColor:[NSColor blackColor]];
  [window addChildWindow:childWindow ordered:NSWindowAbove];
  mainWindow = window;
  //childWindow.alphaValue = 0.5; 
}

void Cocoa_DestroyChildWindow()
{
  if (childWindow != nil)
  {
    [mainWindow removeChildWindow:childWindow];
    [childWindow close];
    childWindow = nil;
  }
}

void Cocoa_GL_SwapBuffers(void* theContext)
{
  [ (NSOpenGLContext*)theContext flushBuffer ];
}

int Cocoa_GetNumDisplays()
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  
  return numDisplays;
}

int Cocoa_GetDisplay(int screen)
{
	CGDirectDisplayID displayArray[MAX_DISPLAYS];
	CGDisplayCount    numDisplays;
  
	// Get the list of displays.
	CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  if (displayArray[screen] == CGMainDisplayID()) 
    mainDisplayScreen = screen;
    
	return displayArray[screen];
}

void Cocoa_GetScreenResolutionOfAnotherScreen(int screen, int* w, int* h)
{
  CFDictionaryRef mode = CGDisplayCurrentMode(Cocoa_GetDisplay(screen));
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

void Cocoa_GetScreenResolution(int* w, int* h)
{
  // Figure out the screen size.
  CGDirectDisplayID display_id = kCGDirectMainDisplay;
  CFDictionaryRef mode  = CGDisplayCurrentMode(display_id);
  
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

// get a double value from a dictionary
static double getDictDouble(CFDictionaryRef refDict, CFStringRef key)
{
  double double_value;
  CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
  if (!number_value) // if can't get a number for the dictionary
    return -1;  // fail
  if (!CFNumberGetValue(number_value, kCFNumberDoubleType, &double_value)) // or if cant convert it
    return -1; // fail
  return double_value; // otherwise return the long value
}

double Cocoa_GetScreenRefreshRate(int screen)
{
  // Figure out the refresh rate.
  CFDictionaryRef mode = CGDisplayCurrentMode(Cocoa_GetDisplay(screen));
  return (mode != NULL) ? getDictDouble(mode, kCGDisplayRefreshRate) : 0.0f;
}

void QZ_ChangeWindowSize(int w, int h);

static NSView* windowedView = 0;

void Cocoa_MoveWindowToDisplay(int screen)
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  NSWindow* window = [view window];
  
  NSScreen* pScreen = [[NSScreen screens] objectAtIndex:screen];
  [window disableScreenUpdatesUntilFlush];
  [window setFrameOrigin:[pScreen frame].origin];
  [window center];
  [window flushWindow];
}

void* Cocoa_GL_ResizeWindow(void *theContext, int w, int h)
{
  if (!theContext)
    return 0;
  
  NSOpenGLContext* context = Cocoa_GL_GetCurrentContext();
  NSView* view;
  NSWindow* window;
  
  view = [context view];
  
  // If we're moving to full-screen, we've probably changed contexts already,
  // so it's better to grab the view from the saved one.
  //
  if (windowedView != 0)
  {
    // First, resize that view, though.
    if (view)
    {
       window = [view window];

      [window setContentSize:NSMakeSize(w, h)];
      [window update];
      [view setFrameSize:NSMakeSize(w, h)];
      [context update];
      [window center];
    }
  
    view = windowedView;
    window = [view window];
  }
  
  if (view && w>0 && h>0)
  {
    window = [view window];
    if (window)
    {
      [window setContentSize:NSMakeSize(w, h)];
      [window update];
      [view setFrameSize:NSMakeSize(w, h)];
      [context update];
      [window center];
    }
  }
  
  // HACK to make sure SDL realizes the window size changed to help with mouse pointers.
  QZ_ChangeWindowSize(w, h);  

  return context;
}

void Cocoa_GL_BlankOtherDisplays(int screen)
{
  int numDisplays = [[NSScreen screens] count];
  int i = 0;

  // Blank.
  for (i=0; i<numDisplays; i++)
  {
    if (i != screen && blankingWindows[i] == 0)
    {
      // Get the size.
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:i];
      NSRect    screenRect = [pScreen frame];
          
      // Build a blanking window.
      screenRect.origin = NSZeroPoint;
      blankingWindows[i] = [[NSWindow alloc] initWithContentRect:screenRect
                                                      styleMask:NSBorderlessWindowMask
                                                        backing:NSBackingStoreBuffered
                                                          defer:NO 
                                                         screen:pScreen];
                                            
      [blankingWindows[i] setBackgroundColor:[NSColor blackColor]];
      [blankingWindows[i] setLevel:CGShieldingWindowLevel()];
      [blankingWindows[i] makeKeyAndOrderFront:nil];
      
      // Get the current backlight level
      float panelBrightness = HUGE_VALF;
      CGDisplayErr dErr;
      dErr = IODisplayGetFloatParameter(CGDisplayIOServicePort(i), kNilOptions, CFSTR(kIODisplayBrightnessKey), &panelBrightness);
      if (dErr == kIOReturnSuccess)
      {
        blankingBrightness[i] = panelBrightness;
        IODisplaySetFloatParameter(CGDisplayIOServicePort(i), kNilOptions, CFSTR(kIODisplayBrightnessKey), 0.0f);
      }
      else
        blankingBrightness[i] = -1.0f;
      
      /*
      Cocoa_GetPanelBrightness(&unblankBrigtnessLevel);
      if (unblankBrigtnessLevel > -1.0f)
      {
        IODisplaySetFloatParameter(CGDisplayIOServicePort(CGMainDisplayID()), kNilOptions, CFSTR(kIODisplayBrightnessKey), 0.0f);
        mainDisplayBlanked = true;
      }*/
    }
  } 
}

void Cocoa_GL_UnblankOtherDisplays(int screen)
{
  int numDisplays = [[NSScreen screens] count];
  int i = 0;

  for (i=0; i<numDisplays; i++)
  {
    if (blankingWindows[i] != 0)
    {
      // Get rid of the blanking window we created.
      [blankingWindows[i] close];
      [blankingWindows[i] release];
      blankingWindows[i] = 0;
    }
    if (blankingBrightness[i] >= 0.0f)
    {
      // Restore the backlight brightness on supported screens
      IODisplaySetFloatParameter(CGDisplayIOServicePort(i), kNilOptions, CFSTR(kIODisplayBrightnessKey), blankingBrightness[i]);
    }
  }
}

static NSOpenGLContext* lastOwnedContext = 0;
static NSWindow* lastWindow = NULL;

void Cocoa_GL_SetFullScreen(int screen, int width, int height, bool fs, bool blankOtherDisplays, bool fakeFullScreen)
{
  static NSView* lastView = NULL;
  static int fullScreenDisplay = 0;
  static int lastDisplay = -1;

  // If we're already fullscreen then we must be moving to a different display.
  // Recurse to reset fullscreen mode and then continue.
  //
  if (fs == true && lastDisplay != -1)
    Cocoa_GL_SetFullScreen(0, 0, 0, false, blankOtherDisplays, fakeFullScreen);
  
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  
  // Let's whack the child window if there is one.
  Cocoa_DestroyChildWindow();
  
  if (!context)
    return;
  
  if (fs)
  {
    NSScreen* pScreen = [[NSScreen screens] objectAtIndex:screen];
    NSOpenGLContext* newContext = NULL;
  
    // Fade to black to hide resolution-switching flicker and garbage.
  	CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
      CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);

    // Save these values.
    lastView = [context view];
    lastDisplay = screen;
    windowedView = lastView;
    
    if (fakeFullScreen == false)
    {
      // obtain fullscreen pixel format
      NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetFullScreenPixelFormat(screen);
      if (!pixFmt)
        return;
      
      // create our new context (sharing with the current one)
      newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*) pixFmt, (void*)context);
      
      // release pixelformat
      [pixFmt release];
      pixFmt = nil;
      
      if (!newContext)
        return;
      
      // Make sure the view is on the screen that we're activating (to hide it).
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:screen];
      [[lastView window] setFrameOrigin:[pScreen frame].origin];
      
      // clear the current context
      [NSOpenGLContext clearCurrentContext];
              
      // set fullscreen
      [newContext setFullScreen];
      
      // Capture the display before going fullscreen.
      fullScreenDisplay = Cocoa_GetDisplay(screen);
      if (blankOtherDisplays == true)
        CGCaptureAllDisplays();
      else
        CGDisplayCapture(fullScreenDisplay);

      // If we don't hide menu bar, it will get events and interrupt the program.
      if (fullScreenDisplay == kCGDirectMainDisplay)
        HideMenuBar();
    }
    else
    {
      // Get the screen rect of our main display
      NSRect    screenRect = [pScreen frame];
      NSWindow* mainWindow = [[NSWindow alloc] initWithContentRect:screenRect
                                              styleMask:NSBorderlessWindowMask
                                              backing:NSBackingStoreBuffered
                                              defer:NO 
                                              screen:pScreen];
                                              
      [mainWindow setBackgroundColor:[NSColor blackColor]];
      [mainWindow makeKeyAndOrderFront:nil];
      
      // Display our window fairly high...
      [mainWindow setLevel:NSFloatingWindowLevel];
      
      // ...and the original one beneath it and on the same screen.
      [[lastView window] setFrameOrigin:[pScreen frame].origin];
      [[lastView window] orderWindow:NSWindowBelow relativeTo:[mainWindow windowNumber]];
          
      NSView* blankView = [[NSView alloc] init];
      [mainWindow setContentView:blankView];
      [mainWindow setContentSize:NSMakeSize(width, height)];
      [mainWindow update];
      [blankView setFrameSize:NSMakeSize(width, height)];
      
      // Obtain windowed pixel format and create a new context.
      NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
      newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void* )pixFmt, (void* )context);
      [pixFmt release];
      
      [newContext setView:blankView];
      
      // Hide the menu bar.
      fullScreenDisplay = Cocoa_GetDisplay(screen);
      if (fullScreenDisplay == kCGDirectMainDisplay)
        HideMenuBar();
          
      // Save the window.
      lastWindow = mainWindow;
      
      // Blank other displays if requested.
      if (blankOtherDisplays)
        Cocoa_GL_BlankOtherDisplays(screen);
    }

    // Hide the mouse.
    [NSCursor hide];
    
    // Release old context if we created it.
    if (lastOwnedContext == context)
      Cocoa_GL_ReleaseContext((void*)context);

    // activate context
    [newContext makeCurrentContext];
    lastOwnedContext = newContext;
    
    if (fade_token != kCGDisplayFadeReservationInvalidToken) 
    {
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
      CGReleaseDisplayFadeReservation(fade_token);
    }
  }
  else
  {
  	// Fade to black to hide resolution-switching flicker and garbage.
  	CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
      CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);
    
    // exit fullscreen
    [context clearDrawable];
    
    [NSCursor unhide];
    
    if (fakeFullScreen == false)
    {
      if (fullScreenDisplay == kCGDirectMainDisplay)
        ShowMenuBar();
      
      // release displays
      CGReleaseAllDisplays();
    }
    else
    {
      // Show menubar.
      if (fullScreenDisplay == kCGDirectMainDisplay)
        ShowMenuBar();

      // Get rid of the new window we created.
      [lastWindow close];
      [lastWindow release];
      
      // Unblank.
      if (blankOtherDisplays)
        Cocoa_GL_UnblankOtherDisplays(screen);
    }
    
    // obtain windowed pixel format
    NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
    if (!pixFmt)
      return;
    
    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void* )pixFmt, (void* )context);
    
    // release pixelformat
    [pixFmt release];
    pixFmt = nil;
    
    if (!newContext)
      return;
    
    // Assign view from old context, move back to original screen.
    NSScreen* screen = [[NSScreen screens] objectAtIndex:lastDisplay];
    [newContext setView:lastView];
    [[lastView window] setFrameOrigin:[screen frame].origin];
    
    // Release the fullscreen context.
    if (lastOwnedContext == context)
      Cocoa_GL_ReleaseContext((void*)context);
    
    // Activate context.
    [newContext makeCurrentContext];
    lastOwnedContext = newContext;
    
    if (fade_token != kCGDisplayFadeReservationInvalidToken) 
    {
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
      CGReleaseDisplayFadeReservation(fade_token);
    }
    
    // Reset.
    lastView = NULL;
    lastDisplay = -1;
    fullScreenDisplay = 0;
    windowedView = 0;
  }
}

int Cocoa_GetCurrentDisplay()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  NSScreen* screen = [[view window] screen];
  
  int numDisplays = [[NSScreen screens] count];
  int i = 0;
  
  for (i=0; i<numDisplays; i++)
  {
    if ([[NSScreen screens] objectAtIndex:i] == screen)
      return i;
  }
  
  return -1;
}

void Cocoa_GL_EnableVSync(bool enable)
{
#if 1
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  
  // Flush synchronised with vertical retrace                       
  GLint theOpenGLCPSwapInterval = enable ? 1 : 0;
  [context setValues:&theOpenGLCPSwapInterval forParameter:NSOpenGLCPSwapInterval];
  
#else

  CGLContextObj cglContext;
  cglContext = CGLGetCurrentContext();
  if (cglContext)
  {
    GLint interval;
    if (enable)
      interval = 1;
    else
      interval = 0;
    
    int cglErr = CGLSetParameter(cglContext, kCGLCPSwapInterval, &interval);
    if (cglErr != kCGLNoError)
      printf("ERROR: CGLSetParameter for kCGLCPSwapInterval failed with error %d: %s", cglErr, CGLErrorString(cglErr));
  }
#endif
}

void* Cocoa_GL_GetWindowPixelFormat()
{
  NSOpenGLPixelFormatAttribute wattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAWindow,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 8,
    //NSOpenGLPFAColorSize, 32,
    //NSOpenGLPFAAlphaSize, 8,
    0
  };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetFullScreenPixelFormat(int screen)
{
  NSOpenGLPixelFormatAttribute fsattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAFullScreen,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 8,
    NSOpenGLPFAScreenMask,
    CGDisplayIDToOpenGLDisplayMask((CGDirectDisplayID)Cocoa_GetDisplay(screen)),
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

  // Enable GL multithreading if available.                                                           
  //CGLContextObj theCGLContextObj = (CGLContextObj) [newContext CGLContextObj];
  //CGLEnable(theCGLContextObj, kCGLCEMPEngine);

  // Flush synchronised with vertical retrace                       
  GLint theOpenGLCPSwapInterval = 1;
  [newContext setValues:(const GLint*)&theOpenGLCPSwapInterval forParameter:(NSOpenGLContextParameter) NSOpenGLCPSwapInterval];
                                                           
  return newContext;
}

void* Cocoa_GL_ReplaceSDLWindowContext()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  
  if (!view)
    return 0;
  
  // disassociate view from context
  [context clearDrawable];
  
  // release the context
  if (lastOwnedContext == context)
    Cocoa_GL_ReleaseContext((void*)context);
  
  // obtain window pixelformat
  NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
  if (!pixFmt)
    return 0;
  
  NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*)pixFmt, nil);
  [pixFmt release];
  
  if (!newContext)
    return 0;
  
  // associate with current view
  [newContext setView:view];
  [newContext makeCurrentContext];
  lastOwnedContext = newContext;
  
  return newContext;
}

int Cocoa_DimDisplayNow()
{
  io_registry_entry_t r = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/IOResources/IODisplayWrangler");
  if(!r) return 1;
  int err = IORegistryEntrySetCFProperty(r, CFSTR("IORequestIdle"), kCFBooleanTrue);
  IOObjectRelease(r);
  return err;
}

void Cocoa_UpdateSystemActivity()
{
  UpdateSystemActivity(UsrActivity);   
}

void Cocoa_TurnOffScreenSaver()
{
  int pid = GetProcessPid("ScreenSaverEngin");
  if (pid != -1)
    kill(pid, SIGKILL);
}
                   
int Cocoa_SleepSystem()
{
  io_connect_t root_domain;
  mach_port_t root_port;

  if (KERN_SUCCESS != IOMasterPort(MACH_PORT_NULL, &root_port))
    return 1;

  if (0 == (root_domain = IOPMFindPowerManagement(root_port)))    
    return 2;

  if (kIOReturnSuccess != IOPMSleepSystem(root_domain))
    return 3;

  return 0;
}        

OSStatus SendAppleEventToSystemProcess(AEEventID EventToSend)
{
  AEAddressDesc targetDesc;
  static const ProcessSerialNumber kPSNOfSystemProcess = { 0, kSystemProcess };
  AppleEvent eventReply = {typeNull, NULL};
  AppleEvent appleEventToSend = {typeNull, NULL};
  
  OSStatus error = noErr;
  
  error = AECreateDesc(typeProcessSerialNumber, &kPSNOfSystemProcess, 
                       sizeof(kPSNOfSystemProcess), &targetDesc);
  
  if (error != noErr)
  {
    return(error);
  }
  
  error = AECreateAppleEvent(kCoreEventClass, EventToSend, &targetDesc, 
                             kAutoGenerateReturnID, kAnyTransactionID, &appleEventToSend);
  
  AEDisposeDesc(&targetDesc);
  if (error != noErr)
  {
    return(error);
  }
  
  error = AESend(&appleEventToSend, &eventReply, kAENoReply, 
                 kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
  
  AEDisposeDesc(&appleEventToSend);
  if (error != noErr)
  {
    return(error);
  }
  
  AEDisposeDesc(&eventReply);
  
  return(error); 
}

bool Cocoa_ShutDownSystem()
{
  OSStatus error = noErr;
  error = SendAppleEventToSystemProcess(kAEShutDown);
  return (error == noErr);
}

void Cocoa_HideMouse()
{
  [NSCursor hide];
}

void Cocoa_GetSmartFolderResults(const char* strFile, void (*CallbackFunc)(void* userData, void* userData2, const char* path), void* userData, void* userData2)
{
  NSString*     filePath = [[NSString alloc] initWithUTF8String:strFile];
  NSDictionary* doc = [[NSDictionary alloc] initWithContentsOfFile:filePath];
  NSString*     raw = [doc objectForKey:@"RawQuery"];
  NSArray*      searchPaths = [[doc objectForKey:@"SearchCriteria"] objectForKey:@"FXScopeArrayOfPaths"];

  if (raw == 0)
    return;

  // Ugh, Carbon from now on...
  MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, (CFStringRef)raw, NULL, NULL);
  if (query)
  {
  	if (searchPaths)
  	  MDQuerySetSearchScope(query, (CFArrayRef)searchPaths, 0);
  	  
    MDQueryExecute(query, 0);

	// Keep track of when we started.
	CFAbsoluteTime startTime = CFAbsoluteTimeGetCurrent(); 
    for (;;)
    {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, YES);
    
      // If we're done or we timed out.
      if (MDQueryIsGatheringComplete(query) == true ||
      	  CFAbsoluteTimeGetCurrent() - startTime >= 5)
      {
        // Stop the query.
        MDQueryStop(query);
      
    	CFIndex count = MDQueryGetResultCount(query);
    	char title[BUFSIZ];
    	int i;
  
    	for (i = 0; i < count; ++i) 
   		{
      	  MDItemRef resultItem = (MDItemRef)MDQueryGetResultAtIndex(query, i);
      	  CFStringRef titleRef = (CFStringRef)MDItemCopyAttribute(resultItem, kMDItemPath);
      
      	  CFStringGetCString(titleRef, title, BUFSIZ, kCFStringEncodingUTF8);
      	  CallbackFunc(userData, userData2, title);
      	  CFRelease(titleRef);
    	}  
    
        CFRelease(query);
    	break;
      }
    }
  }
  
  // Freeing these causes a crash when scanning for new content.
  CFRelease(filePath);
  CFRelease(doc);
}

const char* Cocoa_GetAppVersion()
{
  return [(NSString*)[[NSBundle mainBundle] objectForInfoDictionaryKey:(id)kCFBundleVersionKey] UTF8String];
}

void* Cocoa_GetDisplayPort()
{
  //NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  //NSView* view = [context view];
  //WindowRef refWindow = [[view window] windowRef];
  //return GetWindowPort(refWindow);
  
  WindowRef refWindow = [childWindow windowRef];
  return (void* )GetWindowPort(refWindow);
}

/* Get/set LCD panel brightness */

void Cocoa_GetPanelBrightness(float* brightness)
{
//  int mainDisplayId = CGMainDisplayID();
  if (blankingWindows[mainDisplayScreen] != 0)
  {
    *brightness = blankingBrightness[mainDisplayScreen];
    return;
  }
  float panelBrightness = HUGE_VALF;
  CGDisplayErr dErr;
  
  dErr = IODisplayGetFloatParameter(CGDisplayIOServicePort(CGMainDisplayID()), kNilOptions, CFSTR(kIODisplayBrightnessKey), &panelBrightness);
  if (dErr == kIOReturnSuccess)
    *brightness = panelBrightness;
  else
    *brightness = -1.0f;
}

void Cocoa_SetPanelBrightness(float brightness)
{
  if ((brightness >=0.0f) && (brightness <= 1.0f)) {
    if (blankingWindows[mainDisplayScreen] == 0)
      IODisplaySetFloatParameter(CGDisplayIOServicePort(CGMainDisplayID()), kNilOptions, CFSTR(kIODisplayBrightnessKey), brightness);    
    else
      blankingBrightness[mainDisplayScreen] = brightness;
  }
}

const char* Cocoa_HW_ModelName()
{ return [[[AppleHardwareInfo sharedInstance] modelName] UTF8String]; }

const char* Cocoa_HW_LongModelName()
{ return [[[AppleHardwareInfo sharedInstance] longModelName] UTF8String]; }

bool Cocoa_HW_HasBattery()
{ return [[AppleHardwareInfo sharedInstance] hasBattery]; }

bool Cocoa_HW_IsOnACPower()
{ return [[AppleHardwareInfo sharedInstance] isOnACPower]; }

bool Cocoa_HW_IsCharging()
{ return [[AppleHardwareInfo sharedInstance] isCharging]; }

int  Cocoa_HW_CurrentBatteryCapacity()
{ return [[AppleHardwareInfo sharedInstance] currentBatteryCapacity]; }

int  Cocoa_HW_TimeToBatteryEmpty()
{ return [[AppleHardwareInfo sharedInstance] timeToEmpty]; }

int  Cocoa_HW_TimeToFullCharge()
{ return [[AppleHardwareInfo sharedInstance] timeToFullCharge]; }

void Cocoa_HW_SetBatteryWarningEnabled(bool enabled)
{ [[AppleHardwareInfo sharedInstance] setLowBatteryWarningEnabled:enabled]; }

void Cocoa_HW_SetBatteryTimeWarning(int timeWarning)
{ [AppleHardwareInfo sharedInstance].batteryTimeWarning = timeWarning; }

void Cocoa_HW_SetBatteryCapacityWarning(int capacityWarning)
{ [AppleHardwareInfo sharedInstance].batteryCapacityWarning = capacityWarning; }

void Cocoa_HW_SetKeyboardBacklightEnabled(bool enabled)
{ [[AppleHardwareInfo sharedInstance] setKeyboardBacklightEnabled:enabled]; }

void Cocoa_CheckForUpdates()
{
  [[SUPlexUpdater sharedInstance] checkForUpdatesWithUI:nil];
}

void Cocoa_CheckForUpdatesInBackground()
{
  [[SUPlexUpdater sharedInstance] checkForUpdatesInBackground];
}

void Cocoa_SetUpdateAlertType(int alertType)
{
  [[SUPlexUpdater sharedInstance] setAlertType:alertType];
}

void Cocoa_SetUpdateSuspended(bool willSuspend)
{
  [[SUPlexUpdater sharedInstance] setSuspended:willSuspend];
}

void Cocoa_SetUpdateCheckInterval(double seconds)
{
  [[SUPlexUpdater sharedInstance] setCheckInterval:seconds];
}

void Cocoa_UpdateProgressDialog()
{
  Cocoa_CPPUpdateProgressDialog();
}

/*
 * I was recently looking into the fade effect on OS X which happens when you switch from windowed mode to full screen and vice versa.  I thought it’d be better if it only faded the effected display, so I started looking into a great example from Apple. There was an issue though. After retrieving the current gamma value, and subsequently setting the value (to the value just retrieved!) the display would get brighter. Odd, that should only happen if I turn the gamma down. Well, as it turns out, the CGGetDisplayTransferByFormula function doesn’t actually calculate the current gamma correctly. If you toss the values retrieved from CGGetDisplayTransferByTable into excel and have it calculate the value for you, you’ll see that it doesn’t match up. The solution was to just throw out the value and calculate it in code. Thanks Apple, you just wasted my week.

For those interested (pay attention Apple!), the following code will do the trick. Note that M_LN2 is the natural log of 2, defined in math.h.
 CGGetDisplayTransferByFormula(display,
    &redMin, &redMax, &redGamma,
    &greenMin, &greenMax, &greenGamma,
    &blueMin, &blueMax, &blueGamma);

CGGetDisplayTransferByTable(display, 3, redTable, greenTable, blueTable, &sampleCount);

redGamma = (CGGammaValue)(log(redTable[2] / redTable[1]) / M_LN2);
greenGamma = (CGGammaValue)(log(greenTable[2] / greenTable[1]) / M_LN2);
blueGamma = (CGGammaValue)(log(blueTable[2] / blueTable[1]) / M_LN2);

 */
void Cocoa_SetGammaRamp(unsigned short* pRed, unsigned short* pGreen, unsigned short* pBlue)
{
  const CGTableCount tableSize = 255;
  CGGammaValue redTable[tableSize];
  CGGammaValue greenTable[tableSize];
  CGGammaValue blueTable[tableSize];

  int i;

  // Extract gamma values into separate tables, convert to floats between 0.0 and 1.0.
  for (i = 0; i < 256; i++)
  {
      redTable[i]   = pRed[i]   / 65535.0;
      greenTable[i] = pGreen[i] / 65535.0;
      blueTable[i]  = pBlue[i]  / 65535.0;
  }

  // Look up the current display for Plex and set the gamma ramp.
  int iScreen = Cocoa_GetCurrentDisplay();
  CGDirectDisplayID displayID = (CGDirectDisplayID)Cocoa_GetDisplay(iScreen);
  CGSetDisplayTransferByTable(displayID, tableSize, redTable, greenTable, blueTable);
}

bool Cocoa_Proxy_Enabled(const char* protocol)
{
  NSDictionary* proxyDict = (NSDictionary*)SCDynamicStoreCopyProxies(NULL);
  @try
  {
    NSString* protocolEnabled = [[[NSString stringWithCString:protocol] uppercaseString] stringByAppendingString:@"Enable"];
    return ([[proxyDict objectForKey:protocolEnabled] boolValue]);
  }
  @finally
  {
    [proxyDict release];
  }
  return false;
}

#endif // WORKING

const char* Cocoa_Proxy_Host(const char* protocol)
{
  NSDictionary* proxyDict = (NSDictionary*)SCDynamicStoreCopyProxies(NULL);
  @try
  {
    NSString* protocolProxy = [[[NSString stringWithCString:protocol] uppercaseString] stringByAppendingString:@"Proxy"];
    NSHost* host = [NSHost hostWithName:(NSString*)[proxyDict objectForKey:protocolProxy]];
    if (host)
      return [[host address] UTF8String];
  }
  @finally
  {
    [proxyDict release];
  }
  return "";
}

const char* Cocoa_Proxy_Port(const char* protocol)
{
  NSDictionary* proxyDict = (NSDictionary*)SCDynamicStoreCopyProxies(NULL);
  @try
  {
    NSString* protocolPort = [[[NSString stringWithCString:protocol] uppercaseString] stringByAppendingString:@"Port"];
    return [[NSString stringWithFormat:@"%i", [[proxyDict objectForKey:protocolPort] intValue]] UTF8String];
  }
  @finally
  {
    [proxyDict release];
  }
  return "";
}

const char* Cocoa_Proxy_Username(const char* protocol)
{
  return "";
}

const char* Cocoa_Proxy_Password(const char* protocol)
{
  return "";
}

#ifdef WORKING

void Cocoa_LaunchApp(const char* appToLaunch)
{
  NSString* appPath = [NSString stringWithCString:appToLaunch];
  if ([[NSWorkspace sharedWorkspace] launchApplication:appPath])
  {
    // Don't quit when running dashboard
    if ([appPath rangeOfString:@"/Dashboard.app"].location != NSNotFound) return;
    
    // If the application launched successfully, wait until it's running & get the process ID
    int i;
    BOOL isRunning = false;
    NSString* runningPath;
    NSNumber* pid;
    while (!isRunning)
    {
      for (i=0; i < [[[NSWorkspace sharedWorkspace] launchedApplications] count]; i++)
      {
        runningPath = [[[[NSWorkspace sharedWorkspace] launchedApplications] objectAtIndex:i] objectForKey:@"NSApplicationPath"];
        if ([appPath rangeOfString:runningPath].location != NSNotFound)
        {
          isRunning = YES;
          pid = [[[[NSWorkspace sharedWorkspace] launchedApplications] objectAtIndex:i] objectForKey:@"NSApplicationProcessIdentifier"];
        }
      }
      sleep(1);
    }
    
    // Restart Plex when the launched app quits
    [NSTask launchedTaskWithLaunchPath:[[NSBundle mainBundle] pathForResource:@"relaunch" ofType:@""] arguments:[NSArray arrayWithObjects:[[NSBundle mainBundle] bundlePath], [NSString stringWithFormat:@"%d", [pid intValue]], nil]];
    [NSApp terminate:nil];
  }
}

void Cocoa_LaunchAutomatorWorkflow(const char* wflowToLaunch)
{
	NSString* path = [NSString stringWithCString:wflowToLaunch];
	if ([path rangeOfString:@".workflow/"].location == NSNotFound) return;
	[[NSWorkspace sharedWorkspace] openFile:path withApplication:@"Automator Runner"];
	[NSApp terminate:nil];
}

void Cocoa_LaunchFrontRow()
{
  [NSTask launchedTaskWithLaunchPath:[[NSBundle mainBundle] pathForResource:@"frontrowlauncher" ofType:@""] arguments:[NSArray arrayWithObjects:[[NSBundle mainBundle] bundlePath], nil]];    
  [NSApp terminate:nil];    
}
#endif // WORKING
const char* Cocoa_GetAppIcon(const char *applicationPath)
{
  // Check for info.plist inside the bundle
  NSString* appPath = [NSString stringWithCString:applicationPath];
  NSDictionary* appPlist = [NSDictionary dictionaryWithContentsOfFile:[appPath stringByAppendingPathComponent:@"Contents/Info.plist"]];
  if (!appPlist) return NULL;
  
  // Get the path to the target PNG icon
  NSString* pngFile = [[NSString stringWithFormat:@"~/Library/Application Support/Plex/userdata/Thumbnails/Programs/%@.png",
                        [appPlist objectForKey:@"CFBundleIdentifier"]] stringByExpandingTildeInPath];
  
  // If no PNG has been created, open the app's ICNS file & convert
  if (![[NSFileManager defaultManager] fileExistsAtPath:pngFile])
  {
    NSString* iconFile = [appPath stringByAppendingPathComponent:[NSString stringWithFormat:@"/Contents/Resources/%@", [appPlist objectForKey:@"CFBundleIconFile"]]];
    if ([iconFile rangeOfString:@".icns"].location == NSNotFound) iconFile = [iconFile stringByAppendingString:@".icns"];
    NSImage* icon = [[NSImage alloc] initWithContentsOfFile:iconFile];
    if (!icon) return NULL;
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithData:[icon TIFFRepresentation]];
    NSData* png = [rep representationUsingType:NSPNGFileType properties:nil];
    [png writeToFile:pngFile atomically:YES];
    [png release];
    [rep release];
    [icon release];
  }
  return [pngFile UTF8String];
}
bool Cocoa_IsAppBundle(const char* filePath)
{
  if (filePath == 0)
    return false;
  
  NSString* appPath = [NSString stringWithUTF8String:filePath];
  NSFileManager* fm = [NSFileManager defaultManager];
  return (([appPath rangeOfString:@".app"].location != NSNotFound) &&
          [fm fileExistsAtPath:appPath] &&
          [fm fileExistsAtPath:[appPath stringByAppendingPathComponent:@"/Contents/Info.plist"]] &&
          [fm fileExistsAtPath:[appPath stringByAppendingPathComponent:@"/Contents/MacOS"]]);
                                           
                                           
}

bool Cocoa_IsWflowBundle(const char* filePath)
{
  if (filePath == 0)
    return false;
  
  NSString* appPath = [NSString stringWithUTF8String:filePath];
  NSFileManager* fm = [NSFileManager defaultManager];
  return (([appPath rangeOfString:@".workflow"].location != NSNotFound) &&
          [fm fileExistsAtPath:appPath] &&
          [fm fileExistsAtPath:[appPath stringByAppendingPathComponent:@"/Contents/document.wflow"]]);
}
#ifdef WORKING
const char* Cocoa_GetIconFromBundle(const char *_bundlePath, const char* _iconName)
{
  NSString* bundlePath = [NSString stringWithCString:_bundlePath];
	NSString* iconName = [NSString stringWithCString:_iconName];
	NSBundle* bundle = [NSBundle bundleWithPath:bundlePath];
	NSString* iconPath = [bundle pathForResource:iconName ofType:@"icns"];
	NSString* bundleIdentifier = [bundle bundleIdentifier];

	if (![[NSFileManager defaultManager] fileExistsAtPath:iconPath]) return NULL;

  // Get the path to the target PNG icon
  NSString* pngFile = [[NSString stringWithFormat:@"~/Library/Application Support/Plex/userdata/Thumbnails/%@-%@.png",
                        bundleIdentifier, iconName] stringByExpandingTildeInPath];

  // If no PNG has been created, open the ICNS file & convert
  if (![[NSFileManager defaultManager] fileExistsAtPath:pngFile])
  {
    NSImage* icon = [[NSImage alloc] initWithContentsOfFile:iconPath];
    if (!icon) return NULL;
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithData:[icon TIFFRepresentation]];
    NSData* png = [rep representationUsingType:NSPNGFileType properties:nil];
    [png writeToFile:pngFile atomically:YES];
    [png release];
    [rep release];
    [icon release];
  }
  return [pngFile UTF8String];
}

void Cocoa_ExecAppleScriptFile(const char* filePath)
{
	NSString* scriptFile = [NSString stringWithUTF8String:filePath];
	NSString* userScriptsPath = [@"~/Library/Application Support/Plex/scripts" stringByExpandingTildeInPath];
	NSString* bundleScriptsPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"Contents/Resources/Plex/scripts"];
	NSString* bundleSysScriptsPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"Contents/Resources/Plex/system/AppleScripts"];

	// Check whether a script exists in the app bundle's AppleScripts folder
	if ([[NSFileManager defaultManager] fileExistsAtPath:[bundleSysScriptsPath stringByAppendingPathComponent:scriptFile]])
		scriptFile = [bundleSysScriptsPath stringByAppendingPathComponent:scriptFile];

	// Check whether a script exists in app support
	else if ([[NSFileManager defaultManager] fileExistsAtPath:[userScriptsPath stringByAppendingPathComponent:scriptFile]]) // Check whether a script exists in the app bundle
		scriptFile = [userScriptsPath stringByAppendingPathComponent:scriptFile];

	// Check whether a script exists in the app bundle's Scripts folder
	else if ([[NSFileManager defaultManager] fileExistsAtPath:[bundleScriptsPath stringByAppendingPathComponent:scriptFile]])
		scriptFile = [bundleScriptsPath stringByAppendingPathComponent:scriptFile];

	// If no script could be found, check if we were given a full path
	else if (![[NSFileManager defaultManager] fileExistsAtPath:scriptFile])
		return;

	NSAppleScript* appleScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:scriptFile] error:nil];
	[appleScript executeAndReturnError:nil];
	[appleScript release];
}

void Cocoa_ExecAppleScript(const char* scriptSource)
{
	NSAppleScript* appleScript = [[NSAppleScript alloc] initWithSource:[NSString stringWithUTF8String:scriptSource]];
	[appleScript executeAndReturnError:nil];
	[appleScript release];
}

bool Cocoa_IsGUIShowing()
{
  return ([[AdvancedSettingsController sharedInstance] windowIsVisible] || [[PlexApplication sharedInstance] isAboutWindowVisible] || [[PlexCompatibility sharedInstance] userInterfaceVisible]);
}

void Cocoa_SetKeyboardBacklightControlEnabled(bool enabled)
{
  [[AppleHardwareInfo sharedInstance] setKeyboardBacklightControlEnabled:enabled];
}

void Cocoa_StopLookingForRemotePlexSources()
{
  [[XBMCMain sharedInstance] stopSearchingForPlexMediaServers];
}

void Cocoa_RemoveAllRemotePlexSources()
{
  [[XBMCMain sharedInstance] removeAllRemotePlexSources];
}

void CheckOSCompatibility()
{
  [[PlexCompatibility sharedInstance] checkCompatibility];
}

bool isSnowLeopardOrBetter()
{
	SInt32 MacVersion;
	
	if (Gestalt(gestaltSystemVersion, &MacVersion) == noErr)
	{
		return (MacVersion >= 0x1063);
	}
	return false;
}

#endif

