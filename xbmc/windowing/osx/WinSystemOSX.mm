/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemOSX.h"

#include "AppInboundProtocol.h"
#include "CompileInfo.h"
#include "OSScreenSaverOSX.h"
#include "ServiceBroker.h"
#include "VideoSyncOsx.h"
#include "WinEventsOSX.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/AESinkDARWINOSX.h"
#include "cores/RetroPlayer/process/osx/RPProcessInfoOSX.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VTB.h"
#include "cores/VideoPlayer/Process/osx/ProcessInfoOSX.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVTBGL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "guilib/DispResource.h"
#include "guilib/GUIWindowManager.h"
#include "input/KeyboardStat.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/gl/ScreenshotSurfaceGL.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/osx/CocoaDPMSSupport.h"

#include "platform/darwin/DarwinUtils.h"
#include "platform/darwin/DictionaryUtils.h"
#include "platform/darwin/osx/CocoaInterface.h"
#import "platform/darwin/osx/OSXTextInputResponder.h"
#include "platform/darwin/osx/XBMCHelper.h"
#include "platform/darwin/osx/powermanagement/CocoaPowerSyscall.h"

#include <cstdlib>
#include <signal.h>

#import <Cocoa/Cocoa.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <QuartzCore/QuartzCore.h>
#import <SDL/SDL.h>

// turn off deprecated warning spew.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using namespace KODI;
using namespace MESSAGING;
using namespace WINDOWING;

//------------------------------------------------------------------------------------------
// special object-c class for handling the NSWindowDidMoveNotification callback.
@interface windowDidMoveNoteClass : NSObject
{
  void *m_userdata;
}
+ (windowDidMoveNoteClass*) initWith: (void*) userdata;
-  (void) windowDidMoveNotification:(NSNotification*) note;
@end

@implementation windowDidMoveNoteClass
+ (windowDidMoveNoteClass*) initWith: (void*) userdata
{
    windowDidMoveNoteClass *windowDidMove = [windowDidMoveNoteClass new];
    windowDidMove->m_userdata = userdata;
    return windowDidMove;
}
-  (void) windowDidMoveNotification:(NSNotification*) note
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
      std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
      if (appPort)
        appPort->OnEvent(newEvent);
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
+ (windowDidReSizeNoteClass*) initWith: (void*) userdata;
- (void) windowDidReSizeNotification:(NSNotification*) note;
@end
@implementation windowDidReSizeNoteClass
+ (windowDidReSizeNoteClass*) initWith: (void*) userdata
{
    windowDidReSizeNoteClass *windowDidReSize = [windowDidReSizeNoteClass new];
    windowDidReSize->m_userdata = userdata;
    return windowDidReSize;
}
- (void) windowDidReSizeNotification:(NSNotification*) note
{
  CWinSystemOSX *winsys = (CWinSystemOSX*)m_userdata;
	if (!winsys)
    return;

}
@end

//------------------------------------------------------------------------------------------
// special object-c class for handling the NSWindowDidChangeScreenNotification callback.
@interface windowDidChangeScreenNoteClass : NSObject
{
  void *m_userdata;
}
+ (windowDidChangeScreenNoteClass*) initWith: (void*) userdata;
- (void) windowDidChangeScreenNotification:(NSNotification*) note;
@end
@implementation windowDidChangeScreenNoteClass
+ (windowDidChangeScreenNoteClass*) initWith: (void*) userdata
{
    windowDidChangeScreenNoteClass *windowDidChangeScreen = [windowDidChangeScreenNoteClass new];
    windowDidChangeScreen->m_userdata = userdata;
    return windowDidChangeScreen;
}
- (void) windowDidChangeScreenNotification:(NSNotification*) note
{
  CWinSystemOSX *winsys = (CWinSystemOSX*)m_userdata;
	if (!winsys)
    return;
  winsys->WindowChangedScreen();
}
@end
//------------------------------------------------------------------------------------------

class CWinSystemOSXImpl
{
public:
  NSOpenGLContext* m_glContext;
  static NSOpenGLContext* m_lastOwnedContext;

  windowDidMoveNoteClass* m_windowDidMove;
  windowDidReSizeNoteClass* m_windowDidReSize;
  windowDidChangeScreenNoteClass* m_windowChangedScreen;
};

