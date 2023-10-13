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
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
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
#import "windowing/osx/OpenGL/WindowControllerMacOS.h"
#include "windowing/osx/VideoSyncOsx.h"
#include "windowing/osx/WinEventsOSX.h"

#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/osx/powermanagement/CocoaPowerSyscall.h"

#include <array>
#include <chrono>
#include <memory>
#include <mutex>

#import <IOKit/graphics/IOGraphicsLib.h>

using namespace KODI;
using namespace MESSAGING;
using namespace WINDOWING;
using namespace std::chrono_literals;

namespace
{
constexpr int MAX_DISPLAYS = 32;
constexpr const char* DEFAULT_SCREEN_NAME = "Default";
} // namespace

static std::array<NSWindowController*, MAX_DISPLAYS> blankingWindowControllers;

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

EdgeInsets GetScreenEdgeInsets(NSUInteger screenIdx)
{
  EdgeInsets safeAreaInsets;
  NSScreen* pScreen = NSScreen.screens[screenIdx];

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
    // screen backing factor might be higher than 1 (point size vs pixel size in retina displays)
    safeAreaInsets = EdgeInsets(
        insets.right * pScreen.backingScaleFactor, insets.bottom * pScreen.backingScaleFactor,
        insets.left * pScreen.backingScaleFactor, insets.top * pScreen.backingScaleFactor);
  }
  return safeAreaInsets;
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

void CheckAndUpdateCurrentMonitor(NSUInteger screenNumber)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string storedScreenName = settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  const std::string currentScreenName = screenNameForDisplay(screenNumber).UTF8String;
  if (storedScreenName != currentScreenName)
  {
    CDisplaySettings::GetInstance().SetMonitor(currentScreenName);
  }
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

std::string ComputeVideoModeId(
    size_t resWidth, size_t resHeight, size_t pixelWidth, size_t pixelHeight, bool interlaced)
{
  const char* interlacedDesc = interlaced ? "i" : "p";
  return StringUtils::Format("{}x{}{}({}x{})", resWidth, resHeight, interlacedDesc, pixelWidth,
                             pixelHeight);
}

CFArrayRef CopyAllDisplayModes(CGDirectDisplayID display)
{
  int value = 1;

  CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &value);
  if (!number)
  {
    CLog::LogF(LOGERROR, "Could not create Number!");
    return nullptr;
  }

  CFStringRef key = kCGDisplayShowDuplicateLowResolutionModes;
  CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&key,
                                               (const void**)&number, 1, nullptr, nullptr);
  CFRelease(number);

  if (!options)
  {
    CLog::LogF(LOGERROR, "Could not create Dictionary!");
    return nullptr;
  }

  CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(display, options);
  CFRelease(options);

  if (!displayModes)
  {
    CLog::LogF(LOGERROR, "No displaymodes found!");
    return nullptr;
  }

  return displayModes;
}

CGDisplayModeRef CreateModeById(const std::string& modeId, NSUInteger screenIdx)
{
  if (modeId.empty())
    return nullptr;

  bool stretched;
  bool interlaced;
  bool safeForHardware;
  size_t resWidth;
  size_t resHeight;
  size_t pixelWidth;
  size_t pixelHeight;
  size_t bitsperpixel;
  RESOLUTION_INFO res;

  CLog::LogF(LOGDEBUG, "Looking for mode for screen {} with id {}", screenIdx, modeId);

  CFArrayRef allModes = CopyAllDisplayModes(GetDisplayID(screenIdx));

  if (!allModes)
    return nullptr;

  for (int i = 0; i < CFArrayGetCount(allModes); ++i)
  {
    CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
    uint32_t flags = CGDisplayModeGetIOFlags(displayMode);
    stretched = (flags & kDisplayModeStretchedFlag) != 0;
    bitsperpixel = DisplayBitsPerPixelForMode(displayMode);
    safeForHardware = (flags & kDisplayModeSafetyFlags) != 0;
    interlaced = (flags & kDisplayModeInterlacedFlag) != 0;
    resWidth = CGDisplayModeGetWidth(displayMode);
    resHeight = CGDisplayModeGetHeight(displayMode);
    pixelWidth = CGDisplayModeGetPixelWidth(displayMode);
    pixelHeight = CGDisplayModeGetPixelHeight(displayMode);

    if (bitsperpixel == 32 && safeForHardware && !stretched &&
        modeId == ComputeVideoModeId(resWidth, resHeight, pixelWidth, pixelHeight, interlaced))
    {
      CGDisplayModeRetain(displayMode);
      CFRelease(allModes);
      CLog::LogF(LOGDEBUG, "Found a match!");
      return displayMode;
    }
  }

  CFRelease(allModes);
  CLog::LogF(LOGERROR, "No match found!");
  return nullptr;
}

