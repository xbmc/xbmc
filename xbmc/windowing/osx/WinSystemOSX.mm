/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemOSX.h"

#include "AppInboundProtocol.h"
#include "ServiceBroker.h"
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
#include "messaging/ApplicationMessenger.h"
#include "rendering/gl/ScreenshotSurfaceGL.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/osx/CocoaDPMSSupport.h"
#include "windowing/osx/OSScreenSaverOSX.h"
#include "windowing/osx/VideoSyncOsx.h"
#include "windowing/osx/WinEventsOSX.h"

#include "platform/darwin/DarwinUtils.h"
#include "platform/darwin/DictionaryUtils.h"
#include "platform/darwin/osx/CocoaInterface.h"
#import "platform/darwin/osx/OSXGLView.h"
#import "platform/darwin/osx/OSXGLWindow.h"
#include "platform/darwin/osx/powermanagement/CocoaPowerSyscall.h"

#include <chrono>
#include <cstdlib>
#include <signal.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <QuartzCore/QuartzCore.h>

using namespace KODI;
using namespace MESSAGING;
using namespace WINDOWING;

#define MAX_DISPLAYS 32
static NSWindow* blankingWindows[MAX_DISPLAYS];

//---------------------------------------------------------------------------------
void SetMenuBarVisible(bool visible)
{
  if (visible)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      NSApplicationPresentationOptions options = NSApplicationPresentationDefault;
      [NSApplication.sharedApplication setPresentationOptions:options];
    });
  }
  else
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      NSApplicationPresentationOptions options =
          NSApplicationPresentationHideMenuBar | NSApplicationPresentationHideDock;
      [NSApplication.sharedApplication setPresentationOptions:options];
    });
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// No replacement for CGDisplayModeCopyPixelEncoding
// Disable warning for now
size_t DisplayBitsPerPixelForMode(CGDisplayModeRef mode)
{
  size_t bitsPerPixel = 0;

  CFStringRef pixEnc = CGDisplayModeCopyPixelEncoding(mode);
  if (CFStringCompare(pixEnc, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) ==
      kCFCompareEqualTo)
  {
    bitsPerPixel = 32;
  }
  else if (CFStringCompare(pixEnc, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) ==
           kCFCompareEqualTo)
  {
    bitsPerPixel = 16;
  }
  else if (CFStringCompare(pixEnc, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) ==
           kCFCompareEqualTo)
  {
    bitsPerPixel = 8;
  }

  CFRelease(pixEnc);

  return bitsPerPixel;
}
#pragma GCC diagnostic pop

#pragma mark - GetScreenName

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// No real replacement of CGDisplayIOServicePort
// Stackoverflow links to https://github.com/glfw/glfw/pull/192 as a possible replacement
// disable warning for now
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
#pragma GCC diagnostic pop

#pragma mark - GetDisplay

CGDirectDisplayID GetDisplayID(int screen_index)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  if (screen_index >= 0 && screen_index < numDisplays)
    return (displayArray[screen_index]);
  else
    return (displayArray[0]);
}

CGDirectDisplayID GetDisplayIDFromScreen(NSScreen* screen)
{
  NSDictionary* screenInfo = screen.deviceDescription;
  NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];

  return (CGDirectDisplayID)[screenID longValue];
}

int GetDisplayIndex(CGDirectDisplayID display)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  while (numDisplays > 0)
  {
    if (display == displayArray[--numDisplays])
      return numDisplays;
  }
  return -1;
}

int GetDisplayIndex(const std::string& dispName)
{
  int ret = 0;

  // Add full screen settings for additional monitors
  int numDisplays = NSScreen.screens.count;

  for (int disp = 0; disp < numDisplays; disp++)
  {
    NSString* name = screenNameForDisplay(GetDisplayID(disp));
    if (name.UTF8String == dispName)
    {
      ret = disp;
      break;
    }
  }

  return ret;
}

#pragma mark - Display Modes

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
  CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&key,
                                               (const void**)&number, 1, NULL, NULL);
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