NSOpenGLContext* CWinSystemOSXImpl::m_lastOwnedContext = nil;


#define MAX_DISPLAYS 32
// if there was a devicelost callback
// but no device reset for 3 secs
// a timeout fires the reset callback
// (for ensuring that e.x. AE isn't stuck)
#define LOST_DEVICE_TIMEOUT_MS 3000
static NSWindow* blankingWindows[MAX_DISPLAYS];

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
  if (screen_index >= 0 && screen_index < numDisplays)
    return(displayArray[screen_index]);
  else
    return(displayArray[0]);
}

size_t DisplayBitsPerPixelForMode(CGDisplayModeRef mode)
{
  size_t bitsPerPixel = 0;

  CFStringRef pixEnc = CGDisplayModeCopyPixelEncoding(mode);
  if(CFStringCompare(pixEnc, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
  {
    bitsPerPixel = 32;
  }
  else if(CFStringCompare(pixEnc, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
  {
    bitsPerPixel = 16;
  }
  else if(CFStringCompare(pixEnc, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
  {
    bitsPerPixel = 8;
  }

  CFRelease(pixEnc);

  return bitsPerPixel;
}

CFArrayRef GetAllDisplayModes(CGDirectDisplayID display)
{
  int value = 1;

  CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &value);
  if (!number)
  {
    CLog::Log(LOGERROR, "GetAllDisplayModes - could not create Number!");
    return NULL;
  }

  CFStringRef key = kCGDisplayShowDuplicateLowResolutionModes;
  CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&key, (const void **)&number, 1, NULL, NULL);
  CFRelease(number);

  if (!options)
  {
    CLog::Log(LOGERROR, "GetAllDisplayModes - could not create Dictionary!");
    return NULL;
  }

  CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(display, options);
  CFRelease(options);

  if (!displayModes)
  {
    CLog::Log(LOGERROR, "GetAllDisplayModes - no displaymodes found!");
    return NULL;
  }

  return displayModes;
}

// mimic former behavior of deprecated CGDisplayBestModeForParameters
CGDisplayModeRef BestMatchForMode(CGDirectDisplayID display, size_t bitsPerPixel, size_t width, size_t height, boolean_t &match)
{
  
  // Get a copy of the current display mode
  CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(display);

  // Loop through all display modes to determine the closest match.
  // CGDisplayBestModeForParameters is deprecated on 10.6 so we will emulate it's behavior
  // Try to find a mode with the requested depth and equal or greater dimensions first.
  // If no match is found, try to find a mode with greater depth and same or greater dimensions.
  // If still no match is found, just use the current mode.
  CFArrayRef allModes = GetAllDisplayModes(display);

  for(int i = 0; i < CFArrayGetCount(allModes); i++)	{
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);

    if(DisplayBitsPerPixelForMode(mode) != bitsPerPixel)
      continue;

    if((CGDisplayModeGetWidth(mode) == width) && (CGDisplayModeGetHeight(mode) == height))
    {
      CGDisplayModeRelease(displayMode); // release the copy we got before ...
      displayMode = mode;
      match = true;
      break;
    }
  }

  // No depth match was found
  if(!match)
  {
    for(int i = 0; i < CFArrayGetCount(allModes); i++)
    {
      CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
      if(DisplayBitsPerPixelForMode(mode) >= bitsPerPixel)
        continue;

      if((CGDisplayModeGetWidth(mode) == width) && (CGDisplayModeGetHeight(mode) == height))
      {
        displayMode = mode;
        match = true;
        break;
      }
    }
  }

  CFRelease(allModes);

  return displayMode;
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
  NSString* screenName;
  @autoreleasepool
  {
    NSDictionary* deviceInfo = (__bridge_transfer NSDictionary*)IODisplayCreateInfoDictionary(
        CGDisplayIOServicePort(displayID), kIODisplayOnlyPreferredName);
    NSDictionary* localizedNames =
        [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];

    if ([localizedNames count] > 0)
    {
      screenName = [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];
    }
  }

  if (screenName == nil)
  {
    screenName = [[NSString alloc] initWithFormat:@"%i", displayID];
  }
  else
  {
    // ensure screen name is unique by appending displayid
    screenName = [screenName stringByAppendingFormat:@" (%@)", [@(displayID) stringValue]];
  }

  return screenName;
}

int GetDisplayIndex(std::string dispName)
{
  int ret = 0;

  // Add full screen settings for additional monitors
  int numDisplays = [[NSScreen screens] count];

  for (int disp = 0; disp < numDisplays; disp++)
  {
    NSString *name = screenNameForDisplay(GetDisplayID(disp));
    if ([name UTF8String] == dispName)
    {
      ret = disp;
      break;
    }
  }

  return ret;
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
CGDisplayModeRef GetMode(int width, int height, double refreshrate, int screenIdx)
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

  CLog::Log(LOGDEBUG, "GetMode looking for suitable mode with %d x %d @ %f Hz on display %d", width, height, refreshrate, screenIdx);

  CFArrayRef displayModes = GetAllDisplayModes(GetDisplayID(screenIdx));

  if (!displayModes)
    return NULL;

  for (int i=0; i < CFArrayGetCount(displayModes); ++i)
  {
    CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, i);
    uint32_t flags = CGDisplayModeGetIOFlags(displayMode);
    stretched = flags & kDisplayModeStretchedFlag ? true : false;
    interlaced = flags & kDisplayModeInterlacedFlag ? true : false;
    bitsperpixel = DisplayBitsPerPixelForMode(displayMode);
    safeForHardware = flags & kDisplayModeSafetyFlags ? true : false;
    televisionoutput = flags & kDisplayModeTelevisionFlag ? true : false;
    w = CGDisplayModeGetWidth(displayMode);
    h = CGDisplayModeGetHeight(displayMode);
    rate = CGDisplayModeGetRefreshRate(displayMode);


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

  CFRelease(displayModes);
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
    RESOLUTION res = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
    if (res == RES_INVALID)
      return;

    NSScreen* pScreen = nil;
    unsigned int screenIdx = 0;

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
      winsys->HandleOnResetDevice();
    }
  }

  if ((flags & kCGDisplayAddFlag) || (flags & kCGDisplayRemoveFlag))
    winsys->UpdateResolutions();
}

//------------------------------------------------------------------------------
NSOpenGLContext* CreateWindowedContext(NSOpenGLContext* shareCtx);
void ResizeWindowInternal(int newWidth, int newHeight, int newLeft, int newTop, NSView* last_view);

//------------------------------------------------------------------------------
CWinSystemOSX::CWinSystemOSX()
  : CWinSystemBase()
  , m_impl{new CWinSystemOSXImpl}
  , m_lostDeviceTimer(this)
{
  m_SDLSurface = NULL;
  m_osx_events = NULL;
  m_obscured   = false;
  m_obscured_timecheck = XbmcThreads::SystemClockMillis() + 1000;
  m_lastDisplayNr = -1;
  m_movedToOtherScreen = false;
  m_refreshRate = 0.0;
  m_delayDispReset = false;

  m_winEvents.reset(new CWinEventsOSX());

  AE::CAESinkFactory::ClearSinks();
  CAESinkDARWINOSX::Register();
  CCocoaPowerSyscall::Register();
  m_dpms = std::make_shared<CCocoaDPMSSupport>();
}

CWinSystemOSX::~CWinSystemOSX() = default;

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
  HandleOnResetDevice();
}