// try to find mode that matches the desired size, refreshrate
// non interlaced, nonstretched, safe for hardware
CGDisplayModeRef CreateMode(size_t width, size_t height, double refreshrate, NSUInteger screenIdx)
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

  CLog::LogF(LOGDEBUG, "Looking for suitable mode with {} x {} @ {} Hz on display {}", width,
             height, refreshrate, screenIdx);

  CFArrayRef allModes = CopyAllDisplayModes(GetDisplayID(screenIdx));

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
    w = CGDisplayModeGetPixelWidth(displayMode);
    h = CGDisplayModeGetPixelHeight(displayMode);

    rate = CGDisplayModeGetRefreshRate(displayMode);

    if (bitsperpixel == 32 && safeForHardware && !stretched && !interlaced == false && w == width &&
        h == height && (rate == refreshrate || rate == 0))
    {
      CGDisplayModeRetain(displayMode);
      CFRelease(allModes);
      CLog::LogF(LOGDEBUG, "Found a match!");
      return displayMode;
    }
  }

  CFRelease(allModes);
  CLog::LogF(LOGERROR, "No match found!");
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
  CFArrayRef allModes = CopyAllDisplayModes(display);

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
      CGDisplayModeRetain(displayMode);
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
        CGDisplayModeRetain(displayMode);
        break;
      }
    }
  }

  CFRelease(allModes);

  return displayMode;
}

#pragma mark - Blank Displays

void BlankOtherDisplays(NSUInteger screenBeingUsed)
{
  const NSUInteger numDisplays = NSScreen.screens.count;

  // Blank all other displays except the current one.
  for (NSUInteger i = 0; i < numDisplays; i++)
  {
    if (i != screenBeingUsed)
    {
      // Get the size of the screen
      NSScreen* pScreen = [NSScreen.screens objectAtIndex:i];
      NSRect screenRect = pScreen.frame;
      screenRect.origin = NSZeroPoint;

      dispatch_sync(dispatch_get_main_queue(), ^{
        // Build a blanking (black) window.
        auto blankingWindow = [[NSWindow alloc] initWithContentRect:screenRect
                                                          styleMask:NSWindowStyleMaskBorderless
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO
                                                             screen:pScreen];
        [blankingWindow setBackgroundColor:NSColor.blackColor];
        [blankingWindow setLevel:CGShieldingWindowLevel()];
        [blankingWindow makeKeyAndOrderFront:nil];

        // Create a controller and bind the blanking window to it
        blankingWindowControllers[i] = [[NSWindowController alloc] init];
        [blankingWindowControllers[i] setWindow:blankingWindow];
      });
    }
  }
}

void UnblankDisplay(NSUInteger screenToUnblank)
{
  if (screenToUnblank < blankingWindowControllers.size() &&
      blankingWindowControllers[screenToUnblank])
  {
    [[blankingWindowControllers[screenToUnblank] window] close];
    blankingWindowControllers[screenToUnblank] = nil;
  }
}