// try to find mode that matches the desired size, refreshrate
// non interlaced, nonstretched, safe for hardware
CGDisplayModeRef GetMode(int width, int height, double refreshrate, int screenIdx)
{
  if (screenIdx >= (signed)[[NSScreen screens] count])
    return NULL;

  bool stretched;
  bool interlaced;
  bool safeForHardware;
  bool televisionoutput;
  int w, h, bitsperpixel;
  double rate;
  RESOLUTION_INFO res;

  CLog::Log(LOGDEBUG, "GetMode looking for suitable mode with %d x %d @ %f Hz on display %d", width,
            height, refreshrate, screenIdx);

  CFArrayRef displayModes = GetAllDisplayModes(GetDisplayID(screenIdx));

  if (!displayModes)
    return NULL;

  for (int i = 0; i < CFArrayGetCount(displayModes); ++i)
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

    if ((bitsperpixel == 32) && (safeForHardware == true) && (stretched == false) &&
        (interlaced == false) && (w == width) && (h == height) &&
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

// mimic former behavior of deprecated CGDisplayBestModeForParameters
CGDisplayModeRef BestMatchForMode(
    CGDirectDisplayID display, size_t bitsPerPixel, size_t width, size_t height, boolean_t& match)
{

  // Get a copy of the current display mode
  CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(display);

  // Loop through all display modes to determine the closest match.
  // CGDisplayBestModeForParameters is deprecated on 10.6 so we will emulate it's behavior
  // Try to find a mode with the requested depth and equal or greater dimensions first.
  // If no match is found, try to find a mode with greater depth and same or greater dimensions.
  // If still no match is found, just use the current mode.
  CFArrayRef allModes = GetAllDisplayModes(display);

  for (int i = 0; i < CFArrayGetCount(allModes); i++)
  {
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);

    if (DisplayBitsPerPixelForMode(mode) != bitsPerPixel)
      continue;

    if ((CGDisplayModeGetWidth(mode) == width) && (CGDisplayModeGetHeight(mode) == height))
    {
      CGDisplayModeRelease(displayMode); // release the copy we got before ...
      displayMode = mode;
      match = true;
      break;
    }
  }

  // No depth match was found
  if (!match)
  {
    for (int i = 0; i < CFArrayGetCount(allModes); i++)
    {
      CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
      if (DisplayBitsPerPixelForMode(mode) >= bitsPerPixel)
        continue;

      if ((CGDisplayModeGetWidth(mode) == width) && (CGDisplayModeGetHeight(mode) == height))
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

#pragma mark - Blank Displays

void BlankOtherDisplays(int screen_index)
{
  int i;
  int numDisplays = [[NSScreen screens] count];

  // zero out blankingWindows for debugging
  for (i = 0; i < MAX_DISPLAYS; i++)
  {
    blankingWindows[i] = 0;
  }

  // Blank.
  for (i = 0; i < numDisplays; i++)
  {
    if (i != screen_index)
    {
      // Get the size.
      NSScreen* pScreen = [NSScreen.screens objectAtIndex:i];
      NSRect screenRect = pScreen.frame;

      // Build a blanking window.
      screenRect.origin = NSZeroPoint;
      blankingWindows[i] = [[NSWindow alloc] initWithContentRect:screenRect
                                                       styleMask:NSWindowStyleMaskBorderless
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO
                                                          screen:pScreen];

      [blankingWindows[i] setBackgroundColor:NSColor.blackColor];
      [blankingWindows[i] setLevel:CGShieldingWindowLevel()];
      [blankingWindows[i] makeKeyAndOrderFront:nil];
    }
  }
}

void UnblankDisplays(void)
{
  int numDisplays = NSScreen.screens.count;
  int i = 0;

  for (i = 0; i < numDisplays; i++)
  {
    if (blankingWindows[i] != 0)
    {
      // Get rid of the blanking windows we created.
      [blankingWindows[i] close];
      blankingWindows[i] = 0;
    }
  }
}

#pragma mark - Fade Display

static NSWindow* curtainWindow;
void fadeInDisplay(NSScreen* theScreen, double fadeTime)
{
  int fadeSteps = 100;
  double fadeInterval = (fadeTime / (double)fadeSteps);

  if (curtainWindow != nil)
  {
    for (int step = 0; step < fadeSteps; step++)
    {
      double fade = 1.0 - (step * fadeInterval);
      [curtainWindow setAlphaValue:fade];

      NSDate* nextDate = [NSDate dateWithTimeIntervalSinceNow:fadeInterval];
      [NSRunLoop.currentRunLoop runUntilDate:nextDate];
    }
  }
  [curtainWindow close];
  curtainWindow = nil;
}

void fadeOutDisplay(NSScreen* theScreen, double fadeTime)
{
  int fadeSteps = 100;
  double fadeInterval = (fadeTime / (double)fadeSteps);

  [NSCursor hide];

  curtainWindow = [[NSWindow alloc] initWithContentRect:[theScreen frame]
                                              styleMask:NSWindowStyleMaskBorderless
                                                backing:NSBackingStoreBuffered
                                                  defer:YES
                                                 screen:theScreen];

  [curtainWindow setAlphaValue:0.0];
  [curtainWindow setBackgroundColor:NSColor.blackColor];
  [curtainWindow setLevel:NSScreenSaverWindowLevel];

  [curtainWindow makeKeyAndOrderFront:nil];
  [curtainWindow setFrame:[curtainWindow frameRectForContentRect:[theScreen frame]]
                  display:YES
                  animate:NO];

  for (int step = 0; step < fadeSteps; step++)
  {
    double fade = step * fadeInterval;
    [curtainWindow setAlphaValue:fade];

    NSDate* nextDate = [NSDate dateWithTimeIntervalSinceNow:fadeInterval];
    [NSRunLoop.currentRunLoop runUntilDate:nextDate];
  }
}

//---------------------------------------------------------------------------------
static void DisplayReconfigured(CGDirectDisplayID display,
                                CGDisplayChangeSummaryFlags flags,
                                void* userData)
{
  CWinSystemOSX* winsys = (CWinSystemOSX*)userData;
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

    if (screenIdx < NSScreen.screens.count)
    {
      pScreen = [NSScreen.screens objectAtIndex:screenIdx];
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
}

#pragma mark - CWinSystemOSX
//------------------------------------------------------------------------------
CWinSystemOSX::CWinSystemOSX() : CWinSystemBase(), m_lostDeviceTimer(this)
{
  m_appWindow = nullptr;
  m_glView = nullptr;
  m_obscured = false;
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

void CWinSystemOSX::Register(IDispResource* resource)
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

void CWinSystemOSX::AnnounceOnLostDevice()
{
  CSingleLock lock(m_resourceSection);
  // tell any shared resources
  CLog::Log(LOGDEBUG, "CWinSystemOSX::AnnounceOnLostDevice");
  for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnLostDisplay();
}

void CWinSystemOSX::HandleOnResetDevice()
{

  int delay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      "videoscreen.delayrefreshchange");
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
  for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnResetDisplay();
}

#pragma mark - Timers

void CWinSystemOSX::StartLostDeviceTimer()
{
  if (m_lostDeviceTimer.IsRunning())
    m_lostDeviceTimer.Restart();
  else
    m_lostDeviceTimer.Start(std::chrono::milliseconds(3000), false);
}

void CWinSystemOSX::StopLostDeviceTimer()
{
  m_lostDeviceTimer.Stop();
}

void CWinSystemOSX::OnTimeout()
{
  HandleOnResetDevice();
}

#pragma mark - WindowSystem

bool CWinSystemOSX::InitWindowSystem()
{
  if (!CWinSystemBase::InitWindowSystem())
    return false;

  CGDisplayRegisterReconfigurationCallback(DisplayReconfigured, (void*)this);

  return true;
}

bool CWinSystemOSX::DestroyWindowSystem()
{
  CGDisplayRemoveReconfigurationCallback(DisplayReconfigured, (void*)this);

  DestroyWindowInternal();

  if (m_glView)
  {
    m_glView = NULL;
  }

  UnblankDisplays();
  return true;
}

bool CWinSystemOSX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  // force initial window creation to be windowed, if fullscreen, it will switch to it below
  // fixes the white screen of death if starting fullscreen and switching to windowed.
  RESOLUTION_INFO resInfo = CDisplaySettings::GetInstance().GetResolutionInfo(RES_WINDOW);
  m_nWidth = resInfo.iWidth;
  m_nHeight = resInfo.iHeight;
  m_bFullScreen = false;
  m_name = name;

  __block NSWindow* appWindow;
  // because we are not main thread, delay any updates
  // and only become keyWindow after it finishes.
  [NSAnimationContext beginGrouping];
  [NSAnimationContext.currentContext setCompletionHandler:^{
    [appWindow makeKeyWindow];
  }];

  // for native fullscreen we always want to set the
  // same windowed flags
  __block NSUInteger windowStyleMask;
  if (fullScreen)
    windowStyleMask = NSWindowStyleMaskBorderless;
  else
    windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                      NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

  if (m_appWindow == nullptr)
  {
    // create new content view
    NSRect rect = [appWindow contentRectForFrameRect:appWindow.frame];

    // create new view if we don't have one
    if (!m_glView)
      m_glView = [[OSXGLView alloc] initWithFrame:rect];

    OSXGLView* view = (OSXGLView*)m_glView;

    dispatch_sync(dispatch_get_main_queue(), ^{
      appWindow = [[OSXGLWindow alloc] initWithContentRect:NSMakeRect(0, 0, m_nWidth, m_nHeight)
                                                 styleMask:windowStyleMask];
      NSString* title = [NSString stringWithUTF8String:m_name.c_str()];
      appWindow.backgroundColor = NSColor.blackColor;
      appWindow.title = title;
      [appWindow setOneShot:NO];

      NSWindowCollectionBehavior behavior = appWindow.collectionBehavior;
      behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
      [appWindow setCollectionBehavior:behavior];

      // associate with current window
      [appWindow setContentView:view];
    });

    [view.getGLContext makeCurrentContext];
    [view.getGLContext update];

    m_appWindow = appWindow;
    m_bWindowCreated = true;
  }

  // warning, we can order front but not become
  // key window or risk starting up with bad flicker
  // becoming key window must happen in completion block.
  [(NSWindow*)m_appWindow performSelectorOnMainThread:@selector(orderFront:)
                                           withObject:nil
                                        waitUntilDone:YES];

  [NSAnimationContext endGrouping];

  if (fullScreen)
  {
    m_fullscreenWillToggle = true;
    [appWindow performSelectorOnMainThread:@selector(toggleFullScreen:)
                                withObject:nil
                             waitUntilDone:YES];
  }

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

bool CWinSystemOSX::DestroyWindowInternal()
{
  // set this 1st, we should really mutex protext m_appWindow in this class
  m_bWindowCreated = false;
  if (m_appWindow)
  {
    NSWindow* oldAppWindow = m_appWindow;
    m_appWindow = NULL;
    dispatch_sync(dispatch_get_main_queue(), ^{
      [oldAppWindow setContentView:nil];
    });
  }

  return true;
}

bool CWinSystemOSX::DestroyWindow()
{
  return true;
}

bool CWinSystemOSX::Minimize()
{
  @autoreleasepool
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [NSApplication.sharedApplication miniaturizeAll:nil];
    });
  }
  return true;
}

bool CWinSystemOSX::Restore()
{
  @autoreleasepool
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [NSApplication.sharedApplication unhide:nil];
    });
  }
  return true;
}

