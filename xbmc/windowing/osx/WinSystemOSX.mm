/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemOSX.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
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
#include "threads/CriticalSection.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/osx/CocoaDPMSSupport.h"
#include "windowing/osx/OSScreenSaverOSX.h"
#import "windowing/osx/OpenGL/OSXGLView.h"
#import "windowing/osx/OpenGL/OSXGLWindow.h"
#include "windowing/osx/VideoSyncOsx.h"
#include "windowing/osx/WinEventsOSX.h"

#include "platform/darwin/DarwinUtils.h"
#include "platform/darwin/DictionaryUtils.h"
#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/osx/powermanagement/CocoaPowerSyscall.h"

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <signal.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <QuartzCore/QuartzCore.h>

using namespace KODI;
using namespace MESSAGING;
using namespace WINDOWING;
using namespace std::chrono_literals;

namespace
{
constexpr int MAX_DISPLAYS = 32;
constexpr const char* DEFAULT_SCREEN_NAME = "Default";
//! MacOS specific window top position setting
constexpr const char* SETTING_WINDOW_TOP = "window.top";
//! MacOS specific window left position setting
constexpr const char* SETTING_WINDOW_LEFT = "window.left";
} // namespace

static NSWindow* blankingWindows[MAX_DISPLAYS];

size_t DisplayBitsPerPixelForMode(CGDisplayModeRef mode)
{
  size_t bitsPerPixel = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  // No replacement for CGDisplayModeCopyPixelEncoding
  // Disable warning for now
  CFStringRef pixEnc = CGDisplayModeCopyPixelEncoding(mode);
#pragma GCC diagnostic pop

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

#pragma mark - GetDisplay

CGDirectDisplayID GetDisplayID(NSUInteger screen_index)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  if (screen_index >= 0 && screen_index < static_cast<NSUInteger>(numDisplays))
    return displayArray[screen_index];
  else
    return displayArray[0];
}

#pragma mark - GetScreenName

NSString* GetScreenName(NSUInteger screenIdx)
{
  NSString* screenName;
  const CGDirectDisplayID displayID = GetDisplayID(screenIdx);

  if (@available(macOS 10.15, *))
  {
    screenName = [NSScreen.screens objectAtIndex:screenIdx].localizedName;
  }
  else
  {
    //! TODO: Remove when 10.15 is the minimal target
    @autoreleasepool
    {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      // No real replacement of CGDisplayIOServicePort
      // Stackoverflow links to https://github.com/glfw/glfw/pull/192 as a possible replacement
      // disable warning for now
      NSDictionary* deviceInfo = (__bridge_transfer NSDictionary*)IODisplayCreateInfoDictionary(
          CGDisplayIOServicePort(displayID), kIODisplayOnlyPreferredName);

#pragma GCC diagnostic pop

      NSDictionary* localizedNames =
          [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];

      if ([localizedNames count] > 0)
      {
        screenName = [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];
      }
    }
  }
  return screenName;
}

NSString* screenNameForDisplay(NSUInteger screenIdx)
{
  // screen id 0 is always called "Default"
  if (screenIdx == 0)
  {
    return @(DEFAULT_SCREEN_NAME);
  }

  const CGDirectDisplayID displayID = GetDisplayID(screenIdx);
  NSString* screenName = GetScreenName(screenIdx);

  if (screenName == nil)
  {
    screenName = [[NSString alloc] initWithFormat:@"%u", displayID];
  }
  else
  {
    // ensure screen name is unique by appending displayid
    screenName = [screenName stringByAppendingFormat:@" (%@)", [@(displayID - 1) stringValue]];
  }

  return screenName;
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

NSUInteger GetDisplayIndex(const std::string& dispName)
{
  NSUInteger ret = 0;

  // Add full screen settings for additional monitors
  const NSUInteger numDisplays = NSScreen.screens.count;
  for (NSUInteger disp = 0; disp <= numDisplays - 1; disp++)
  {
    NSString* name = screenNameForDisplay(disp);
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
    return nullptr;
  }

  CFStringRef key = kCGDisplayShowDuplicateLowResolutionModes;
  CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&key,
                                               (const void**)&number, 1, nullptr, nullptr);
  CFRelease(number);

  if (!options)
  {
    CLog::Log(LOGERROR, "GetAllDisplayModes - could not create Dictionary!");
    return nullptr;
  }

  CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(display, options);
  CFRelease(options);

  if (!displayModes)
  {
    CLog::Log(LOGERROR, "GetAllDisplayModes - no displaymodes found!");
    return nullptr;
  }

  return displayModes;
}