void UnblankDisplays(NSUInteger screenBeingUsed)
{
  for (NSUInteger i = 0; i < NSScreen.screens.count; i++)
  {
    if (blankingWindowControllers[i] && i != screenBeingUsed)
    {
      // Get rid of the blanking windows we created.
      // Note after closing the window, setting the NSWindowController to nil will dealoc
      dispatch_sync(dispatch_get_main_queue(), ^{
        UnblankDisplay(i);
      });
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
    if (!winsys->HasValidResolution())
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

  m_winEvents = std::make_unique<CWinEventsOSX>();

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
  // ensure that graphics context knows about the current refreshrate before doing the callbacks
  auto screenResolution = GetScreenResolution(m_lastDisplayNr);
  m_gfxContext->SetFPS(screenResolution.refreshrate);

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

  UnblankDisplays(m_lastDisplayNr);
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

  __block NSRect bounds;
  dispatch_sync(dispatch_get_main_queue(), ^{
    auto title = [NSString stringWithUTF8String:m_name.c_str()];
    auto size = NSMakeSize(m_nWidth, m_nHeight);
    // propose the window size based on the last stored RES_WINDOW resolution info
    m_appWindowController = [[XBMCWindowControllerMacOS alloc] initWithTitle:title
                                                                 defaultSize:size];

    m_appWindow = m_appWindowController.window;
    m_glView = m_appWindow.contentView;
    bounds = m_appWindow.contentView.bounds;
  });

  [m_glView Update];

  NSScreen* currentScreen = [NSScreen mainScreen];
  dispatch_sync(dispatch_get_main_queue(), ^{
    // NSWindowController does not track the last used screen so set the frame coordinates
    // to the center of the screen in that case
    if (screen && currentScreen != screen)
    {
      [m_appWindow setFrameOrigin:NSMakePoint(NSMidX(screen.frame) - m_nWidth / 2,
                                              NSMidY(screen.frame) - m_nHeight / 2)];
    }
    [m_appWindowController showWindow:m_appWindow];
  });

  m_bWindowCreated = true;

  CheckAndUpdateCurrentMonitor(m_lastDisplayNr);

  // warning, we can order front but not become
  // key window or risk starting up with bad flicker
  // becoming key window must happen in completion block.
  [(NSWindow*)m_appWindow performSelectorOnMainThread:@selector(orderFront:)
                                           withObject:nil
                                        waitUntilDone:YES];

  [NSAnimationContext endGrouping];

  // get screen refreshrate - this is needed
  // when we startup in windowed mode and don't run through SetFullScreen
  auto screenResolution = GetScreenResolution(m_lastDisplayNr);
  m_refreshRate = screenResolution.refreshrate;

  // NSWindowController decides what is the best size for the window, so make sure to
  // update the stored resolution right after the window creation (it's used for example by the splash screen)
  // with the actual size of the window.
  // Make sure the window frame rect is converted to backing units ONLY after moving it to the display screen
  // (as it might be moving to another non-HiDPI screen).
  dispatch_sync(dispatch_get_main_queue(), ^{
    bounds = [m_appWindow convertRectToBacking:bounds];
  });
  SetWindowResolution(bounds.size.width, bounds.size.height);

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
    dispatch_sync(dispatch_get_main_queue(), ^{
      [m_appWindow setContentView:nil];
      [[m_appWindowController window] close];
    });

    m_appWindow = nil;
    m_appWindowController = nil;
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

  __block OSXGLView* view;
  dispatch_sync(dispatch_get_main_queue(), ^{
    view = m_appWindow.contentView;
  });

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
      [view Update];
    });
  }

  m_nWidth = newWidth;
  m_nHeight = newHeight;

  return true;
}