bool CWinSystemOSX::Show(bool raise)
{
  @autoreleasepool
  {
    auto app = NSApplication.sharedApplication;
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

bool CWinSystemOSX::Hide()
{
  @autoreleasepool
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [NSApplication.sharedApplication hide:nil];
    });
  }
  return true;
}

NSRect CWinSystemOSX::GetWindowDimensions()
{
  if (m_appWindow)
  {
    NSWindow* win = (NSWindow*)m_appWindow;
    NSRect frame = win.contentView.frame;
    return frame;
  }
}

#pragma mark - Resize Window

bool CWinSystemOSX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (!m_appWindow)
    return false;

  [(OSXGLWindow*)m_appWindow setResizeState:true];

  __block OSXGLView* view;
  dispatch_sync(dispatch_get_main_queue(), ^{
    view = m_appWindow.contentView;
  });

  if (view)
  {
    // It seems, that in macOS 10.15 this defaults to YES, but we currently do not support
    // Retina resolutions properly. Ensure that the view uses a 1 pixel per point framebuffer.
    dispatch_sync(dispatch_get_main_queue(), ^{
      view.wantsBestResolutionOpenGLSurface = NO;
    });
  }

  if (newWidth < 0)
  {
    newWidth = [(NSWindow*)m_appWindow minSize].width;
  }

  if (newHeight < 0)
  {
    newHeight = [(NSWindow*)m_appWindow minSize].height;
  }

  if (view)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      NSOpenGLContext* context = [view getGLContext];
      NSWindow* window = m_appWindow;

      NSRect pos = window.frame;

      NSRect myNewContentFrame = NSMakeRect(pos.origin.x, pos.origin.y, newWidth, newHeight);
      NSRect myNewWindowRect = [window frameRectForContentRect:myNewContentFrame];
      [window setFrame:myNewWindowRect display:TRUE];

      [context update];
    });
  }

  m_nWidth = newWidth;
  m_nHeight = newHeight;

  [(OSXGLWindow*)m_appWindow setResizeState:false];

  return true;
}