// try to find mode that matches the desired size, refreshrate
// non interlaced, nonstretched, safe for hardware
CGDisplayModeRef GetMode(size_t width, size_t height, double refreshrate, NSUInteger screenIdx)
{
  if (screenIdx >= [[NSScreen screens] count])
    return nullptr;

  bool stretched;
  bool interlaced;
  bool safeForHardware;
  size_t w;
  size_t h;
  size_t bitsperpixel;
  double rate;
  RESOLUTION_INFO res;

  CLog::Log(LOGDEBUG, "GetMode looking for suitable mode with {} x {} @ {} Hz on display {}", width,
            height, refreshrate, screenIdx);

  CFArrayRef allModes = GetAllDisplayModes(GetDisplayID(screenIdx));

  if (!allModes)
    return nullptr;

  for (int i = 0; i < CFArrayGetCount(allModes); ++i)
  {
    CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
    uint32_t flags = CGDisplayModeGetIOFlags(displayMode);
    stretched = (flags & kDisplayModeStretchedFlag) != 0;
    interlaced = (flags & kDisplayModeInterlacedFlag) != 0;
    bitsperpixel = DisplayBitsPerPixelForMode(displayMode);
    safeForHardware = (flags & kDisplayModeSafetyFlags) != 0;
    w = CGDisplayModeGetWidth(displayMode);
    h = CGDisplayModeGetHeight(displayMode);
    rate = CGDisplayModeGetRefreshRate(displayMode);

    if ((bitsperpixel == 32) && (safeForHardware == true) && (stretched == false) &&
        (interlaced == false) && (w == width) && (h == height) &&
        (rate == refreshrate || rate == 0))
    {
      CFRelease(allModes);
      CLog::Log(LOGDEBUG, "GetMode found a match!");
      return CGDisplayModeRetain(displayMode);
    }
  }

  CFRelease(allModes);
  CLog::Log(LOGERROR, "GetMode - no match found!");
  return nullptr;
}

// mimic former behavior of deprecated CGDisplayBestModeForParameters
CGDisplayModeRef BestMatchForMode(CGDirectDisplayID display,
                                  size_t bitsPerPixel,
                                  size_t width,
                                  size_t height)
{
  // Loop through all display modes to determine the closest match.
  // CGDisplayBestModeForParameters is deprecated on 10.6 so we will emulate it's behavior
  // Try to find a mode with the requested depth and equal or greater dimensions first.
  // If no match is found, try to find a mode with greater depth and same or greater dimensions.
  // If still no match is found, just use the current mode.
  CFArrayRef allModes = GetAllDisplayModes(display);

  if (!allModes)
    return nullptr;

  CGDisplayModeRef displayMode = nullptr;

  for (int i = 0; i < CFArrayGetCount(allModes); i++)
  {
    CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);

    if (!mode)
      continue;

    if (DisplayBitsPerPixelForMode(mode) != bitsPerPixel)
      continue;

    if ((CGDisplayModeGetWidth(mode) == width) && (CGDisplayModeGetHeight(mode) == height))
    {
      displayMode = mode;
      break;
    }
  }

  // No depth match was found
  if (!displayMode)
  {
    for (int i = 0; i < CFArrayGetCount(allModes); i++)
    {
      CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);

      if (!mode)
        continue;

      if (DisplayBitsPerPixelForMode(mode) >= bitsPerPixel)
        continue;

      if ((CGDisplayModeGetWidth(mode) == width) && (CGDisplayModeGetHeight(mode) == height))
      {
        displayMode = mode;
        break;
      }
    }
  }

  CFRelease(allModes);

  return displayMode;
}