bool CWinSystemOSX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_lastDisplayNr = GetDisplayIndex(settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  const bool fullScreenState = m_bFullScreen;
  m_bFullScreen = fullScreen;

  // handle resolution/refreshrate switching early here
  if (m_bFullScreen)
  {
    // switch videomode
    SwitchToVideoMode(res);
  }

  if (m_fullscreenWillToggle)
  {
    ResizeWindow(m_nWidth, m_nHeight, -1, -1);
    m_fullscreenWillToggle = false;

    // Blank other displays if requested.
    if (blankOtherDisplays)
    {
      BlankOtherDisplays(m_lastDisplayNr);
    }
    else
    {
      UnblankDisplays(m_lastDisplayNr);
    }

    return true;
  }

  if (m_bFullScreen)
  {
    // Blank/Unblank other displays if requested.
    if (blankOtherDisplays)
    {
      BlankOtherDisplays(m_lastDisplayNr);
    }
    else
    {
      UnblankDisplays(m_lastDisplayNr);
    }
  }
  else
  {
    // Show menubar.
    dispatch_sync(dispatch_get_main_queue(), ^{
      [NSApplication.sharedApplication setPresentationOptions:NSApplicationPresentationDefault];
    });

    // Unblank.
    // Force the unblank when returning from fullscreen, we get called with blankOtherDisplays set false.
    UnblankDisplays(m_lastDisplayNr);
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

void CWinSystemOSX::SignalFullScreenStateChanged(bool fullscreenState)
{
  if (!m_fullScreenMovingToScreen.has_value())
  {
    return;
  }

  if (!fullscreenState)
  {
    // check if we are already on the target screen (e.g. due to a display lost)
    if (m_lastDisplayNr != m_fullScreenMovingToScreen.value())
    {
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_MOVETOSCREEN, static_cast<int>(m_fullScreenMovingToScreen.value()));
    }
    else
    {
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
    }
  }
  else if (fullscreenState)
  {
    // fullscreen move of the window has been finished
    m_fullScreenMovingToScreen.reset();
  }
}

#pragma mark - Video Modes

bool CWinSystemOSX::SwitchToVideoMode(RESOLUTION_INFO& res)
{
  CGDisplayModeRef dispMode = nullptr;

  const NSUInteger screenIdx =
      GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOSCREEN_MONITOR));

  // Figure out the screen size. (default to main screen)
  const CGDirectDisplayID display_id = GetDisplayID(screenIdx);

  // find mode that matches the desired size, refreshrate non interlaced, nonstretched, safe for hardware

  // try to find an exact match first (by using the unique ids we assign to resolution infos)
  dispMode = CreateModeById(res.strId, screenIdx);
  if (!dispMode)
  {
    dispMode =
        CreateMode(res.iWidth, res.iHeight, static_cast<double>(res.fRefreshRate), screenIdx);
  }

  // not found - fallback to bestmodeforparameters
  if (!dispMode)
  {
    dispMode = BestMatchForMode(display_id, 32, res.iWidth, res.iHeight);

    if (!dispMode)
    {
      dispMode = BestMatchForMode(display_id, 16, res.iWidth, res.iHeight);

      // still no match? fallback to current resolution of the display which HAS to work [tm]
      if (!dispMode)
      {
        auto screenResolution = GetScreenResolution(screenIdx);
        dispMode = CreateMode(screenResolution.pixelWidth, screenResolution.pixelHeight,
                              screenResolution.refreshrate, screenIdx);

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
  CFRelease(dispMode);
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
    size_t resWidth;
    size_t resHeight;
    size_t pixelWidth;
    size_t pixelHeight;
    size_t bitsperpixel;
    double refreshrate;
    RESOLUTION_INFO res;

    CFArrayRef displayModes = CopyAllDisplayModes(GetDisplayID(disp));
    NSString* const dispName = screenNameForDisplay(disp);
    res.guiInsets = GetScreenEdgeInsets(disp);

    CLog::LogF(LOGINFO, "Display {} has name {}", disp, dispName.UTF8String);

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

      if (bitsperpixel == 32 && safeForHardware && !stretched && !interlaced)
      {
        resWidth = CGDisplayModeGetWidth(displayMode);
        resHeight = CGDisplayModeGetHeight(displayMode);
        pixelWidth = CGDisplayModeGetPixelWidth(displayMode);
        pixelHeight = CGDisplayModeGetPixelHeight(displayMode);
        refreshrate = CGDisplayModeGetRefreshRate(displayMode);
        if (static_cast<int>(refreshrate) == 0) // LCD display?
        {
          // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
          refreshrate = 60.0;
        }
        const std::string modeId =
            ComputeVideoModeId(resWidth, resHeight, pixelWidth, pixelHeight, interlaced);
        CLog::LogF(
            LOGINFO,
            "Found possible resolution for display {} ({}) with {} x {} @ {} Hz (pixel size: "
            "{} x {}{}) (id:{})",
            disp, dispName.UTF8String, resWidth, resHeight, refreshrate, pixelWidth, pixelHeight,
            pixelWidth > resWidth && pixelHeight > resHeight ? " - HiDPI" : "", modeId);

        // only add the resolution if it belongs to "our" screen
        // all others are only logged above...
        if (disp == dispIdx)
        {
          res.strId = modeId;
          UpdateDesktopResolution(res, (dispName != nil) ? dispName.UTF8String : "Unknown",
                                  static_cast<int>(pixelWidth), static_cast<int>(pixelHeight),
                                  static_cast<int>(resWidth), static_cast<int>(resHeight),
                                  refreshrate, 0);
          m_gfxContext->ResetOverscan(res);
          CDisplaySettings::GetInstance().AddResolutionInfo(res);
        }
      }
    }
    CFRelease(displayModes);
  }
}