bool CWinSystemOSX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CSingleLock lock(m_critSection);

  static NSPoint last_window_origin;

  static NSSize last_view_size;
  static NSPoint last_view_origin;

  //  if (m_lastDisplayNr == -1)
  //    m_lastDisplayNr = res.iScreen;

  __block NSWindow* window = m_appWindow;
  __block OSXGLView* view;
  dispatch_sync(dispatch_get_main_queue(), ^{
    view = window.contentView;
  });

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_lastDisplayNr = GetDisplayIndex(settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  //handle resolution/refreshrate switching early here
  if (m_bFullScreen)
  {
    // switch videomode
    SwitchToVideoMode(res.iWidth, res.iHeight, static_cast<double>(res.fRefreshRate));
    // hide the OS mouse
    [NSCursor hide];
  }

  dispatch_sync(dispatch_get_main_queue(), ^{
    [window setAllowsConcurrentViewDrawing:NO];
  });

  if (m_fullscreenWillToggle)
  {
    ResizeWindow(m_nWidth, m_nHeight, -1, -1);
    m_fullscreenWillToggle = false;
    return true;
  }

  if (m_bFullScreen)
  {
    // Save info about the windowed context so we can restore it when returning to windowed.
    __block NSPoint block_last_window_origin;
    __block NSSize block_last_view_size;
    __block NSPoint block_last_view_origin;

    dispatch_sync(dispatch_get_main_queue(), ^{
      block_last_view_size = view.frame.size;
      block_last_view_origin = view.frame.origin;
      block_last_window_origin = window.frame.origin;
    });

    last_view_size = block_last_view_size;
    last_view_origin = block_last_view_origin;
    last_window_origin = block_last_window_origin;

    // This is Cocoa Windowed FullScreen Mode
    // Get the screen rect of our current display
    NSScreen* pScreen = [NSScreen.screens objectAtIndex:m_lastDisplayNr];
    NSRect screenRect = pScreen.frame;

    // remove frame origin offset of original display
    screenRect.origin = NSZeroPoint;

    window = m_appWindow;
    dispatch_sync(dispatch_get_main_queue(), ^{
      view = [window contentView];
      [view setFrameSize:NSMakeSize(m_nWidth, m_nHeight)];

      NSString* title = [NSString stringWithFormat:@"%s", ""];
      window.title = title;
    });

    // Hide the menu bar.
    SetMenuBarVisible(false);

    // Blank other displays if requested.
    if (blankOtherDisplays)
      BlankOtherDisplays(m_lastDisplayNr);

    dispatch_sync(dispatch_get_main_queue(), ^{
      [window setAllowsConcurrentViewDrawing:YES];
    });
  }
  else
  {
    // Show menubar.
    SetMenuBarVisible(true);

    // Unblank.
    // Force the unblank when returning from fullscreen, we get called with blankOtherDisplays set false.
    //if (blankOtherDisplays)
    UnblankDisplays();
  }

  //DisplayFadeFromBlack(fade_token, needtoshowme);

  m_fullscreenWillToggle = true;
  // toggle cocoa fullscreen mode
  if ([m_appWindow respondsToSelector:@selector(toggleFullScreen:)])
    [m_appWindow performSelectorOnMainThread:@selector(toggleFullScreen:)
                                  withObject:nil
                               waitUntilDone:YES];

  ResizeWindow(m_nWidth, m_nHeight, -1, -1);

  return true;
}

