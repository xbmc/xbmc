/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#if defined(TARGET_DARWIN_OSX)

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL
#include "WinSystemOSX.h"
#include "WinEventsOSX.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "CompileInfo.h"
#include "guilib/DispResource.h"
#include "guilib/GUIWindowManager.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/DisplaySettings.h"
#include "input/KeyboardStat.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "osx/XBMCHelper.h"
#include "utils/SystemInfo.h"
#include "osx/CocoaInterface.h"
#include "osx/DarwinUtils.h"
#undef BOOL

#import <SDL/SDL_video.h>
#import <SDL/SDL_events.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import "osx/OSXTextInputResponder.h"

// turn off deprecated warning spew.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

//------------------------------------------------------------------------------------------
// special object-c class for handling the inhibit display NSTimer callback.
@interface windowInhibitScreenSaverClass : NSObject
- (void) updateSystemActivity: (NSTimer*)timer;
@end

@implementation windowInhibitScreenSaverClass
-(void) updateSystemActivity: (NSTimer*)timer
{
  UpdateSystemActivity(UsrActivity);
}
@end

//------------------------------------------------------------------------------------------
// special object-c class for handling the NSWindowDidMoveNotification callback.
@interface windowDidMoveNoteClass : NSObject
{
  void *m_userdata;
}
+ initWith: (void*) userdata;
-  (void) windowDidMoveNotification:(NSNotification*) note;
@end

@implementation windowDidMoveNoteClass
+ initWith: (void*) userdata;
{
    windowDidMoveNoteClass *windowDidMove = [windowDidMoveNoteClass new];
    windowDidMove->m_userdata = userdata;
    return [windowDidMove autorelease];
}
-  (void) windowDidMoveNotification:(NSNotification*) note;
{
  CWinSystemOSX *winsys = (CWinSystemOSX*)m_userdata;
	if (!winsys)
    return;

  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  if (context)
  {
    if ([context view])
    {
      NSPoint window_origin = [[[context view] window] frame].origin;
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(newEvent));
      newEvent.type = XBMC_VIDEOMOVE;
      newEvent.move.x = window_origin.x;
      newEvent.move.y = window_origin.y;
      g_application.OnEvent(newEvent);
    }
  }
}
@end
//------------------------------------------------------------------------------------------
// special object-c class for handling the NSWindowDidReSizeNotification callback.
@interface windowDidReSizeNoteClass : NSObject
{
  void *m_userdata;
}
+ initWith: (void*) userdata;
- (void) windowDidReSizeNotification:(NSNotification*) note;
@end
@implementation windowDidReSizeNoteClass
+ initWith: (void*) userdata;
{
    windowDidReSizeNoteClass *windowDidReSize = [windowDidReSizeNoteClass new];
    windowDidReSize->m_userdata = userdata;
    return [windowDidReSize autorelease];
}
- (void) windowDidReSizeNotification:(NSNotification*) note;
{
  CWinSystemOSX *winsys = (CWinSystemOSX*)m_userdata;
	if (!winsys)
    return;
  /* placeholder, do not uncomment or you will SDL recurse into death
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  if (context)
  {
    if ([context view])
    {
      NSSize view_size = [[context view] frame].size;
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(newEvent));
      newEvent.type = XBMC_VIDEORESIZE;
      newEvent.resize.w = view_size.width;
      newEvent.resize.h = view_size.height;
      if (newEvent.resize.w * newEvent.resize.h)
      {
        g_application.OnEvent(newEvent);
        g_windowManager.MarkDirty();
      }
    }
  }
  */
}
@end

//------------------------------------------------------------------------------------------
// special object-c class for handling the NSWindowDidChangeScreenNotification callback.
@interface windowDidChangeScreenNoteClass : NSObject
{
  void *m_userdata;
}
+ initWith: (void*) userdata;
- (void) windowDidChangeScreenNotification:(NSNotification*) note;
@end
@implementation windowDidChangeScreenNoteClass
+ initWith: (void*) userdata;
{
    windowDidChangeScreenNoteClass *windowDidChangeScreen = [windowDidChangeScreenNoteClass new];
    windowDidChangeScreen->m_userdata = userdata;
    return [windowDidChangeScreen autorelease];
}
- (void) windowDidChangeScreenNotification:(NSNotification*) note;
{
  CWinSystemOSX *winsys = (CWinSystemOSX*)m_userdata;
	if (!winsys)
    return;
  winsys->WindowChangedScreen();
}
@end
//------------------------------------------------------------------------------------------


#define MAX_DISPLAYS 32
// if there was a devicelost callback
// but no device reset for 3 secs
// a timeout fires the reset callback
// (for ensuring that e.x. AE isn't stuck)
#define LOST_DEVICE_TIMEOUT_MS 3000
static NSWindow* blankingWindows[MAX_DISPLAYS];

void* CWinSystemOSX::m_lastOwnedContext = 0;

//------------------------------------------------------------------------------------------
CRect CGRectToCRect(CGRect cgrect)
{
  CRect crect = CRect(
    cgrect.origin.x,
    cgrect.origin.y,
    cgrect.origin.x + cgrect.size.width,
    cgrect.origin.y + cgrect.size.height);
  return crect;
}

//------------------------------------------------------------------------------------------
Boolean GetDictionaryBoolean(CFDictionaryRef theDict, const void* key)
{
        // get a boolean from the dictionary
        Boolean value = false;
        CFBooleanRef boolRef;
        boolRef = (CFBooleanRef)CFDictionaryGetValue(theDict, key);
        if (boolRef != NULL)
                value = CFBooleanGetValue(boolRef);
        return value;
}
//------------------------------------------------------------------------------------------
long GetDictionaryLong(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        long value = 0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberLongType, &value);
        return value;
}
//------------------------------------------------------------------------------------------
int GetDictionaryInt(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        int value = 0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberIntType, &value);
        return value;
}
//------------------------------------------------------------------------------------------
float GetDictionaryFloat(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        int value = 0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberFloatType, &value);
        return value;
}
//------------------------------------------------------------------------------------------
double GetDictionaryDouble(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        double value = 0.0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberDoubleType, &value);
        return value;
}

//---------------------------------------------------------------------------------
void SetMenuBarVisible(bool visible)
{
  if(visible)
  {
    [[NSApplication sharedApplication]
      setPresentationOptions:   NSApplicationPresentationDefault];
  }
  else
  {
    [[NSApplication sharedApplication]
      setPresentationOptions:   NSApplicationPresentationHideMenuBar |
                                NSApplicationPresentationHideDock];
  }
}
//---------------------------------------------------------------------------------
CGDirectDisplayID GetDisplayID(int screen_index)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  return(displayArray[screen_index]);
}