bool CWinSystemOSX::InitWindowSystem()
{
  CLog::LogF(LOGNOTICE, "Setup SDL");

  /* Clean up on exit, exit on window close and interrupt */
  std::atexit(SDL_Quit);

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    CLog::LogF(LOGFATAL, "Unable to initialize SDL: %s", SDL_GetError());
    return false;
  }
  // SDL_Init will install a handler for segfaults, restore the default handler.
  signal(SIGSEGV, SIG_DFL);

  SDL_EnableUNICODE(1);

  // set repeat to 10ms to ensure repeat time < frame time
  // so that hold times can be reliably detected
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  m_osx_events = new CWinEventsOSX();

  CGDisplayRegisterReconfigurationCallback(DisplayReconfigured, (void*)this);

  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  auto windowDidMove = [windowDidMoveNoteClass initWith:this];
  [center addObserver:windowDidMove
    selector:@selector(windowDidMoveNotification:)
    name:NSWindowDidMoveNotification object:nil];
  m_impl->m_windowDidMove = windowDidMove;

  auto windowDidReSize = [windowDidReSizeNoteClass initWith:this];
  [center addObserver:windowDidReSize
    selector:@selector(windowDidReSizeNotification:)
    name:NSWindowDidResizeNotification object:nil];
  m_impl->m_windowDidReSize = windowDidReSize;

  auto windowDidChangeScreen = [windowDidChangeScreenNoteClass initWith:this];
  [center addObserver:windowDidChangeScreen
    selector:@selector(windowDidChangeScreenNotification:)
    name:NSWindowDidChangeScreenNotification object:nil];
  m_impl->m_windowChangedScreen = windowDidChangeScreen;

  return true;
}

