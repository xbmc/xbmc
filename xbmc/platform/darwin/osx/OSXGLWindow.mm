/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "OSXGLWindow.h"

#include "AppInboundProtocol.h"
#include "AppParamParser.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "windowing/osx/WinEventsOSX.h"
#import "windowing/osx/WinSystemOSX.h"

#include "platform/darwin/osx/CocoaInterface.h"
#import "platform/darwin/osx/OSXGLView.h"

//------------------------------------------------------------------------------------------
@implementation OSXGLWindow

@synthesize resizeState = m_resizeState;

- (id)initWithContentRect:(NSRect)box styleMask:(uint)style
{
  self = [super initWithContentRect:box styleMask:style backing:NSBackingStoreBuffered defer:YES];
  [self setDelegate:self];
  [self setAcceptsMouseMovedEvents:YES];
  // autosave the window position/size
  // Tell the controller to not cascade its windows.
  [self.windowController setShouldCascadeWindows:NO];
  [self setFrameAutosaveName:@"OSXGLWindowPositionHeightWidth"];

  g_application.m_AppFocused = true;

  return self;
}

- (void)dealloc
{
  [self setDelegate:nil];
}

- (BOOL)windowShouldClose:(id)sender
{
  if (!g_application.m_bStop)
    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);

  return NO;
}

- (void)windowDidExpose:(NSNotification*)aNotification
{
  g_application.m_AppFocused = true;
}

- (void)windowDidMove:(NSNotification*)aNotification
{
  NSOpenGLContext* context = NSOpenGLContext.currentContext;
  if (context)
  {
    if (context.view)
    {
      NSPoint window_origin = [[[context view] window] frame].origin;
      XBMC_Event newEvent = {};
      newEvent.type = XBMC_VIDEOMOVE;
      newEvent.move.x = window_origin.x;
      newEvent.move.y = window_origin.y;
      std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
      if (appPort)
        appPort->OnEvent(newEvent);
    }
  }
}

- (void)windowWillStartLiveResize:(NSNotification*)notification
{
  m_resizeState = true;
}

- (void)windowDidEndLiveResize:(NSNotification*)notification
{
  m_resizeState = false;
}

- (void)windowDidResize:(NSNotification*)aNotification
{
  if (!m_resizeState)
  {
    NSRect rect = [self contentRectForFrameRect:self.frame];

    if (!CServiceBroker::GetWinSystem()->IsFullScreen())
    {
      RESOLUTION res_index = RES_DESKTOP;
      if ((static_cast<int>(rect.size.width) ==
           CDisplaySettings::GetInstance().GetResolutionInfo(res_index).iWidth) &&
          (static_cast<int>(rect.size.height) ==
           CDisplaySettings::GetInstance().GetResolutionInfo(res_index).iHeight))
        return;
    }
    XBMC_Event newEvent = {};
    newEvent.type = XBMC_VIDEORESIZE;
    newEvent.resize.w = static_cast<int>(rect.size.width);
    newEvent.resize.h = static_cast<int>(rect.size.height);

    // check for valid sizes cause in some cases
    // we are hit during fullscreen transition from osx
    // and might be technically "zero" sized
    if (newEvent.resize.w != 0 && newEvent.resize.h != 0)
    {
      std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
      if (appPort)
        appPort->OnEvent(newEvent);
    }
    //CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
  }
}

- (void)windowDidChangeScreen:(NSNotification*)notification
{
  // user has moved the window to a
  // different screen
  //  if (CServiceBroker::GetWinSystem()->IsFullScreen())
  //    CServiceBroker::GetWinSystem()->SetMovedToOtherScreen(true);
}

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
{
  return frameSize;
}

- (void)windowWillEnterFullScreen:(NSNotification*)pNotification
{
  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  // if osx is the issuer of the toggle
  // call XBMCs toggle function
  if (!winSystem->GetFullscreenWillToggle())
  {
    // indicate that we are toggling
    // flag will be reset in SetFullscreen once its
    // called from XBMCs gui thread
    winSystem->SetFullscreenWillToggle(true);

    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_TOGGLEFULLSCREEN);
  }
  else
  {
    // in this case we are just called because
    // of xbmc did a toggle - just reset the flag
    // we don't need to do anything else
    winSystem->SetFullscreenWillToggle(false);
  }
}

- (void)windowDidExitFullScreen:(NSNotification*)pNotification
{
  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  // if osx is the issuer of the toggle
  // call XBMCs toggle function
  if (!winSystem->GetFullscreenWillToggle())
  {
    // indicate that we are toggling
    // flag will be reset in SetFullscreen once its
    // called from XBMCs gui thread
    winSystem->SetFullscreenWillToggle(true);
    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_TOGGLEFULLSCREEN);
  }
  else
  {
    // in this case we are just called because
    // of xbmc did a toggle - just reset the flag
    // we don't need to do anything else
    winSystem->SetFullscreenWillToggle(false);
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
@end