CGDirectDisplayID GetDisplayIDFromScreen(NSScreen *screen)
{
  NSDictionary* screenInfo = [screen deviceDescription];
  NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];

  return (CGDirectDisplayID)[screenID longValue];
}

int GetDisplayIndex(CGDirectDisplayID display)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  while (numDisplays > 0)
  {
    if (display == displayArray[--numDisplays])
	  return numDisplays;
  }
  return -1;
}

void BlankOtherDisplays(int screen_index)
{
  int i;
  int numDisplays = [[NSScreen screens] count];

  // zero out blankingWindows for debugging
  for (i=0; i<MAX_DISPLAYS; i++)
  {
    blankingWindows[i] = 0;
  }

  // Blank.
  for (i=0; i<numDisplays; i++)
  {
    if (i != screen_index)
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
    }
  }
}

void UnblankDisplays(void)
{
  int numDisplays = [[NSScreen screens] count];
  int i = 0;

  for (i=0; i<numDisplays; i++)
  {
    if (blankingWindows[i] != 0)
    {
      // Get rid of the blanking windows we created.
      [blankingWindows[i] close];
      if ([blankingWindows[i] isReleasedWhenClosed] == NO)
        [blankingWindows[i] release];
      blankingWindows[i] = 0;
    }
  }
}

CGDisplayFadeReservationToken DisplayFadeToBlack(bool fade)
{
  // Fade to black to hide resolution-switching flicker and garbage.
  CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
  if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess && fade)
    CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);

  return(fade_token);
}

void DisplayFadeFromBlack(CGDisplayFadeReservationToken fade_token, bool fade)
{
  if (fade_token != kCGDisplayFadeReservationInvalidToken)
  {
    if (fade)
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
    CGReleaseDisplayFadeReservation(fade_token);
  }
}

NSString* screenNameForDisplay(CGDirectDisplayID displayID)
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  NSString *screenName = nil;

  NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(CGDisplayIOServicePort(displayID), kIODisplayOnlyPreferredName);
  NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];

  if ([localizedNames count] > 0) {
      screenName = [[localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]] retain];
  }

  [deviceInfo release];
  [pool release];

  return [screenName autorelease];
}

void ShowHideNSWindow(NSWindow *wind, bool show)
{
  if (show)
    [wind orderFront:nil];
  else
    [wind orderOut:nil];
}

static NSWindow *curtainWindow;
void fadeInDisplay(NSScreen *theScreen, double fadeTime)
{
  int     fadeSteps     = 100;
  double  fadeInterval  = (fadeTime / (double) fadeSteps);

  if (curtainWindow != nil)
  {
    for (int step = 0; step < fadeSteps; step++)
    {
      double fade = 1.0 - (step * fadeInterval);
      [curtainWindow setAlphaValue:fade];

      NSDate *nextDate = [NSDate dateWithTimeIntervalSinceNow:fadeInterval];
      [[NSRunLoop currentRunLoop] runUntilDate:nextDate];
    }
  }
  [curtainWindow close];
  curtainWindow = nil;

  [NSCursor unhide];
}

void fadeOutDisplay(NSScreen *theScreen, double fadeTime)
{
  int     fadeSteps     = 100;
  double  fadeInterval  = (fadeTime / (double) fadeSteps);

  [NSCursor hide];

  curtainWindow = [[NSWindow alloc]
    initWithContentRect:[theScreen frame]
    styleMask:NSBorderlessWindowMask
    backing:NSBackingStoreBuffered
    defer:YES
    screen:theScreen];

  [curtainWindow setAlphaValue:0.0];
  [curtainWindow setBackgroundColor:[NSColor blackColor]];
  [curtainWindow setLevel:NSScreenSaverWindowLevel];

  [curtainWindow makeKeyAndOrderFront:nil];
  [curtainWindow setFrame:[curtainWindow
    frameRectForContentRect:[theScreen frame]]
    display:YES
    animate:NO];

  for (int step = 0; step < fadeSteps; step++)
  {
    double fade = step * fadeInterval;
    [curtainWindow setAlphaValue:fade];

    NSDate *nextDate = [NSDate dateWithTimeIntervalSinceNow:fadeInterval];
    [[NSRunLoop currentRunLoop] runUntilDate:nextDate];
  }
}

// try to find mode that matches the desired size, refreshrate
// non interlaced, nonstretched, safe for hardware
CFDictionaryRef GetMode(int width, int height, double refreshrate, int screenIdx)
{
  if ( screenIdx >= (signed)[[NSScreen screens] count])
    return NULL;

  Boolean stretched;
  Boolean interlaced;
  Boolean safeForHardware;
  Boolean televisionoutput;
  int w, h, bitsperpixel;
  double rate;
  RESOLUTION_INFO res;

  CLog::Log(LOGDEBUG, "GetMode looking for suitable mode with %d x %d @ %f Hz on display %d\n", width, height, refreshrate, screenIdx);

  CFArrayRef displayModes = CGDisplayAvailableModes(GetDisplayID(screenIdx));

  if (NULL == displayModes)
  {
    CLog::Log(LOGERROR, "GetMode - no displaymodes found!");
    return NULL;
  }

  for (int i=0; i < CFArrayGetCount(displayModes); ++i)
  {
    CFDictionaryRef displayMode = (CFDictionaryRef)CFArrayGetValueAtIndex(displayModes, i);

    stretched = GetDictionaryBoolean(displayMode, kCGDisplayModeIsStretched);
    interlaced = GetDictionaryBoolean(displayMode, kCGDisplayModeIsInterlaced);
    bitsperpixel = GetDictionaryInt(displayMode, kCGDisplayBitsPerPixel);
    safeForHardware = GetDictionaryBoolean(displayMode, kCGDisplayModeIsSafeForHardware);
    televisionoutput = GetDictionaryBoolean(displayMode, kCGDisplayModeIsTelevisionOutput);
    w = GetDictionaryInt(displayMode, kCGDisplayWidth);
    h = GetDictionaryInt(displayMode, kCGDisplayHeight);
    rate = GetDictionaryDouble(displayMode, kCGDisplayRefreshRate);


    if ((bitsperpixel == 32)      &&
        (safeForHardware == YES)  &&
        (stretched == NO)         &&
        (interlaced == NO)        &&
        (w == width)              &&
        (h == height)             &&
        (rate == refreshrate || rate == 0))
    {
      CLog::Log(LOGDEBUG, "GetMode found a match!");
      return displayMode;
    }
  }
  CLog::Log(LOGERROR, "GetMode - no match found!");
  return NULL;
}