bool CWinSystemOSX::DestroyWindowSystem()
{
  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  [center removeObserver:m_impl->m_windowDidMove name:NSWindowDidMoveNotification object:nil];
  [center removeObserver:m_impl->m_windowDidReSize name:NSWindowDidResizeNotification object:nil];
  [center removeObserver:m_impl->m_windowChangedScreen
                    name:NSWindowDidChangeScreenNotification
                  object:nil];

  CGDisplayRemoveReconfigurationCallback(DisplayReconfigured, (void*)this);

  delete m_osx_events;
  m_osx_events = NULL;

  UnblankDisplays();
  m_impl->m_glContext = nil;
  return true;
}

bool CWinSystemOSX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  // force initial window creation to be windowed, if fullscreen, it will switch to it below
  // fixes the white screen of death if starting fullscreen and switching to windowed.
  RESOLUTION_INFO resInfo = CDisplaySettings::GetInstance().GetResolutionInfo(RES_WINDOW);
  m_nWidth  = resInfo.iWidth;
  m_nHeight = resInfo.iHeight;
  m_bFullScreen = false;

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

  if (CDisplaySettings::GetInstance().GetCurrentResolution() != RES_WINDOW)
  {
    // If we are not starting up windowed, then hide the initial SDL window
    // so we do not see it flash before the fade-out and switch to fullscreen.
    ShowHideNSWindow([view window], false);
  }

  // disassociate view from context
  [cur_context clearDrawable];

  // release the context
  if (CWinSystemOSXImpl::m_lastOwnedContext == cur_context)
  {
    [ NSOpenGLContext clearCurrentContext ];
    [ cur_context clearDrawable ];
    cur_context = nil;
  }

  // create a new context
  auto new_context = CreateWindowedContext(nil);
  if (!new_context)
    return false;

  // associate with current view
  [new_context setView:view];
  [new_context makeCurrentContext];

  // set the window title
  [[[new_context view] window]
      setTitle:[NSString stringWithFormat:@"%s Media Center", CCompileInfo::GetAppName()]];

  m_impl->m_glContext = new_context;
  CWinSystemOSXImpl::m_lastOwnedContext = new_context;
  m_bWindowCreated = true;

  // get screen refreshrate - this is needed
  // when we startup in windowed mode and don't run through SetFullScreen
  int dummy;
  GetScreenResolution(&dummy, &dummy, &m_refreshRate, m_lastDisplayNr);

  // register platform dependent objects
  CDVDFactoryCodec::ClearHWAccels();
  VTB::CDecoder::Register();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CLinuxRendererGL::Register();
  CRendererVTB::Register();
  VIDEOPLAYER::CProcessInfoOSX::Register();
  RETRO::CRPProcessInfoOSX::Register();
  RETRO::CRPProcessInfoOSX::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGL);
  CScreenshotSurfaceGL::Register();

  return true;
}

bool CWinSystemOSX::DestroyWindow()
{
  return true;
}