#pragma mark - Blank Displays

void BlankOtherDisplays(NSUInteger screen_index)
{
  const NSUInteger numDisplays = NSScreen.screens.count;

  // zero out blankingWindows for debugging
  for (NSUInteger i = 0; i < MAX_DISPLAYS; i++)
  {
    blankingWindows[i] = 0;
  }

  // Blank.
  for (NSUInteger i = 0; i < numDisplays; i++)
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
  for (auto i = 0; i < static_cast<int>(NSScreen.screens.count); i++)
  {
    if (blankingWindows[i] != 0)
    {
      // Get rid of the blanking windows we created.
      [blankingWindows[i] close];
      blankingWindows[i] = 0;
    }
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

  CLog::Log(LOGDEBUG, "CWinSystemOSX::DisplayReconfigured with flags {}", flags);

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
  m_lastDisplayNr = -1;
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
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemOSX::Unregister(IDispResource* resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemOSX::AnnounceOnLostDevice()
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  // tell any shared resources
  CLog::LogF(LOGDEBUG, "Lost Device Announce");
  for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnLostDisplay();
}

void CWinSystemOSX::HandleOnResetDevice()
{
  auto delay =
      std::chrono::milliseconds(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                    "videoscreen.delayrefreshchange") *
                                100);
  if (delay > 0ms)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay);
  }
  else
  {
    AnnounceOnResetDevice();
  }
}

void CWinSystemOSX::AnnounceOnResetDevice()
{
  double currentFps = m_refreshRate;
  size_t w = 0;
  size_t h = 0;
  const NSUInteger currentScreenIdx = m_lastDisplayNr;
  // ensure that graphics context knows about the current refreshrate before
  // doing the callbacks
  GetScreenResolution(&w, &h, &currentFps, currentScreenIdx);

  CServiceBroker::GetWinSystem()->GetGfxContext().SetFPS(currentFps);

  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  // tell any shared resources
  CLog::LogF(LOGDEBUG, "Reset Device Announce");
  for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnResetDisplay();
}

#pragma mark - Timers

void CWinSystemOSX::StartLostDeviceTimer()
{
  if (m_lostDeviceTimer.IsRunning())
    m_lostDeviceTimer.Restart();
  else
    m_lostDeviceTimer.Start(3000ms, false);
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
    m_glView = nullptr;
  }

  UnblankDisplays();
  return true;
}