#pragma mark - Resolution

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();
  const NSUInteger dispIdx =
      GetDisplayIndex(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOSCREEN_MONITOR));

  auto screenResolution = GetScreenResolution(dispIdx);
  NSString* const dispName = screenNameForDisplay(dispIdx);
  RESOLUTION_INFO& resInfo = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
  resInfo.guiInsets = GetScreenEdgeInsets(dispIdx);
  resInfo.strId = ComputeVideoModeId(screenResolution.resWidth, screenResolution.resHeight,
                                     screenResolution.pixelWidth, screenResolution.pixelHeight,
                                     screenResolution.interlaced);
  UpdateDesktopResolution(
      resInfo, dispName.UTF8String, static_cast<int>(screenResolution.pixelWidth),
      static_cast<int>(screenResolution.pixelHeight), static_cast<int>(screenResolution.resWidth),
      static_cast<int>(screenResolution.resHeight), screenResolution.refreshrate, 0);

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  // now just fill in the possible resolutions for the attached screens
  // and push to the resolution info vector
  FillInVideoModes();
  CDisplaySettings::GetInstance().ApplyCalibrations();
}

CWinSystemOSX::ScreenResolution CWinSystemOSX::GetScreenResolution(unsigned long screenIdx)
{
  ScreenResolution screenResolution;
  CGDirectDisplayID display_id = (CGDirectDisplayID)GetDisplayID(screenIdx);
  CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display_id);
  uint32_t flags = CGDisplayModeGetIOFlags(mode);
  screenResolution.interlaced = (flags & kDisplayModeInterlacedFlag) != 0;
  screenResolution.resWidth = CGDisplayModeGetWidth(mode);
  screenResolution.resHeight = CGDisplayModeGetHeight(mode);
  screenResolution.pixelWidth = CGDisplayModeGetPixelWidth(mode);
  screenResolution.pixelHeight = CGDisplayModeGetPixelHeight(mode);
  screenResolution.refreshrate = CGDisplayModeGetRefreshRate(mode);
  CGDisplayModeRelease(mode);
  if (static_cast<int>(screenResolution.refreshrate) == 0)
  {
    // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
    screenResolution.refreshrate = 60.0;
  }
  return screenResolution;
}

bool CWinSystemOSX::HasValidResolution() const
{
  return m_gfxContext->GetVideoResolution() != RES_INVALID;
}

#pragma mark - Window Move

void CWinSystemOSX::OnMove(int x, int y)
{
  // check if the current screen/monitor settings needs to be updated
  CheckAndUpdateCurrentMonitor(m_lastDisplayNr);

  // check if refresh rate needs to be updated
  static double oldRefreshRate = m_refreshRate;
  Cocoa_CVDisplayLinkUpdate();

  auto screenResolution = GetScreenResolution(m_lastDisplayNr);
  m_refreshRate = screenResolution.refreshrate;

  if (oldRefreshRate != m_refreshRate)
  {
    oldRefreshRate = m_refreshRate;

    // send a message so that videoresolution (and refreshrate) is changed
    dispatch_sync(dispatch_get_main_queue(), ^{
      NSRect rect = [m_appWindow.contentView
          convertRectToBacking:[m_appWindow contentRectForFrameRect:m_appWindow.frame]];
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_VIDEORESIZE, rect.size.width,
                                                 rect.size.height);
    });
  }
  if (m_fullScreenMovingToScreen.has_value())
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
  }
}