//---------------------------------------------------------------------------------
static void DisplayReconfigured(CGDirectDisplayID display,
  CGDisplayChangeSummaryFlags flags, void* userData)
{
  CWinSystemOSX *winsys = (CWinSystemOSX*)userData;
  if (!winsys)
    return;

  CLog::Log(LOGDEBUG, "CWinSystemOSX::DisplayReconfigured with flags %d", flags);

  // we fire the callbacks on start of configuration
  // or when the mode set was finished
  // or when we are called with flags == 0 (which is undocumented but seems to happen
  // on some macs - we treat it as device reset)

  // first check if we need to call OnLostDevice
  if (flags & kCGDisplayBeginConfigurationFlag)
  {
    // pre/post-reconfiguration changes
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    if (res == RES_INVALID)
      return;

    NSScreen* pScreen = nil;
    unsigned int screenIdx = CDisplaySettings::Get().GetResolutionInfo(res).iScreen;

    if ( screenIdx < [[NSScreen screens] count] )
    {
        pScreen = [[NSScreen screens] objectAtIndex:screenIdx];
    }

    // kCGDisplayBeginConfigurationFlag is only fired while the screen is still
    // valid
    if (pScreen)
    {
      CGDirectDisplayID xbmc_display = GetDisplayIDFromScreen(pScreen);
      if (xbmc_display == display)
      {
        // we only respond to changes on the display we are running on.
        winsys->AnnounceOnLostDevice();
        winsys->StartLostDeviceTimer();
      }
    }
  }
  else // the else case checks if we need to call OnResetDevice
  {
    // we fire if kCGDisplaySetModeFlag is set or if flags == 0
    // (which is undocumented but seems to happen
    // on some macs - we treat it as device reset)
    // we also don't check the screen here as we might not even have
    // one anymore (e.x. when tv is turned off)
    if (flags & kCGDisplaySetModeFlag || flags == 0)
    {
      winsys->StopLostDeviceTimer(); // no need to timeout - we've got the callback
      winsys->AnnounceOnResetDevice();
    }
  }
  
  if ((flags & kCGDisplayAddFlag) || (flags & kCGDisplayRemoveFlag))
    winsys->UpdateResolutions();
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
CWinSystemOSX::CWinSystemOSX() : CWinSystemBase(), m_lostDeviceTimer(this)
{
  m_eWindowSystem = WINDOW_SYSTEM_OSX;
  m_glContext = 0;
  m_SDLSurface = NULL;
  m_osx_events = NULL;
  m_obscured   = false;
  m_obscured_timecheck = XbmcThreads::SystemClockMillis() + 1000;
  m_use_system_screensaver = true;
  // check runtime, we only allow this on 10.5+
  m_can_display_switch = (floor(NSAppKitVersionNumber) >= 949);
  m_lastDisplayNr = -1;
  m_movedToOtherScreen = false;
  m_refreshRate = 0.0;
}

CWinSystemOSX::~CWinSystemOSX()
{
};

void CWinSystemOSX::StartLostDeviceTimer()
{
  if (m_lostDeviceTimer.IsRunning())
    m_lostDeviceTimer.Restart();
  else
    m_lostDeviceTimer.Start(LOST_DEVICE_TIMEOUT_MS, false);
}

void CWinSystemOSX::StopLostDeviceTimer()
{
  m_lostDeviceTimer.Stop();
}

void CWinSystemOSX::OnTimeout()
{
  AnnounceOnResetDevice();
}

bool CWinSystemOSX::InitWindowSystem()
{
  SDL_EnableUNICODE(1);

  // set repeat to 10ms to ensure repeat time < frame time
  // so that hold times can be reliably detected
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  m_osx_events = new CWinEventsOSX();

  if (m_can_display_switch)
    CGDisplayRegisterReconfigurationCallback(DisplayReconfigured, (void*)this);

  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  windowDidMoveNoteClass *windowDidMove;
  windowDidMove = [windowDidMoveNoteClass initWith: this];
  [center addObserver:windowDidMove
    selector:@selector(windowDidMoveNotification:)
    name:NSWindowDidMoveNotification object:nil];
  m_windowDidMove = windowDidMove;


  windowDidReSizeNoteClass *windowDidReSize;
  windowDidReSize = [windowDidReSizeNoteClass initWith: this];
  [center addObserver:windowDidReSize
    selector:@selector(windowDidReSizeNotification:)
    name:NSWindowDidResizeNotification object:nil];
  m_windowDidReSize = windowDidReSize;

  windowDidChangeScreenNoteClass *windowDidChangeScreen;
  windowDidChangeScreen = [windowDidChangeScreenNoteClass initWith: this];
  [center addObserver:windowDidChangeScreen
    selector:@selector(windowDidChangeScreenNotification:)
    name:NSWindowDidChangeScreenNotification object:nil];
  m_windowChangedScreen = windowDidChangeScreen;

  return true;
}

bool CWinSystemOSX::DestroyWindowSystem()
{
  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  [center removeObserver:(windowDidMoveNoteClass*)m_windowDidMove name:NSWindowDidMoveNotification object:nil];
  [center removeObserver:(windowDidReSizeNoteClass*)m_windowDidReSize name:NSWindowDidResizeNotification object:nil];
  [center removeObserver:(windowDidChangeScreenNoteClass*)m_windowChangedScreen name:NSWindowDidChangeScreenNotification object:nil];  

  if (m_can_display_switch)
    CGDisplayRemoveReconfigurationCallback(DisplayReconfigured, (void*)this);

  delete m_osx_events;
  m_osx_events = NULL;

  UnblankDisplays();
  if (m_glContext)
  {
    NSOpenGLContext* oldContext = (NSOpenGLContext*)m_glContext;
    [oldContext release];
    m_glContext = NULL;
  }
  return true;
}

bool CWinSystemOSX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Enable vertical sync to avoid any tearing.
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

  m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, SDL_OPENGL | SDL_RESIZABLE);
  if (!m_SDLSurface)
    return false;

  // the context SDL creates isn't full screen compatible, so we create new one
  // first, find the current contect and make sure a view is attached
  NSOpenGLContext* cur_context = [NSOpenGLContext currentContext];
  NSView* view = [cur_context view];
  if (!view)
    return false;

  // if we are not starting up windowed, then hide the initial SDL window
  // so we do not see it flash before the fade-out and switch to fullscreen.
  if (CDisplaySettings::Get().GetCurrentResolution() != RES_WINDOW)
    ShowHideNSWindow([view window], false);

  // disassociate view from context
  [cur_context clearDrawable];

  // release the context
  if (m_lastOwnedContext == cur_context)
  {
    [ NSOpenGLContext clearCurrentContext ];
    [ cur_context clearDrawable ];
    [ cur_context release ];
  }

  // create a new context
  NSOpenGLContext* new_context = (NSOpenGLContext*)CreateWindowedContext(nil);
  if (!new_context)
    return false;

  // associate with current view
  [new_context setView:view];
  [new_context makeCurrentContext];

  // set the window title
  NSMutableString *string;
  string = [NSMutableString stringWithUTF8String:CCompileInfo::GetAppName()];
  [string appendString:@" Entertainment Center" ];
  [ [ [new_context view] window] setTitle:string ];

  m_glContext = new_context;
  m_lastOwnedContext = new_context;
  m_bWindowCreated = true;

  // get screen refreshrate - this is needed
  // when we startup in windowed mode and don't run through SetFullScreen
  int dummy;
  m_lastDisplayNr = res.iScreen;
  GetScreenResolution(&dummy, &dummy, &m_refreshRate, GetCurrentScreen());

  return true;
}