bool CWinSystemOSX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  // find the screen where the application started the last time. It'd be the default screen if the
  // screen index is not found/available.
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_lastDisplayNr = GetDisplayIndex(settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  NSScreen* screen = nil;
  if (m_lastDisplayNr < NSScreen.screens.count)
  {
    screen = [NSScreen.screens objectAtIndex:m_lastDisplayNr];
  }

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

  const NSUInteger windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
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

      NSWindowCollectionBehavior behavior = appWindow.collectionBehavior;
      //! @todo actually implement fullscreen tilling and remove NSWindowCollectionBehaviorFullScreenDisallowsTiling
      behavior |= NSWindowCollectionBehaviorFullScreenPrimary |
                  NSWindowCollectionBehaviorFullScreenDisallowsTiling;
      [appWindow setCollectionBehavior:behavior];

      // associate with current window
      [appWindow setContentView:view];

      // set the window to the appropriate screen and screen position
      if (screen)
      {
        if (m_bFullScreen)
        {
          [appWindow setFrameOrigin:screen.frame.origin];
        }
        else
        {
          // if there are stored window positions use that as the origin point
          const int top = settings->GetInt(SETTING_WINDOW_TOP);
          const int left = settings->GetInt(SETTING_WINDOW_LEFT);

          NSPoint windowPos;
          if (top != 0 || left != 0)
          {
            windowPos = NSMakePoint(left, top);
          }
          else
          {
            // otherwise center the window on the screen
            windowPos =
                NSMakePoint(screen.frame.origin.x + screen.frame.size.width / 2 - m_nWidth / 2,
                            screen.frame.origin.y + screen.frame.size.height / 2 - m_nHeight / 2);
          }
          [appWindow setFrameOrigin:windowPos];
        }
      }
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

  // get screen refreshrate - this is needed
  // when we startup in windowed mode and don't run through SetFullScreen
  size_t dummy;
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
    m_appWindow = nullptr;
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
  NSRect frame = NSZeroRect;
  if (m_appWindow)
  {
    NSWindow* win = (NSWindow*)m_appWindow;
    frame = win.contentView.frame;
  }
  return frame;
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
  std::unique_lock<CCriticalSection> lock(m_critSection);

  __block NSWindow* window = m_appWindow;

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_lastDisplayNr = GetDisplayIndex(settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  const bool fullScreenState = m_bFullScreen;
  m_bFullScreen = fullScreen;

  //handle resolution/refreshrate switching early here
  if (m_bFullScreen)
  {
    // switch videomode
    SwitchToVideoMode(res.iWidth, res.iHeight, static_cast<double>(res.fRefreshRate));
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
    // This is Cocoa Windowed FullScreen Mode
    // Get the screen rect of our current display
    NSScreen* pScreen = [NSScreen.screens objectAtIndex:m_lastDisplayNr];

    // remove frame origin offset of original display
    pScreen.frame.origin = NSZeroPoint;

    // Update safeareainsets (display may have a notch)
    //! @TODO update code block once minimal SDK version is bumped to at least 12.0 (remove NSInvocation and selector)
    auto safeAreaInsetsSelector = @selector(safeAreaInsets);
    if ([pScreen respondsToSelector:safeAreaInsetsSelector])
    {
      NSEdgeInsets insets;
      NSMethodSignature* safeAreaSignature =
          [pScreen methodSignatureForSelector:safeAreaInsetsSelector];
      NSInvocation* safeAreaInvocation =
          [NSInvocation invocationWithMethodSignature:safeAreaSignature];
      [safeAreaInvocation setSelector:safeAreaInsetsSelector];
      [safeAreaInvocation invokeWithTarget:pScreen];
      [safeAreaInvocation getReturnValue:&insets];

      RESOLUTION currentRes = m_gfxContext->GetVideoResolution();
      RESOLUTION_INFO resInfo = m_gfxContext->GetResInfo(currentRes);
      resInfo.guiInsets = EdgeInsets(insets.right, insets.bottom, insets.left, insets.top);
      m_gfxContext->SetResInfo(currentRes, resInfo);
    }

    dispatch_sync(dispatch_get_main_queue(), ^{
      [window.contentView setFrameSize:NSMakeSize(m_nWidth, m_nHeight)];
      window.title = @"";
      [window setAllowsConcurrentViewDrawing:YES];
    });

    // Blank other displays if requested.
    if (blankOtherDisplays)
      BlankOtherDisplays(m_lastDisplayNr);
  }
  else
  {
    // Show menubar.
    dispatch_sync(dispatch_get_main_queue(), ^{
      [NSApplication.sharedApplication setPresentationOptions:NSApplicationPresentationDefault];
    });

    // Unblank.
    // Force the unblank when returning from fullscreen, we get called with blankOtherDisplays set false.
    //if (blankOtherDisplays)
    UnblankDisplays();
  }

  // toggle cocoa fullscreen mode
  if (fullScreenState != m_bFullScreen)
  {
    m_fullscreenWillToggle = true;
    [m_appWindow performSelectorOnMainThread:@selector(toggleFullScreen:)
                                  withObject:nil
                               waitUntilDone:YES];
  }

  ResizeWindow(m_nWidth, m_nHeight, -1, -1);

  return true;
}

#pragma mark - Resolution

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  // Add desktop resolution
  size_t w;
  size_t h;
  double fps;

  const NSUInteger dispIdx =
      GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOSCREEN_MONITOR));
  GetScreenResolution(&w, &h, &fps, dispIdx);
  NSString* const dispName = screenNameForDisplay(dispIdx);
  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP),
                          dispName.UTF8String, static_cast<int>(w), static_cast<int>(h), fps, 0);

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  // now just fill in the possible resolutions for the attached screens
  // and push to the resolution info vector
  FillInVideoModes();
  CDisplaySettings::GetInstance().ApplyCalibrations();
}