extern "C" void SDL_SetWidthHeight(int w, int h);
void ResizeWindowInternal(int newWidth, int newHeight, int newLeft, int newTop, NSView* last_view)
{
  if (last_view && [last_view window])
  {
    auto size = NSMakeSize(newWidth, newHeight);
    NSWindow* lastWindow = [last_view window];
    [lastWindow setContentSize:size];
    [lastWindow update];
    [last_view setFrameSize:size];
  }
}
bool CWinSystemOSX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (!m_impl->m_glContext)
    return false;

  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  NSView* view;
  NSWindow* window;

  view = [context view];

  if (view)
  {
    // It seems, that in macOS 10.15 this defaults to YES, but we currently do not support
    // Retina resolutions properly. Ensure that the view uses a 1 pixel per point framebuffer.
    view.wantsBestResolutionOpenGLSurface = NO;
  }

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
      // to windowed on screen 1 directly where in ScreenChangedNotification
      // we don't have a window to get the current screen on
      // in that case ResizeWindow is called at a later stage from SetFullScreen(false)
      // and we can grab the correct display number here then
      m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen( [window screen] ));
    }
  }

  // HACK: resize SDL's view manually so that mouse bounds are correctly updated.
  // there are two parts to this, the internal SDL (current_video->screen) and
  // the cocoa view ( handled in SetFullScreen).
  SDL_SetWidthHeight(newWidth, newHeight);

  [context makeCurrentContext];

  m_nWidth = newWidth;
  m_nHeight = newHeight;
  m_impl->m_glContext = context;
  CServiceBroker::GetWinSystem()->GetGfxContext().SetFPS(m_refreshRate);

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
    RESOLUTION_INFO& window = CDisplaySettings::GetInstance().GetResolutionInfo(RES_WINDOW);
    CWinSystemOSX::SetFullScreen(false, window, blankOtherDisplays);
    needtoshowme = true;
  }

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_lastDisplayNr = GetDisplayIndex(settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  cur_context = [NSOpenGLContext currentContext];

  //handle resolution/refreshrate switching early here
  if (m_bFullScreen)
  {
    // switch videomode
    SwitchToVideoMode(res.iWidth, res.iHeight, res.fRefreshRate);
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
    windowedFullScreenwindow = nil;
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

    // This is Cocoa Windowed FullScreen Mode
    // Get the screen rect of our current display
    NSScreen* pScreen = [[NSScreen screens] objectAtIndex:m_lastDisplayNr];
    NSRect    screenRect = [pScreen frame];

    // remove frame origin offset of original display
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
    newContext = CreateWindowedContext(cur_context);
    [newContext setView:blankView];

    // Hide the menu bar.
    SetMenuBarVisible(false);

    // Blank other displays if requested.
    if (blankOtherDisplays)
      BlankOtherDisplays(m_lastDisplayNr);

    // Hide the mouse.
    [NSCursor hide];

    // Release old context if we created it.
    if (CWinSystemOSXImpl::m_lastOwnedContext == cur_context)
    {
      [ NSOpenGLContext clearCurrentContext ];
      [ cur_context clearDrawable ];
    }

    // activate context
    [newContext makeCurrentContext];
    CWinSystemOSXImpl::m_lastOwnedContext = newContext;
  }
  else
  {
    // Windowed Mode
    // exit fullscreen
    [cur_context clearDrawable];

    [NSCursor unhide];

    // Show menubar.
    SetMenuBarVisible(true);

    // restore the windowed window level
    [[last_view window] setLevel:last_window_level];

    // Get rid of the new window we created.
    if (windowedFullScreenwindow != nil)
    {
      [windowedFullScreenwindow close];
      windowedFullScreenwindow = nil;
    }

    // Unblank.
    // Force the unblank when returning from fullscreen, we get called with blankOtherDisplays set false.
    //if (blankOtherDisplays)
    UnblankDisplays();

    // create our new context (sharing with the current one)
    auto newContext = CreateWindowedContext(cur_context);
    if (!newContext)
      return false;

    // Assign view from old context, move back to original screen.
    [newContext setView:last_view];
    [[last_view window] setFrameOrigin:last_window_origin];
    // return the mouse bounds in SDL view to previous size
    [ last_view setFrameSize:last_view_size ];
    [ last_view setFrameOrigin:last_view_origin ];
    // done with restoring windowed window, don't set last_view to NULL as we can lose it under dual displays.
    //last_window_screen = NULL;

    // Release the fullscreen context.
    if (CWinSystemOSXImpl::m_lastOwnedContext == cur_context)
    {
      [ NSOpenGLContext clearCurrentContext ];
      [ cur_context clearDrawable ];
    }

    // Activate context.
    [newContext makeCurrentContext];
    CWinSystemOSXImpl::m_lastOwnedContext = newContext;
  }

  DisplayFadeFromBlack(fade_token, needtoshowme);

  ShowHideNSWindow([last_view window], needtoshowme);
  // need to make sure SDL tracks any window size changes
  ResizeWindow(m_nWidth, m_nHeight, -1, -1);
  ResizeWindowInternal(m_nWidth, m_nHeight, -1, -1, last_view);
  // restore origin once again when going to windowed mode
  if (!fullScreen)
  {
    [[last_view window] setFrameOrigin:last_window_origin];
  }
  HandlePossibleRefreshrateChange();

  m_updateGLContext = 0;
  return true;
}

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  // Add desktop resolution
  int w, h;
  double fps;

  int dispIdx = GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  GetScreenResolution(&w, &h, &fps, dispIdx);
  NSString *dispName = screenNameForDisplay(GetDisplayID(dispIdx));  
  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP), [dispName UTF8String], w, h, fps, 0);

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  // now just fill in the possible resolutions for the attached screens
  // and push to the resolution info vector
  FillInVideoModes();
  CDisplaySettings::GetInstance().ApplyCalibrations();
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