bool CWinSystemOSX::DestroyWindow()
{
  return true;
}

extern "C" void SDL_SetWidthHeight(int w, int h);
bool CWinSystemOSX::ResizeWindowInternal(int newWidth, int newHeight, int newLeft, int newTop, void *additional)
{
  bool ret = ResizeWindow(newWidth, newHeight, newLeft, newTop);

  if( CDarwinUtils::IsMavericks() )
  {
    NSView * last_view = (NSView *)additional;
    if (last_view && [last_view window])
    {
      NSWindow* lastWindow = [last_view window];
      [lastWindow setContentSize:NSMakeSize(m_nWidth, m_nHeight)];
      [lastWindow update];
      [last_view setFrameSize:NSMakeSize(m_nWidth, m_nHeight)];
    }
  }
  return ret;
}
bool CWinSystemOSX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (!m_glContext)
    return false;

  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  NSView* view;
  NSWindow* window;

  view = [context view];
  if (view && (newWidth > 0) && (newHeight > 0))
  {
    window = [view window];
    if (window)
    {
      [window setContentSize:NSMakeSize(newWidth, newHeight)];
      [window update];
      [view setFrameSize:NSMakeSize(newWidth, newHeight)];
      [context update];
      // this is needed in case we traverse from fullscreen screen 2
      // to windowed on screen 1 directly where in ScreenChangedNotitication
      // we don't have a window to get the current screen on
      // in that case ResizeWindow is called at a later stage from SetFullScreen(false)
      // and we can grab the correct display number here then
      m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen( [window screen] ));;
    }
  }

  // HACK: resize SDL's view manually so that mouse bounds are correctly updated.
  // there are two parts to this, the internal SDL (current_video->screen) and
  // the cocoa view ( handled in SetFullScreen).
  SDL_SetWidthHeight(newWidth, newHeight);

  [context makeCurrentContext];

  m_nWidth = newWidth;
  m_nHeight = newHeight;
  m_glContext = context;
  g_graphicsContext.SetFPS(m_refreshRate);

  return true;
}

static bool needtoshowme = true;