#pragma mark - Resolution

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  // Add desktop resolution
  int w, h;
  double fps;

  int dispIdx = GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_VIDEOSCREEN_MONITOR));
  GetScreenResolution(&w, &h, &fps, dispIdx);
  NSString* dispName = screenNameForDisplay(GetDisplayID(dispIdx));
  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP),
                          dispName.UTF8String, w, h, fps, 0);

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  // now just fill in the possible resolutions for the attached screens
  // and push to the resolution info vector
  FillInVideoModes();
  CDisplaySettings::GetInstance().ApplyCalibrations();
}

void CWinSystemOSX::GetScreenResolution(int* w, int* h, double* fps, int screenIdx)
{
  CGDirectDisplayID display_id = (CGDirectDisplayID)GetDisplayID(screenIdx);
  CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display_id);
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

#pragma mark - Video Modes

bool CWinSystemOSX::SwitchToVideoMode(int width, int height, double refreshrate)
{
  boolean_t match = false;
  CGDisplayModeRef dispMode = NULL;

  int screenIdx = GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_VIDEOSCREEN_MONITOR));

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
  int dispIdx = GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_VIDEOSCREEN_MONITOR));

  // Add full screen settings for additional monitors
  int numDisplays = NSScreen.screens.count;

  for (int disp = 0; disp < numDisplays; disp++)
  {
    bool stretched;
    bool interlaced;
    bool safeForHardware;
    bool televisionoutput;
    int w, h, bitsperpixel;
    double refreshrate;
    RESOLUTION_INFO res;

    CFArrayRef displayModes = GetAllDisplayModes(GetDisplayID(disp));
    NSString* dispName = screenNameForDisplay(GetDisplayID(disp));

    CLog::Log(LOGINFO, "Display %i has name %s", disp, [dispName UTF8String]);

    if (nullptr == displayModes)
      continue;

    for (int i = 0; i < CFArrayGetCount(displayModes); ++i)
    {
      CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, i);

      uint32_t flags = CGDisplayModeGetIOFlags(displayMode);
      stretched = flags & kDisplayModeStretchedFlag ? true : false;
      interlaced = flags & kDisplayModeInterlacedFlag ? true : false;
      bitsperpixel = DisplayBitsPerPixelForMode(displayMode);
      safeForHardware = flags & kDisplayModeSafetyFlags ? true : false;
      televisionoutput = flags & kDisplayModeTelevisionFlag ? true : false;

      if ((bitsperpixel == 32) && (safeForHardware == true) && (stretched == false) &&
          (interlaced == false))
      {
        w = CGDisplayModeGetWidth(displayMode);
        h = CGDisplayModeGetHeight(displayMode);
        refreshrate = CGDisplayModeGetRefreshRate(displayMode);
        if (static_cast<int>(refreshrate) == 0) // LCD display?
        {
          // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
          refreshrate = 60.0;
        }
        CLog::Log(LOGINFO, "Found possible resolution for display %d with %d x %d @ %f Hz", disp, w,
                  h, refreshrate);

        // only add the resolution if it belongs to "our" screen
        // all others are only logged above...
        if (disp == dispIdx)
        {
          UpdateDesktopResolution(res, (dispName != nil) ? [dispName UTF8String] : "Unknown", w, h,
                                  refreshrate, 0);
          CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
          CDisplaySettings::GetInstance().AddResolutionInfo(res);
        }
      }
    }
    CFRelease(displayModes);
  }
}