NSOpenGLContext* CreateWindowedContext(NSOpenGLContext* shareCtx)
{
  NSOpenGLPixelFormat* pixFmt;
  if (getenv("KODI_GL_PROFILE_LEGACY"))
  {
    NSOpenGLPixelFormatAttribute wattrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADepthSize,
        static_cast<NSOpenGLPixelFormatAttribute>(8),
        static_cast<NSOpenGLPixelFormatAttribute>(0)};
    pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];
  }
  else
  {
    NSOpenGLPixelFormatAttribute wattrs_gl3[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADepthSize,
        static_cast<NSOpenGLPixelFormatAttribute>(24),
        static_cast<NSOpenGLPixelFormatAttribute>(0)};
    pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs_gl3];
  }

  auto newContext = [[NSOpenGLContext alloc] initWithFormat:pixFmt shareContext:shareCtx];

  if (!newContext)
  {
    // bah, try again for non-accelerated renderer
    NSOpenGLPixelFormatAttribute wattrs2[] =
    {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFANoRecovery,
      NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)8,
      (NSOpenGLPixelFormatAttribute)0
    };
    newContext = [[NSOpenGLContext alloc]
        initWithFormat:[[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs2]
          shareContext:shareCtx];
  }

  return newContext;
}

NSOpenGLContext* CreateFullScreenContext(int screen_index, NSOpenGLContext* shareCtx)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;
  CGDirectDisplayID displayID;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  displayID = displayArray[screen_index];

  NSOpenGLPixelFormat* pixFmt;
  if (getenv("KODI_GL_PROFILE_LEGACY"))
  {
    NSOpenGLPixelFormatAttribute fsattrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADepthSize,
        static_cast<NSOpenGLPixelFormatAttribute>(8),
        NSOpenGLPFAScreenMask,
        static_cast<NSOpenGLPixelFormatAttribute>(CGDisplayIDToOpenGLDisplayMask(displayID)),
        static_cast<NSOpenGLPixelFormatAttribute>(0)};
    pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs];
  }
  else
  {
    NSOpenGLPixelFormatAttribute fsattrs_gl3[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADepthSize,
        static_cast<NSOpenGLPixelFormatAttribute>(24),
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAScreenMask,
        static_cast<NSOpenGLPixelFormatAttribute>(CGDisplayIDToOpenGLDisplayMask(displayID)),
        static_cast<NSOpenGLPixelFormatAttribute>(0)};
    pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs_gl3];
  }

  if (!pixFmt)
    return nil;

  auto newContext = [[NSOpenGLContext alloc] initWithFormat:pixFmt shareContext:shareCtx];

  return newContext;
}