bool CWinSystemOSX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  static NSWindow* windowedFullScreenwindow = NULL;
  static NSScreen* last_window_screen = NULL;
  static NSPoint last_window_origin;
  static NSView* last_view = NULL;
  static NSSize last_view_size;
  static NSPoint last_view_origin;
  static NSInteger last_window_level = NSNormalWindowLevel;
  bool was_fullscreen = m_bFullScreen;
  NSOpenGLContext* cur_context;
  
  if (m_lastDisplayNr == -1)
    m_lastDisplayNr = res.iScreen;

  // Fade to black to hide resolution-switching flicker and garbage.
  CGDisplayFadeReservationToken fade_token = DisplayFadeToBlack(needtoshowme);

  // If we're already fullscreen then we must be moving to a different display.
  // or if we are still on the same display - it might be only a refreshrate/resolution
  // change request.
  // Recurse to reset fullscreen mode and then continue.
  if (was_fullscreen && fullScreen)
  {
    needtoshowme = false;
    ShowHideNSWindow([last_view window], needtoshowme);
    RESOLUTION_INFO& window = CDisplaySettings::Get().GetResolutionInfo(RES_WINDOW);
    CWinSystemOSX::SetFullScreen(false, window, blankOtherDisplays);
    needtoshowme = true;
  }

  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  cur_context = [NSOpenGLContext currentContext];
  
  //handle resolution/refreshrate switching early here
  if (m_bFullScreen)
  {
    if (m_can_display_switch)
    {
      // switch videomode
      SwitchToVideoMode(res.iWidth, res.iHeight, res.fRefreshRate, res.iScreen);
      m_lastDisplayNr = res.iScreen;
    }
  }

  //no context? done.
  if (!cur_context)
  {
    DisplayFadeFromBlack(fade_token, needtoshowme);
    return false;
  }

  if (windowedFullScreenwindow != NULL)
  {
    [windowedFullScreenwindow close];
    if ([windowedFullScreenwindow isReleasedWhenClosed] == NO)
      [windowedFullScreenwindow release];
    windowedFullScreenwindow = NULL;
  }

  if (m_bFullScreen)
  {
    // FullScreen Mode
    NSOpenGLContext* newContext = NULL;

    // Save info about the windowed context so we can restore it when returning to windowed.
    last_view = [cur_context view];
    last_view_size = [last_view frame].size;
    last_view_origin = [last_view frame].origin;
    last_window_screen = [[last_view window] screen];
    last_window_origin = [[last_view window] frame].origin;
    last_window_level = [[last_view window] level];

    if (CSettings::Get().GetBool("videoscreen.fakefullscreen"))
    {
      // This is Cocca Windowed FullScreen Mode
      // Get the screen rect of our current display
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:res.iScreen];
      NSRect    screenRect = [pScreen frame];

      // remove frame origin offset of orginal display
      screenRect.origin = NSZeroPoint;

      // make a new window to act as the windowedFullScreen
      windowedFullScreenwindow = [[NSWindow alloc] initWithContentRect:screenRect
        styleMask:NSBorderlessWindowMask
        backing:NSBackingStoreBuffered
        defer:NO
        screen:pScreen];

      [windowedFullScreenwindow setBackgroundColor:[NSColor blackColor]];
      [windowedFullScreenwindow makeKeyAndOrderFront:nil];

      // make our window the same level as the rest to enable cmd+tab switching
      [windowedFullScreenwindow setLevel:NSNormalWindowLevel];
      // this will make our window topmost and hide all system messages
      //[windowedFullScreenwindow setLevel:CGShieldingWindowLevel()];

      // ...and the original one beneath it and on the same screen.
      [[last_view window] setLevel:NSNormalWindowLevel-1];
      [[last_view window] setFrameOrigin:[pScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ last_view setFrameOrigin:NSMakePoint(0.0, 0.0)];
      [ last_view setFrameSize:NSMakeSize(m_nWidth, m_nHeight) ];

      NSView* blankView = [[NSView alloc] init];
      [windowedFullScreenwindow setContentView:blankView];
      [windowedFullScreenwindow setContentSize:NSMakeSize(m_nWidth, m_nHeight)];
      [windowedFullScreenwindow update];
      [blankView setFrameSize:NSMakeSize(m_nWidth, m_nHeight)];

      // Obtain windowed pixel format and create a new context.
      newContext = (NSOpenGLContext*)CreateWindowedContext((void* )cur_context);
      [newContext setView:blankView];

      // Hide the menu bar.
      if (GetDisplayID(res.iScreen) == kCGDirectMainDisplay || CDarwinUtils::IsMavericks() )
        SetMenuBarVisible(false);

      // Blank other displays if requested.
      if (blankOtherDisplays)
        BlankOtherDisplays(res.iScreen);
    }
    else
    {
      // hide the window
      [[last_view window] setFrameOrigin:[last_window_screen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ last_view setFrameOrigin:NSMakePoint(0.0, 0.0)];
      [ last_view setFrameSize:NSMakeSize(m_nWidth, m_nHeight) ];

      // This is OpenGL FullScreen Mode
      // create our new context (sharing with the current one)
      newContext = (NSOpenGLContext*)CreateFullScreenContext(res.iScreen, (void*)cur_context);
      if (!newContext)
        return false;

      // clear the current context
      [NSOpenGLContext clearCurrentContext];

      // set fullscreen
      [newContext setFullScreen];

      // Capture the display before going fullscreen.
      if (blankOtherDisplays == true)
        CGCaptureAllDisplays();
      else
        CGDisplayCapture(GetDisplayID(res.iScreen));

      // If we don't hide menu bar, it will get events and interrupt the program.
      if (GetDisplayID(res.iScreen) == kCGDirectMainDisplay || CDarwinUtils::IsMavericks() )
        SetMenuBarVisible(false);
    }

    // Hide the mouse.
    [NSCursor hide];

    // Release old context if we created it.
    if (m_lastOwnedContext == cur_context)
    {
      [ NSOpenGLContext clearCurrentContext ];
      [ cur_context clearDrawable ];
      [ cur_context release ];
    }

    // activate context
    [newContext makeCurrentContext];
    m_lastOwnedContext = newContext;
  }
  else
  {
    // Windowed Mode
    // exit fullscreen
    [cur_context clearDrawable];

    [NSCursor unhide];

    // Show menubar.
    if (GetDisplayID(res.iScreen) == kCGDirectMainDisplay || CDarwinUtils::IsMavericks() )
      SetMenuBarVisible(true);

    if (CSettings::Get().GetBool("videoscreen.fakefullscreen"))
    {
      // restore the windowed window level
      [[last_view window] setLevel:last_window_level];

      // Get rid of the new window we created.
      if (windowedFullScreenwindow != NULL)
      {
        [windowedFullScreenwindow close];
        if ([windowedFullScreenwindow isReleasedWhenClosed] == NO)
          [windowedFullScreenwindow release];
        windowedFullScreenwindow = NULL;
      }

      // Unblank.
      // Force the unblank when returning from fullscreen, we get called with blankOtherDisplays set false.
      //if (blankOtherDisplays)
      UnblankDisplays();
    }
    else
    {
      // release displays
      CGReleaseAllDisplays();
    }

    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)CreateWindowedContext((void* )cur_context);
    if (!newContext)
      return false;

    // Assign view from old context, move back to original screen.
    [newContext setView:last_view];
    [[last_view window] setFrameOrigin:last_window_origin];
    // return the mouse bounds in SDL view to prevous size
    [ last_view setFrameSize:last_view_size ];
    [ last_view setFrameOrigin:last_view_origin ];
    // done with restoring windowed window, don't set last_view to NULL as we can lose it under dual displays.
    //last_window_screen = NULL;

    // Release the fullscreen context.
    if (m_lastOwnedContext == cur_context)
    {
      [ NSOpenGLContext clearCurrentContext ];
      [ cur_context clearDrawable ];
      [ cur_context release ];
    }

    // Activate context.
    [newContext makeCurrentContext];
    m_lastOwnedContext = newContext;
  }

  DisplayFadeFromBlack(fade_token, needtoshowme);

  ShowHideNSWindow([last_view window], needtoshowme);
  // need to make sure SDL tracks any window size changes
  ResizeWindowInternal(m_nWidth, m_nHeight, -1, -1, last_view);
  HandlePossibleRefreshrateChange();

  return true;
}

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  // Add desktop resolution
  int w, h;
  double fps;

  // first screen goes into the current desktop mode
  GetScreenResolution(&w, &h, &fps, 0);
  UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, w, h, fps);
  NSString *dispName = screenNameForDisplay(GetDisplayID(0));

  if (dispName != nil)
  {
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput = [dispName UTF8String];
  }

  CDisplaySettings::Get().ClearCustomResolutions();

  // see resolution.h enum RESOLUTION for how the resolutions
  // have to appear in the resolution info vector in CDisplaySettings
  // add the desktop resolutions of the other screens
  for(int i = 1; i < GetNumScreens(); i++)
  {
    RESOLUTION_INFO res;
    // get current resolution of screen i
    GetScreenResolution(&w, &h, &fps, i);
    UpdateDesktopResolution(res, i, w, h, fps);
    dispName = screenNameForDisplay(GetDisplayID(i));

    if (dispName != nil)
    {
      res.strOutput = [dispName UTF8String];
    }

    CDisplaySettings::Get().AddResolutionInfo(res);
  }

  if (m_can_display_switch)
  {
    // now just fill in the possible reolutions for the attached screens
    // and push to the resolution info vector
    FillInVideoModes();
  }
  CDisplaySettings::Get().ApplyCalibrations();
}

/*
void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx)
{
  if (!pixFmt)
    return nil;

  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
    shareContext:(NSOpenGLContext*)shareCtx];

  // snipit from SDL_cocoaopengl.m
  //
  // Wisdom from Apple engineer in reference to UT2003's OpenGL performance:
  //  "You are blowing a couple of the internal OpenGL function caches. This
  //  appears to be happening in the VAO case.  You can tell OpenGL to up
  //  the cache size by issuing the following calls right after you create
  //  the OpenGL context.  The default cache size is 16."    --ryan.
  //

  #ifndef GLI_ARRAY_FUNC_CACHE_MAX
  #define GLI_ARRAY_FUNC_CACHE_MAX 284
  #endif

  #ifndef GLI_SUBMIT_FUNC_CACHE_MAX
  #define GLI_SUBMIT_FUNC_CACHE_MAX 280
  #endif

  {
      long cache_max = 64;
      CGLContextObj ctx = (CGLContextObj)[newContext CGLContextObj];
      CGLSetParameter(ctx, (CGLContextParameter)GLI_SUBMIT_FUNC_CACHE_MAX, &cache_max);
      CGLSetParameter(ctx, (CGLContextParameter)GLI_ARRAY_FUNC_CACHE_MAX, &cache_max);
  }

  // End Wisdom from Apple Engineer section. --ryan.
  return newContext;
}
*/