#pragma mark - Occlusion

bool CWinSystemOSX::IsObscured(void)
{
  if (m_obscured)
    CLog::Log(LOGDEBUG, "CWinSystemOSX::IsObscured(void) - TRUE");
  return m_obscured;
}

void CWinSystemOSX::SetOcclusionState(bool occluded)
{
  //  m_obscured = occluded;
  //  CLog::Log(LOGDEBUG, "CWinSystemOSX::SetOcclusionState(bool occluded) - %s", occluded ? "true":"false");
}

void CWinSystemOSX::NotifyAppFocusChange(bool bGaining)
{
  if (!(m_bFullScreen && bGaining))
    return;
  @autoreleasepool
  {
    // find the window
    NSOpenGLContext* context = NSOpenGLContext.currentContext;
    if (context)
    {
      NSView* view;

      view = context.view;
      if (view)
      {
        NSWindow* window;
        window = view.window;
        if (window)
        {
          SetMenuBarVisible(false);
          [window orderFront:nil];
        }
      }
    }
  }
}

#pragma mark - Window Move

void CWinSystemOSX::OnMove(int x, int y)
{
  static double oldRefreshRate = m_refreshRate;
  Cocoa_CVDisplayLinkUpdate();
  int dummy = 0;

  GetScreenResolution(&dummy, &dummy, &m_refreshRate, m_lastDisplayNr);

  if (oldRefreshRate != m_refreshRate)
  {
    oldRefreshRate = m_refreshRate;

    // send a message so that videoresolution (and refreshrate) is changed
    NSWindow* win = m_appWindow;
    NSRect frame = win.contentView.frame;
    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(
        TMSG_VIDEORESIZE, frame.size.width, frame.size.height);
  }
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

    view = context.view;
    if (view)
    {
      NSWindow* window;
      window = view.window;
      if (window)
      {
        m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen(window.screen));
      }
    }
  }
  if (m_lastDisplayNr == -1)
    m_lastDisplayNr = 0; // default to main screen
}