void CWinSystemOSX::GetScreenResolution(int* w, int* h, double* fps, int screenIdx)
{
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

bool CWinSystemOSX::SwitchToVideoMode(int width, int height, double refreshrate)
{
  boolean_t match = false;
  CGDisplayModeRef dispMode = NULL;

  int screenIdx = GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));

  // Figure out the screen size. (default to main screen)
  CGDirectDisplayID display_id = GetDisplayID(screenIdx);

  // find mode that matches the desired size, refreshrate
  // non interlaced, nonstretched, safe for hardware
  dispMode = GetMode(width, height, refreshrate, screenIdx);

  //not found - fallback to bestemdeforparameters
  if (!dispMode)
  {
    dispMode = BestMatchForMode(display_id, 32, width, height, match);

    if (!match)
      dispMode = BestMatchForMode(display_id, 16, width, height, match);

    // still no match? fallback to current resolution of the display which HAS to work [tm]
    if (!match)
    {
      int tmpWidth;
      int tmpHeight;
      double tmpRefresh;

      GetScreenResolution(&tmpWidth, &tmpHeight, &tmpRefresh, screenIdx);
      dispMode = GetMode(tmpWidth, tmpHeight, tmpRefresh, screenIdx);

      // no way to get a resolution set
      if (!dispMode)
        return false;
    }

    if (!match)
      return false;
  }

  // switch mode and return success
  CGDisplayCapture(display_id);
  CGDisplayConfigRef cfg;
  CGBeginDisplayConfiguration(&cfg);
  CGConfigureDisplayWithDisplayMode(cfg, display_id, dispMode, nullptr);
  CGError err = CGCompleteDisplayConfiguration(cfg, kCGConfigureForAppOnly);
  CGDisplayRelease(display_id);

  m_refreshRate = CGDisplayModeGetRefreshRate(dispMode);

  Cocoa_CVDisplayLinkUpdate();

  return (err == kCGErrorSuccess);
}

void CWinSystemOSX::FillInVideoModes()
{
  int dispIdx = GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));

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

    CFArrayRef displayModes = GetAllDisplayModes(GetDisplayID(disp));
    NSString *dispName = screenNameForDisplay(GetDisplayID(disp));

    CLog::Log(LOGNOTICE, "Display %i has name %s", disp, [dispName UTF8String]);

    if (NULL == displayModes)
      continue;

    for (int i=0; i < CFArrayGetCount(displayModes); ++i)
    {
      CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, i);

      uint32_t flags = CGDisplayModeGetIOFlags(displayMode);
      stretched = flags & kDisplayModeStretchedFlag ? true : false;
      interlaced = flags & kDisplayModeInterlacedFlag ? true : false;
      bitsperpixel = DisplayBitsPerPixelForMode(displayMode);
      safeForHardware = flags & kDisplayModeSafetyFlags ? true : false;
      televisionoutput = flags & kDisplayModeTelevisionFlag ? true : false;

      if ((bitsperpixel == 32) &&
          (safeForHardware == YES) &&
          (stretched == NO) &&
          (interlaced == NO))
      {
        w = CGDisplayModeGetWidth(displayMode);
        h = CGDisplayModeGetHeight(displayMode);
        refreshrate = CGDisplayModeGetRefreshRate(displayMode);
        if ((int)refreshrate == 0)  // LCD display?
        {
          // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
          refreshrate = 60.0;
        }
        CLog::Log(LOGNOTICE, "Found possible resolution for display %d with %d x %d @ %f Hz", disp, w, h, refreshrate);

        // only add the resolution if it belongs to "our" screen
        // all others are only logged above...
        if (disp == dispIdx)
        {
          UpdateDesktopResolution(res, (dispName != nil) ? [dispName UTF8String] : "Unknown", w, h, refreshrate, 0);
          CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
          CDisplaySettings::GetInstance().AddResolutionInfo(res);
        }
      }
    }
    CFRelease(displayModes);
  }
}

bool CWinSystemOSX::FlushBuffer(void)
{
  if (m_updateGLContext < 5)
  {
    [m_impl->m_glContext update];
    m_updateGLContext++;
  }

  [m_impl->m_glContext flushBuffer];

  return true;
}

bool CWinSystemOSX::IsObscured(void)
{
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
    // not visible means the window is not showing.
    // this should never really happen as we are always visible
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
    // kicking in. Unfortunately even the alpha of these windows is 1.0 so
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
            CLog::Log(LOGDEBUG, "WinSystemOSX: Fullscreen window %s obscures Kodi!",
                      appName.c_str());
          obscureLogged = true;
        }
        m_obscured = true;
        break;
      }

      // handle overlapping windows above us that combine
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
    if (rects.empty())
      m_obscured = true;
  }

  CFRelease(windowDescs);
  CFRelease(windowIDs);

  return m_obscured;
}