void* CWinSystemOSX::CreateWindowedContext(void* shareCtx)
{
  NSOpenGLContext* newContext = NULL;

  NSOpenGLPixelFormatAttribute wattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAWindow,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)8,
    (NSOpenGLPixelFormatAttribute)0
  };

  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];

  newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
    shareContext:(NSOpenGLContext*)shareCtx];
  [pixFmt release];

  if (!newContext)
  {
    // bah, try again for non-accelerated renderer
    NSOpenGLPixelFormatAttribute wattrs2[] =
    {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAWindow,
      NSOpenGLPFANoRecovery,
      NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)8,
      (NSOpenGLPixelFormatAttribute)0
    };
    NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs2];

    newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
      shareContext:(NSOpenGLContext*)shareCtx];
    [pixFmt release];
  }

  return newContext;
}

void* CWinSystemOSX::CreateFullScreenContext(int screen_index, void* shareCtx)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;
  CGDirectDisplayID displayID;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  displayID = displayArray[screen_index];

  NSOpenGLPixelFormatAttribute fsattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAFullScreen,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize,  (NSOpenGLPixelFormatAttribute)8,
    NSOpenGLPFAScreenMask, (NSOpenGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask(displayID),
    (NSOpenGLPixelFormatAttribute)0
  };

  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs];
  if (!pixFmt)
    return nil;

  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
    shareContext:(NSOpenGLContext*)shareCtx];
  [pixFmt release];

  return newContext;
}

void CWinSystemOSX::GetScreenResolution(int* w, int* h, double* fps, int screenIdx)
{
  // Figure out the screen size. (default to main screen)
  if (screenIdx >= GetNumScreens())
    return;
  CGDirectDisplayID display_id = (CGDirectDisplayID)GetDisplayID(screenIdx);
  CGDisplayModeRef mode  = CGDisplayCopyDisplayMode(display_id);
  *w = CGDisplayModeGetWidth(mode);
  *h = CGDisplayModeGetHeight(mode);
  *fps = CGDisplayModeGetRefreshRate(mode);
  CGDisplayModeRelease(mode);
  if ((int)*fps == 0)
  {
    // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
    *fps = 60.0;
  }
}