void CWinSystemOSX::OnChangeScreen(unsigned int screenIdx)
{
  const NSUInteger lastDisplay = m_lastDisplayNr;
  if (m_appWindow)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      m_lastDisplayNr = GetDisplayIndex(GetDisplayIDFromScreen(m_appWindow.screen));
    });
  }
  // force unblank the current display
  if (lastDisplay != m_lastDisplayNr && m_bFullScreen)
  {
    UnblankDisplay(m_lastDisplayNr);
    CheckAndUpdateCurrentMonitor(m_lastDisplayNr);
  }
}

unsigned int CWinSystemOSX::GetScreenId(const std::string& screen)
{
  return static_cast<int>(GetDisplayIndex(screen));
}

void CWinSystemOSX::MoveToScreen(unsigned int screenIdx)
{
  // find the future displayId and the screen object
  if (m_bFullScreen)
  {
    // macOS doesn't allow moving fullscreen windows directly to another screen
    // toggle fullscreen first
    if (screenIdx < NSScreen.screens.count)
    {
      m_fullScreenMovingToScreen.emplace(screenIdx);
      m_fullscreenWillToggle = true;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
    }
    return;
  }

  NSScreen* currentScreen;
  NSScreen* targetScreen;
  if (screenIdx < NSScreen.screens.count && m_lastDisplayNr < NSScreen.screens.count)
  {
    currentScreen = NSScreen.screens[m_lastDisplayNr];
    targetScreen = NSScreen.screens[screenIdx];
  }
  // move the window to the center of the new screen
  if (screenIdx != m_lastDisplayNr && targetScreen && currentScreen)
  {
    // moving from a HiDPI screen to a non-HiDPI screen requires that we scale the dimensions of
    // the window properly. m_nWidth and m_nHeight store pixels (not points) after resize callbacks
    const double backingFactor =
        currentScreen.backingScaleFactor > targetScreen.backingScaleFactor
            ? currentScreen.backingScaleFactor / targetScreen.backingScaleFactor
            : currentScreen.backingScaleFactor;
    NSPoint windowPos = NSMakePoint(NSMidX(targetScreen.frame) - (m_nWidth / backingFactor) / 2.0,
                                    NSMidY(targetScreen.frame) - (m_nHeight / backingFactor) / 2.0);
    dispatch_sync(dispatch_get_main_queue(), ^{
      [m_appWindow setFrameOrigin:windowPos];
    });
    m_lastDisplayNr = screenIdx;
  }
}

CGLContextObj CWinSystemOSX::GetCGLContextObj()
{
  __block CGLContextObj cglcontex = nullptr;
  if (m_appWindow)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      cglcontex = [(OSXGLView*)m_appWindow.contentView getGLContextObj];
    });
  }

  return cglcontex;
}

CGraphicContext& CWinSystemOSX::GetGfxContext() const
{
  if (m_glView && [m_glView glContextOwned])
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [m_glView NotifyContext];
    });
  }

  return CWinSystemBase::GetGfxContext();
}

bool CWinSystemOSX::FlushBuffer()
{
  if (m_appWindow)
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [m_appWindow.contentView FlushBuffer];
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

std::unique_ptr<CVideoSync> CWinSystemOSX::GetVideoSync(CVideoReferenceClock* clock)
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
  signalMouseEntered();
}

void CWinSystemOSX::disableInputEvents()
{
  m_winEvents->disableInputEvents();
  signalMouseExited();
}

std::string CWinSystemOSX::GetClipboardText()
{
  std::string utf8_text;

  const char* szStr = Cocoa_Paste();
  if (szStr)
    utf8_text = szStr;

  return utf8_text;
}

bool CWinSystemOSX::HasCursor()
{
  return m_hasCursor;
}

void CWinSystemOSX::signalMouseEntered()
{
  if (m_appWindow.keyWindow)
  {
    m_hasCursor = true;
    m_winEvents->signalMouseEntered();
  }
}

void CWinSystemOSX::signalMouseExited()
{
  if (m_appWindow.keyWindow)
  {
    m_hasCursor = false;
    m_winEvents->signalMouseExited();
  }
}

void CWinSystemOSX::SendInputEvent(NSEvent* nsEvent)
{
  m_winEvents->SendInputEvent(nsEvent);
}
