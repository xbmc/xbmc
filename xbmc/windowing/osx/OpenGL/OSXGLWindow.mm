/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "OSXGLWindow.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "application/AppParamParser.h"
#include "application/Application.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#import "windowing/osx/OpenGL/OSXGLView.h"
#include "windowing/osx/WinEventsOSX.h"
#import "windowing/osx/WinSystemOSX.h"

#include "platform/darwin/osx/CocoaInterface.h"

@implementation XBMCWindowControllerMacOS

- (nullable instancetype)initWithTitle:(NSString*)title defaultSize:(NSSize)size
{
  auto frame = NSMakeRect(0, 0, size.width, size.height);
  const NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

  auto window = [[NSWindow alloc] initWithContentRect:frame
                                            styleMask:style
                                              backing:NSBackingStoreBuffered
                                                defer:YES];
  window.backgroundColor = NSColor.blackColor;
  window.title = title;
  window.titlebarAppearsTransparent = YES;
  window.titleVisibility = NSWindowTitleHidden;

  window.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary |
                              NSWindowCollectionBehaviorFullScreenDisallowsTiling;
  window.tabbingMode = NSWindowTabbingModeDisallowed;

  if ((self = [super initWithWindow:window]))
  {
    self.windowFrameAutosaveName = @"OSXGLWindowPositionHeightWidth";
    self.shouldCascadeWindows = NO;
    window.delegate = self;
    window.contentView = [[OSXGLView alloc] initWithFrame:NSZeroRect];
    window.acceptsMouseMovedEvents = YES; // SHH: ?view can't unless window does
  }
  g_application.m_AppFocused = true;
  return self;
}

- (void)windowDidResize:(NSNotification*)aNotification
{
  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return;

  NSRect rect = [self.window contentRectForFrameRect:self.window.frame];
  int width = static_cast<int>(rect.size.width);
  int height = static_cast<int>(rect.size.height);

  XBMC_Event newEvent = {};

  if (!winSystem->IsFullScreen() && !winSystem->GetFullscreenWillToggle())
  {
    RESOLUTION res_index = RES_DESKTOP;
    if ((width == CDisplaySettings::GetInstance().GetResolutionInfo(res_index).iWidth) &&
        (height == CDisplaySettings::GetInstance().GetResolutionInfo(res_index).iHeight))
      return;

    newEvent.type = XBMC_VIDEORESIZE;
  }
  else
  {
    // macos may trigger a resize/rescale event after (or in) the fullscreen state. Use a different event
    // so that window coordinates are properly set (XBMC_VIDEORESIZE is only supposed to be used when running
    // windowed)
    newEvent.type = XBMC_FULLSCREEN_UPDATE;
  }

  newEvent.resize.w = width;
  newEvent.resize.h = height;

  // check for valid sizes cause in some cases
  // we are hit during fullscreen transition from macos
  // and might be technically "zero" sized
  if (newEvent.resize.w != 0 && newEvent.resize.h != 0)
  {
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->OnEvent(newEvent);
  }
}

- (void)windowDidMiniaturize:(NSNotification*)aNotification
{
  g_application.m_AppFocused = false;
}

- (void)windowDidDeminiaturize:(NSNotification*)aNotification
{
  g_application.m_AppFocused = true;
}

- (void)windowDidBecomeKey:(NSNotification*)aNotification
{
  g_application.m_AppFocused = true;

  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
  {
    winSystem->enableInputEvents();
  }
}

- (void)windowDidResignKey:(NSNotification*)aNotification
{
  g_application.m_AppFocused = false;

  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
  {
    winSystem->disableInputEvents();
  }
}

- (BOOL)windowShouldClose:(id)sender
{
  if (!g_application.m_bStop)
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);

  return NO;
}

- (void)windowDidExpose:(NSNotification*)aNotification
{
  g_application.m_AppFocused = true;
}

- (void)windowDidMove:(NSNotification*)aNotification
{
  if (self.window.contentView)
  {
    NSPoint window_origin = [self.window.contentView frame].origin;
    XBMC_Event newEvent = {};
    newEvent.type = XBMC_VIDEOMOVE;
    newEvent.move.x = window_origin.x;
    newEvent.move.y = window_origin.y;
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
    {
      appPort->OnEvent(newEvent);
    }
  }
}

- (void)windowDidChangeScreen:(NSNotification*)notification
{
  // user has moved the window to a different screen
  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return;
  winSystem->WindowChangedScreen();
}

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
{
  return frameSize;
}

- (void)windowWillEnterFullScreen:(NSNotification*)pNotification
{
  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return;

  // if osx is the issuer of the toggle
  // call Kodi's toggle function
  if (!winSystem->GetFullscreenWillToggle())
  {
    // indicate that we are toggling
    // flag will be reset in SetFullscreen once its
    // called from Kodi's gui thread
    winSystem->SetFullscreenWillToggle(true);

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
  }
  else
  {
    // in this case we are just called because
    // of Kodi did a toggle - just reset the flag
    // we don't need to do anything else
    winSystem->SetFullscreenWillToggle(false);
  }
}

- (NSApplicationPresentationOptions)window:(NSWindow*)window
      willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
  // customize our appearance when entering full screen:
  // we don't want the dock to appear but we want the menubar to hide/show automatically
  //
  return (NSApplicationPresentationFullScreen | // support full screen for this window (required)
          NSApplicationPresentationHideDock | // completely hide the dock
          NSApplicationPresentationAutoHideMenuBar); // yes we want the menu bar to show/hide
}

- (void)windowDidExitFullScreen:(NSNotification*)pNotification
{
  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return;

  // if osx is the issuer of the toggle
  // call Kodi's toggle function
  if (!winSystem->GetFullscreenWillToggle())
  {
    // indicate that we are toggling
    // flag will be reset in SetFullscreen once its
    // called from Kodi's gui thread
    winSystem->SetFullscreenWillToggle(true);
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
  }
  else
  {
    // in this case we are just called because
    // of Kodi did a toggle - just reset the flag
    // we don't need to do anything else
    winSystem->SetFullscreenWillToggle(false);
  }
}
@end