void CWinSystemOSX::GetScreenResolution(size_t* w, size_t* h, double* fps, unsigned long screenIdx)
{
  CGDirectDisplayID display_id = (CGDirectDisplayID)GetDisplayID(screenIdx);
  CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display_id);
  *w = CGDisplayModeGetWidth(mode);
  *h = CGDisplayModeGetHeight(mode);
  *fps = CGDisplayModeGetRefreshRate(mode);
  CGDisplayModeRelease(mode);
  if (static_cast<int>(*fps) == 0)
  {
    // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
    *fps = 60.0;
  }
}

#pragma mark - Video Modes

bool CWinSystemOSX::SwitchToVideoMode(int width, int height, double refreshrate)
{
  CGDisplayModeRef dispMode = nullptr;

  const NSUInteger screenIdx =
      GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOSCREEN_MONITOR));

  // Figure out the screen size. (default to main screen)
  const CGDirectDisplayID display_id = GetDisplayID(screenIdx);

  // find mode that matches the desired size, refreshrate
  // non interlaced, nonstretched, safe for hardware
  dispMode = GetMode(width, height, refreshrate, screenIdx);

  //not found - fallback to bestemdeforparameters
  if (!dispMode)
  {
    dispMode = BestMatchForMode(display_id, 32, width, height);

    if (!dispMode)
    {
      dispMode = BestMatchForMode(display_id, 16, width, height);

      // still no match? fallback to current resolution of the display which HAS to work [tm]
      if (!dispMode)
      {
        size_t currentWidth;
        size_t currentHeight;
        double currentRefresh;

        GetScreenResolution(&currentWidth, &currentHeight, &currentRefresh, screenIdx);
        dispMode = GetMode(currentWidth, currentHeight, currentRefresh, screenIdx);

        // no way to get a resolution set
        if (!dispMode)
          return false;
      }
    }
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
  const NSUInteger dispIdx =
      GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOSCREEN_MONITOR));

  for (NSUInteger disp = 0; disp <= NSScreen.screens.count - 1; disp++)
  {
    bool stretched;
    bool interlaced;
    bool safeForHardware;
    size_t w;
    size_t h;
    size_t bitsperpixel;
    double refreshrate;
    RESOLUTION_INFO res;

    CFArrayRef displayModes = GetAllDisplayModes(GetDisplayID(disp));
    NSString* const dispName = screenNameForDisplay(disp);

    CLog::LogF(LOGINFO, "Display {} has name {}", disp, [dispName UTF8String]);

    if (!displayModes)
      continue;

    for (int i = 0; i < CFArrayGetCount(displayModes); ++i)
    {
      CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, i);

      uint32_t flags = CGDisplayModeGetIOFlags(displayMode);
      stretched = (flags & kDisplayModeStretchedFlag) != 0;
      interlaced = (flags & kDisplayModeInterlacedFlag) != 0;
      bitsperpixel = DisplayBitsPerPixelForMode(displayMode);
      safeForHardware = (flags & kDisplayModeSafetyFlags) != 0;

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
        CLog::Log(LOGINFO, "Found possible resolution for display {} with {} x {} @ {} Hz", disp, w,
                  h, refreshrate);

        // only add the resolution if it belongs to "our" screen
        // all others are only logged above...
        if (disp == dispIdx)
        {
          UpdateDesktopResolution(res, (dispName != nil) ? [dispName UTF8String] : "Unknown",
                                  static_cast<int>(w), static_cast<int>(h), refreshrate, 0);
          CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
          CDisplaySettings::GetInstance().AddResolutionInfo(res);
        }
      }
    }
    CFRelease(displayModes);
  }
}