CGLContextObj CWinSystemOSX::GetCGLContextObj()
{
  CGLContextObj cglcontex = nullptr;
  if (m_appWindow)
  {
    OSXGLView* contentView = m_appWindow.contentView;
    cglcontex = contentView.getGLContext.CGLContextObj;
  }

  return cglcontex;
}

bool CWinSystemOSX::FlushBuffer(void)
{
  if (m_appWindow)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      OSXGLView* contentView = m_appWindow.contentView;
      NSOpenGLContext* glcontex = contentView.getGLContext;
      [glcontex flushBuffer];
    });
  }

  return true;
}

#pragma mark - Vsync

void CWinSystemOSX::EnableVSync(bool enable)
{
  // OpenGL Flush synchronised with vertical retrace
  GLint swapInterval = enable ? 1 : 0;
  [NSOpenGLContext.currentContext setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
}

std::unique_ptr<CVideoSync> CWinSystemOSX::GetVideoSync(void* clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncOsx(clock));
  return pVSync;
}

std::vector<std::string> CWinSystemOSX::GetConnectedOutputs()
{
  std::vector<std::string> outputs;
  outputs.push_back("Default");

  int numDisplays = [[NSScreen screens] count];

  for (int disp = 0; disp < numDisplays; disp++)
  {
    NSString* dispName = screenNameForDisplay(GetDisplayID(disp));
    outputs.push_back(dispName.UTF8String);
  }

  return outputs;
}

#pragma mark - OSScreenSaver

std::unique_ptr<IOSScreenSaver> CWinSystemOSX::GetOSScreenSaverImpl()
{
  return std::unique_ptr<IOSScreenSaver>(new COSScreenSaverOSX);
}

#pragma mark - Input

bool CWinSystemOSX::MessagePump()
{
  return m_winEvents->MessagePump();
}

void CWinSystemOSX::enableInputEvents()
{
  m_winEvents->enableInputEvents();
}

void CWinSystemOSX::disableInputEvents()
{
  m_winEvents->disableInputEvents();
}

std::string CWinSystemOSX::GetClipboardText(void)
{
  std::string utf8_text;

  const char* szStr = Cocoa_Paste();
  if (szStr)
    utf8_text = szStr;

  return utf8_text;
}

void CWinSystemOSX::ShowOSMouse(bool show)
{
}

#pragma mark - Unused

CGDisplayFadeReservationToken DisplayFadeToBlack(bool fade)
{
  // Fade to black to hide resolution-switching flicker and garbage.
  CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
  if (CGAcquireDisplayFadeReservation(5, &fade_token) == kCGErrorSuccess && fade)
    CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0,
                  TRUE);

  return (fade_token);
}

void DisplayFadeFromBlack(CGDisplayFadeReservationToken fade_token, bool fade)
{
  if (fade_token != kCGDisplayFadeReservationInvalidToken)
  {
    if (fade)
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0,
                    0.0, FALSE);
    CGReleaseDisplayFadeReservation(fade_token);
  }
}