void CWinSystemOSX::EnableVSync(bool enable)
{
  // OpenGL Flush synchronised with vertical retrace
  GLint swapInterval = enable ? 1 : 0;
  [[NSOpenGLContext currentContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
}

bool CWinSystemOSX::SwitchToVideoMode(int width, int height, double refreshrate, int screenIdx)
{
  // SwitchToVideoMode will not return until the display has actually switched over.
  // This can take several seconds.
  if( screenIdx >= GetNumScreens())
    return false;

  boolean_t match = false;
  CFDictionaryRef dispMode = NULL;
  // Figure out the screen size. (default to main screen)
  CGDirectDisplayID display_id = GetDisplayID(screenIdx);

  // find mode that matches the desired size, refreshrate
  // non interlaced, nonstretched, safe for hardware
  dispMode = GetMode(width, height, refreshrate, screenIdx);

  //not found - fallback to bestemdeforparameters
  if (!dispMode)
  {
    dispMode = CGDisplayBestModeForParameters(display_id, 32, width, height, &match);

    if (!match)
      dispMode = CGDisplayBestModeForParameters(display_id, 16, width, height, &match);

    if (!match)
      return false;
  }

  // switch mode and return success
  CGDisplayCapture(display_id);
  CGDisplayConfigRef cfg;
  CGBeginDisplayConfiguration(&cfg);
  // we don't need to do this, we are already faded.
  //CGConfigureDisplayFadeEffect(cfg, 0.3f, 0.5f, 0, 0, 0);
  CGConfigureDisplayMode(cfg, display_id, dispMode);
  CGError err = CGCompleteDisplayConfiguration(cfg, kCGConfigureForAppOnly);
  CGDisplayRelease(display_id);
  
  m_refreshRate = GetDictionaryDouble(dispMode, kCGDisplayRefreshRate);

  Cocoa_CVDisplayLinkUpdate();

  return (err == kCGErrorSuccess);
}

void CWinSystemOSX::FillInVideoModes()
{
  // Add full screen settings for additional monitors
  int numDisplays = [[NSScreen screens] count];

  for (int disp = 0; disp < numDisplays; disp++)
  {
    Boolean stretched;
    Boolean interlaced;
    Boolean safeForHardware;
    Boolean televisionoutput;
    int w, h, bitsperpixel;
    double refreshrate;
    RESOLUTION_INFO res;

    CFArrayRef displayModes = CGDisplayAvailableModes(GetDisplayID(disp));
    NSString *dispName = screenNameForDisplay(GetDisplayID(disp));

    if (dispName != nil)
    {
      CLog::Log(LOGNOTICE, "Display %i has name %s", disp, [dispName UTF8String]);
    }

    if (NULL == displayModes)
      continue;

    for (int i=0; i < CFArrayGetCount(displayModes); ++i)
    {
      CFDictionaryRef displayMode = (CFDictionaryRef)CFArrayGetValueAtIndex(displayModes, i);

      stretched = GetDictionaryBoolean(displayMode, kCGDisplayModeIsStretched);
      interlaced = GetDictionaryBoolean(displayMode, kCGDisplayModeIsInterlaced);
      bitsperpixel = GetDictionaryInt(displayMode, kCGDisplayBitsPerPixel);
      safeForHardware = GetDictionaryBoolean(displayMode, kCGDisplayModeIsSafeForHardware);
      televisionoutput = GetDictionaryBoolean(displayMode, kCGDisplayModeIsTelevisionOutput);

      if ((bitsperpixel == 32)      &&
          (safeForHardware == YES)  &&
          (stretched == NO)         &&
          (interlaced == NO))
      {
        w = GetDictionaryInt(displayMode, kCGDisplayWidth);
        h = GetDictionaryInt(displayMode, kCGDisplayHeight);
        refreshrate = GetDictionaryDouble(displayMode, kCGDisplayRefreshRate);
        if ((int)refreshrate == 0)  // LCD display?
        {
          // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
          refreshrate = 60.0;
        }
        CLog::Log(LOGNOTICE, "Found possible resolution for display %d with %d x %d @ %f Hz\n", disp, w, h, refreshrate);

        UpdateDesktopResolution(res, disp, w, h, refreshrate);

        // overwrite the mode str because  UpdateDesktopResolution adds a
        // "Full Screen". Since the current resolution is there twice
        // this would lead to 2 identical resolution entrys in the guisettings.xml.
        // That would cause problems with saving screen overscan calibration
        // because the wrong entry is picked on load.
        // So we just use UpdateDesktopResolutions for the current DESKTOP_RESOLUTIONS
        // in UpdateResolutions. And on all othere resolutions make a unique
        // mode str by doing it without appending "Full Screen".
        // this is what linux does - though it feels that there shouldn't be
        // the same resolution twice... - thats why i add a FIXME here.
        res.strMode = StringUtils::Format("%dx%d @ %.2f", w, h, refreshrate);

        if (dispName != nil)
        {
          res.strOutput = [dispName UTF8String];
        }

        g_graphicsContext.ResetOverscan(res);
        CDisplaySettings::Get().AddResolutionInfo(res);
      }
    }
  }
}

bool CWinSystemOSX::FlushBuffer(void)
{
  [ (NSOpenGLContext*)m_glContext flushBuffer ];

  return true;
}

bool CWinSystemOSX::IsObscured(void)
{
  if (m_bFullScreen && !CSettings::Get().GetBool("videoscreen.fakefullscreen"))
    return false;// in true fullscreen mode - we can't be obscured by anyone...

  // check once a second if we are obscured.
  unsigned int now_time = XbmcThreads::SystemClockMillis();
  if (m_obscured_timecheck > now_time)
    return m_obscured;
  else
    m_obscured_timecheck = now_time + 1000;

  NSOpenGLContext* cur_context = [NSOpenGLContext currentContext];
  NSView* view = [cur_context view];
  if (!view)
  {
    // sanity check, we should always have a view
    m_obscured = true;
    return m_obscured;
  }

  NSWindow *window = [view window];
  if (!window)
  {
    // sanity check, we should always have a window
    m_obscured = true;
    return m_obscured;
  }

  if ([window isVisible] == NO)
  {
    // not visable means the window is not showing.
    // this should never really happen as we are always visable
    // even when minimized in dock.
    m_obscured = true;
    return m_obscured;
  }

  // check if we are minimized (to an icon in the Dock).
  if ([window isMiniaturized] == YES)
  {
    m_obscured = true;
    return m_obscured;
  }

  // check if we are showing on the active workspace.
  if ([window isOnActiveSpace] == NO)
  {
    m_obscured = true;
    return m_obscured;
  }

  // default to false before we start parsing though the windows.
  // if we are are obscured by any windows, then set true.
  m_obscured = false;
  static bool obscureLogged = false;

  CGWindowListOption opts;
  opts = kCGWindowListOptionOnScreenAboveWindow | kCGWindowListExcludeDesktopElements;
  CFArrayRef windowIDs =CGWindowListCreate(opts, (CGWindowID)[window windowNumber]);  

  if (!windowIDs)
    return m_obscured;

  CFArrayRef windowDescs = CGWindowListCreateDescriptionFromArray(windowIDs);
  if (!windowDescs)
  {
    CFRelease(windowIDs);
    return m_obscured;
  }

  CGRect bounds = NSRectToCGRect([window frame]);
  // kCGWindowBounds measures the origin as the top-left corner of the rectangle
  //  relative to the top-left corner of the screen.
  // NSWindow’s frame property measures the origin as the bottom-left corner
  //  of the rectangle relative to the bottom-left corner of the screen.
  // convert bounds from NSWindow to CGWindowBounds here.
  bounds.origin.y = [[window screen] frame].size.height - bounds.origin.y - bounds.size.height;

  std::vector<CRect> partialOverlaps;
  CRect ourBounds = CGRectToCRect(bounds);

  for (CFIndex idx=0; idx < CFArrayGetCount(windowDescs); idx++)
  {
    // walk the window list of windows that are above us and are not desktop elements
    CFDictionaryRef windowDictionary = (CFDictionaryRef)CFArrayGetValueAtIndex(windowDescs, idx);

    // skip the Dock window, it actually covers the entire screen.
    CFStringRef ownerName = (CFStringRef)CFDictionaryGetValue(windowDictionary, kCGWindowOwnerName);
    if (CFStringCompare(ownerName, CFSTR("Dock"), 0) == kCFCompareEqualTo)
      continue;

    // Ignore known brightness tools for dimming the screen. They claim to cover
    // the whole XBMC window and therefore would make the framerate limiter
    // kicking in. Unfortunatly even the alpha of these windows is 1.0 so
    // we have to check the ownerName.
    if (CFStringCompare(ownerName, CFSTR("Shades"), 0)            == kCFCompareEqualTo ||
        CFStringCompare(ownerName, CFSTR("SmartSaver"), 0)        == kCFCompareEqualTo ||
        CFStringCompare(ownerName, CFSTR("Brightness Slider"), 0) == kCFCompareEqualTo ||
        CFStringCompare(ownerName, CFSTR("Displaperture"), 0)     == kCFCompareEqualTo ||
        CFStringCompare(ownerName, CFSTR("Dreamweaver"), 0)       == kCFCompareEqualTo ||
        CFStringCompare(ownerName, CFSTR("Window Server"), 0)     ==  kCFCompareEqualTo)
      continue;

    CFDictionaryRef rectDictionary = (CFDictionaryRef)CFDictionaryGetValue(windowDictionary, kCGWindowBounds);
    if (!rectDictionary)
      continue;

    CGRect windowBounds;
    if (CGRectMakeWithDictionaryRepresentation(rectDictionary, &windowBounds))
    {
      if (CGRectContainsRect(windowBounds, bounds))
      {
        // if the windowBounds completely encloses our bounds, we are obscured.
        if (!obscureLogged)
        {
          std::string appName;
          if (CDarwinUtils::CFStringRefToUTF8String(ownerName, appName))
            CLog::Log(LOGDEBUG, "WinSystemOSX: Fullscreen window %s obscures XBMC!", appName.c_str());
          obscureLogged = true;
        }
        m_obscured = true;
        break;
      }

      // handle overlaping windows above us that combine
      // to obscure by collecting any partial overlaps,
      // then subtract them from our bounds and check
      // for any remaining area.
      CRect intersection = CGRectToCRect(windowBounds);
      intersection.Intersect(ourBounds);
      if (!intersection.IsEmpty())
        partialOverlaps.push_back(intersection);
    }
  }

  if (!m_obscured)
  {
    // if we are here we are not obscured by any fullscreen window - reset flag
    // for allowing the logmessage above to show again if this changes.
    if (obscureLogged)
      obscureLogged = false;
    std::vector<CRect> rects = ourBounds.SubtractRects(partialOverlaps);
    // they got us covered
    if (rects.size() == 0)
      m_obscured = true;
  }

  CFRelease(windowDescs);
  CFRelease(windowIDs);

  return m_obscured;
}

void CWinSystemOSX::NotifyAppFocusChange(bool bGaining)
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  if (m_bFullScreen && bGaining)
  {
    // find the window
    NSOpenGLContext* context = [NSOpenGLContext currentContext];
    if (context)
    {
      NSView* view;

      view = [context view];
      if (view)
      {
        NSWindow* window;
        window = [view window];
        if (window)
        {
          // find the screenID
          NSDictionary* screenInfo = [[window screen] deviceDescription];
          NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];
          if ((CGDirectDisplayID)[screenID longValue] == kCGDirectMainDisplay || CDarwinUtils::IsMavericks() )
          {
            SetMenuBarVisible(false);
          }
          [window orderFront:nil];
        }
      }
    }
  }
  [pool release];
}