#pragma mark - Window Move

void CWinSystemOSX::OnMove(int x, int y)
{
  // check if the current screen/monitor settings needs to be updated
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string storedScreenName = settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  const std::string currentScreenName = screenNameForDisplay(m_lastDisplayNr).UTF8String;
  if (storedScreenName != currentScreenName)
  {
    CDisplaySettings::GetInstance().SetMonitor(currentScreenName);
  }

  // check if refresh rate needs to be updated
  static double oldRefreshRate = m_refreshRate;
  Cocoa_CVDisplayLinkUpdate();

  size_t dummy = 0;
  GetScreenResolution(&dummy, &dummy, &m_refreshRate, m_lastDisplayNr);

  if (oldRefreshRate != m_refreshRate)
  {
    oldRefreshRate = m_refreshRate;

    // send a message so that videoresolution (and refreshrate) is changed
    dispatch_sync(dispatch_get_main_queue(), ^{
      NSWindow* win = m_appWindow;
      NSRect frame = win.contentView.frame;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_VIDEORESIZE, frame.size.width,
                                                 frame.size.height);
    });
  }
  // store window position in window mode
  if (!m_bFullScreen)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      settings->SetInt(SETTING_WINDOW_LEFT, m_appWindow.frame.origin.x);
      settings->SetInt(SETTING_WINDOW_TOP, m_appWindow.frame.origin.y);
      settings->Save();
    });
  }
}

void CWinSystemOSX::WindowChangedScreen()
{
  // if we are here the user dragged the window to a different
  // screen and we return the screen of the window
  if (m_appWindow)
  {
    m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen(m_appWindow.screen));
  }
}

CGLContextObj CWinSystemOSX::GetCGLContextObj()
{
  __block CGLContextObj cglcontex = nullptr;
  if (m_appWindow)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      OSXGLView* contentView = m_appWindow.contentView;
      cglcontex = contentView.getGLContext.CGLContextObj;
    });
  }

  return cglcontex;
}

bool CWinSystemOSX::FlushBuffer()
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
  [NSOpenGLContext.currentContext setValues:&swapInterval
                               forParameter:NSOpenGLContextParameterSwapInterval];
}

std::unique_ptr<CVideoSync> CWinSystemOSX::GetVideoSync(void* clock)
{
  return std::make_unique<CVideoSyncOsx>(clock);
}

std::vector<std::string> CWinSystemOSX::GetConnectedOutputs()
{
  std::vector<std::string> outputs;
  outputs.push_back(DEFAULT_SCREEN_NAME);

  // screen 0 is always the "Default" setting, avoid duplicating the available
  // screens here.
  const NSUInteger numDisplays = NSScreen.screens.count;
  if (numDisplays > 1)
  {
    for (NSUInteger disp = 1; disp <= numDisplays - 1; disp++)
    {
      NSString* const dispName = screenNameForDisplay(disp);
      outputs.push_back(dispName.UTF8String);
    }
  }

  return outputs;
}

#pragma mark - OSScreenSaver

std::unique_ptr<IOSScreenSaver> CWinSystemOSX::GetOSScreenSaverImpl()
{
  return std::make_unique<COSScreenSaverOSX>();
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

std::string CWinSystemOSX::GetClipboardText()
{
  std::string utf8_text;

  const char* szStr = Cocoa_Paste();
  if (szStr)
    utf8_text = szStr;

  return utf8_text;
}

void CWinSystemOSX::signalMouseEntered()
{
  m_winEvents->signalMouseEntered();
}

void CWinSystemOSX::signalMouseExited()
{
  m_winEvents->signalMouseExited();
}