void CWinSystemOSX::NotifyAppFocusChange(bool bGaining)
{
  if (!(m_bFullScreen && bGaining))
    return;
  @autoreleasepool
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
          SetMenuBarVisible(false);
          [window orderFront:nil];
        }
      }
    }
  }
}

void CWinSystemOSX::ShowOSMouse(bool show)
{
  SDL_ShowCursor(show ? 1 : 0);
}

bool CWinSystemOSX::Minimize()
{
  @autoreleasepool
  {
    [[NSApplication sharedApplication] miniaturizeAll:nil];
  }
  return true;
}

bool CWinSystemOSX::Restore()
{
  @autoreleasepool
  {
    [[NSApplication sharedApplication] unhide:nil];
  }
  return true;
}

bool CWinSystemOSX::Hide()
{
  @autoreleasepool
  {
    [[NSApplication sharedApplication] hide:nil];
  }
  return true;
}

void CWinSystemOSX::HandlePossibleRefreshrateChange()
{
  static double oldRefreshRate = m_refreshRate;
  Cocoa_CVDisplayLinkUpdate();
  int dummy = 0;

  GetScreenResolution(&dummy, &dummy, &m_refreshRate, m_lastDisplayNr);

  if (oldRefreshRate != m_refreshRate)
  {
    oldRefreshRate = m_refreshRate;
    // send a message so that videoresolution (and refreshrate)
    // is changed
    CApplicationMessenger::GetInstance().PostMsg(TMSG_VIDEORESIZE, m_SDLSurface->w, m_SDLSurface->h);
  }
}

void CWinSystemOSX::OnMove(int x, int y)
{
  HandlePossibleRefreshrateChange();
}

std::unique_ptr<IOSScreenSaver> CWinSystemOSX::GetOSScreenSaverImpl()
{
  return std::unique_ptr<IOSScreenSaver> (new COSScreenSaverOSX);
}

OSXTextInputResponder *g_textInputResponder = nil;

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
  if (g_textInputResponder)
  {
    [g_textInputResponder removeFromSuperview];
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
  @autoreleasepool
  {
    auto app = [NSApplication sharedApplication];
    if (raise)
    {
      [app unhide:nil];
      [app activateIgnoringOtherApps:YES];
      [app arrangeInFront:nil];
    }
    else
    {
      [app unhideWithoutActivation];
    }
  }
  return true;
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
        m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen([window screen]));
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
  for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnLostDisplay();
}

void CWinSystemOSX::HandleOnResetDevice()
{

  int delay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay * 100);
  }
  else
  {
    AnnounceOnResetDevice();
  }
}

void CWinSystemOSX::AnnounceOnResetDevice()
{
  double currentFps = m_refreshRate;
  int w = 0;
  int h = 0;
  int currentScreenIdx = m_lastDisplayNr;
  // ensure that graphics context knows about the current refreshrate before
  // doing the callbacks
  GetScreenResolution(&w, &h, &currentFps, currentScreenIdx);

  CServiceBroker::GetWinSystem()->GetGfxContext().SetFPS(currentFps);

  CSingleLock lock(m_resourceSection);
  // tell any shared resources
  CLog::Log(LOGDEBUG, "CWinSystemOSX::AnnounceOnResetDevice");
  for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnResetDisplay();
}

void* CWinSystemOSX::GetCGLContextObj()
{
  return [m_impl->m_glContext CGLContextObj];
}

NSOpenGLContext* CWinSystemOSX::GetNSOpenGLContext()
{
  return m_impl->m_glContext;
}

std::string CWinSystemOSX::GetClipboardText(void)
{
  std::string utf8_text;

  const char *szStr = Cocoa_Paste();
  if (szStr)
    utf8_text = szStr;

  return utf8_text;
}

std::unique_ptr<CVideoSync> CWinSystemOSX::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncOsx(clock));
  return pVSync;
}

bool CWinSystemOSX::MessagePump()
{
  return m_winEvents->MessagePump();
}

void CWinSystemOSX::GetConnectedOutputs(std::vector<std::string> *outputs)
{
  outputs->push_back("Default");

  int numDisplays = [[NSScreen screens] count];

  for (int disp = 0; disp < numDisplays; disp++)
  {
    NSString *dispName = screenNameForDisplay(GetDisplayID(disp));
    outputs->push_back([dispName UTF8String]);
  }
}