void CWinSystemOSX::ShowOSMouse(bool show)
{
  SDL_ShowCursor(show ? 1 : 0);
}

bool CWinSystemOSX::Minimize()
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  [[NSApplication sharedApplication] miniaturizeAll:nil];

  [pool release];
  return true;
}

bool CWinSystemOSX::Restore()
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  [[NSApplication sharedApplication] unhide:nil];

  [pool release];
  return true;
}

bool CWinSystemOSX::Hide()
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  [[NSApplication sharedApplication] hide:nil];

  [pool release];
  return true;
}

void CWinSystemOSX::HandlePossibleRefreshrateChange()
{
  static double oldRefreshRate = m_refreshRate;
  Cocoa_CVDisplayLinkUpdate();
  int dummy = 0;
  
  GetScreenResolution(&dummy, &dummy, &m_refreshRate, GetCurrentScreen());

  if (oldRefreshRate != m_refreshRate)
  {
    oldRefreshRate = m_refreshRate;
    // send a message so that videoresolution (and refreshrate)
    // is changed
    ThreadMessage msg = {TMSG_VIDEORESIZE};
    msg.param1 = m_SDLSurface->w;
    msg.param2 = m_SDLSurface->h;
    CApplicationMessenger::Get().SendMessage(msg, false);
  }
}

void CWinSystemOSX::OnMove(int x, int y)
{
  HandlePossibleRefreshrateChange();
}

void CWinSystemOSX::EnableSystemScreenSaver(bool bEnable)
{
  // see Technical Q&A QA1340
  static IOPMAssertionID assertionID = 0;

  if (!bEnable)
  {
    if (assertionID == 0)
    {
      CFStringRef reasonForActivity= CFSTR("XBMC requested disable system screen saver");
      IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
        kIOPMAssertionLevelOn, reasonForActivity, &assertionID);
    }
    UpdateSystemActivity(UsrActivity);
  }
  else if (assertionID != 0)
  {
    IOPMAssertionRelease(assertionID);
    assertionID = 0;
  }

  m_use_system_screensaver = bEnable;
}

bool CWinSystemOSX::IsSystemScreenSaverEnabled()
{
  return m_use_system_screensaver;
}

void CWinSystemOSX::ResetOSScreensaver()
{
  // allow os screensaver only if we are fullscreen
  EnableSystemScreenSaver(!m_bFullScreen);
}

bool CWinSystemOSX::EnableFrameLimiter()
{
  return IsObscured();
}

void CWinSystemOSX::EnableTextInput(bool bEnable)
{
  if (bEnable)
    StartTextInput();
  else
    StopTextInput();
}

OSXTextInputResponder *g_textInputResponder = nil;

bool CWinSystemOSX::IsTextInputEnabled()
{
  return g_textInputResponder != nil && [[g_textInputResponder superview] isEqual: [[NSApp keyWindow] contentView]];
}

void CWinSystemOSX::StartTextInput()
{
  NSView *parentView = [[NSApp keyWindow] contentView];

  /* We only keep one field editor per process, since only the front most
   * window can receive text input events, so it make no sense to keep more
   * than one copy. When we switched to another window and requesting for
   * text input, simply remove the field editor from its superview then add
   * it to the front most window's content view */
  if (!g_textInputResponder) {
    g_textInputResponder =
    [[OSXTextInputResponder alloc] initWithFrame: NSMakeRect(0.0, 0.0, 0.0, 0.0)];
  }

  if (![[g_textInputResponder superview] isEqual: parentView])
  {
//    DLOG(@"add fieldEdit to window contentView");
    [g_textInputResponder removeFromSuperview];
    [parentView addSubview: g_textInputResponder];
    [[NSApp keyWindow] makeFirstResponder: g_textInputResponder];
  }
}
void CWinSystemOSX::StopTextInput()
{
  if (g_textInputResponder) {
    [g_textInputResponder removeFromSuperview];
    [g_textInputResponder release];
    g_textInputResponder = nil;
  }
}

void CWinSystemOSX::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemOSX::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

bool CWinSystemOSX::Show(bool raise)
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  if (raise)
  {
    [[NSApplication sharedApplication] unhide:nil];
    [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];
    [[NSApplication sharedApplication] arrangeInFront:nil];
  }
  else
  {
    [[NSApplication sharedApplication] unhideWithoutActivation];
  }

  [pool release];
  return true;
}

int CWinSystemOSX::GetNumScreens()
{
  int numDisplays = [[NSScreen screens] count];
  return(numDisplays);
}

int CWinSystemOSX::GetCurrentScreen()
{
  return m_lastDisplayNr;
}

void CWinSystemOSX::WindowChangedScreen()
{
  // user has moved the window to a
  // different screen
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  m_lastDisplayNr = -1;
  
  // if we are here the user dragged the window to a different
  // screen and we return the screen of the window
  if (context)
  {
    NSView* view;

    view = [context view];
    if (view)
    {
      NSWindow* window;
      window = [view window];
      if (window)
      {
        m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen( [window screen] ));
      }
    }
  }
  if (m_lastDisplayNr == -1)
    m_lastDisplayNr = 0;// default to main screen
}

void CWinSystemOSX::AnnounceOnLostDevice()
{
  CSingleLock lock(m_resourceSection);
  // tell any shared resources
  CLog::Log(LOGDEBUG, "CWinSystemOSX::AnnounceOnLostDevice");
  for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnLostDevice();
}

void CWinSystemOSX::AnnounceOnResetDevice()
{
  CSingleLock lock(m_resourceSection);
  // tell any shared resources
  CLog::Log(LOGDEBUG, "CWinSystemOSX::AnnounceOnResetDevice");
  for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnResetDevice();
}

void* CWinSystemOSX::GetCGLContextObj()
{
  return [(NSOpenGLContext*)m_glContext CGLContextObj];
}

void* CWinSystemOSX::GetNSOpenGLContext()
{
  return m_glContext;
}

std::string CWinSystemOSX::GetClipboardText(void)
{
  std::string utf8_text;

  const char *szStr = Cocoa_Paste();
  if (szStr)
    utf8_text = szStr;

  return utf8_text;
}

#endif
