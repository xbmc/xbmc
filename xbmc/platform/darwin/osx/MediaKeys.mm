/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaKeys.h"

#include "ServiceBroker.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"

#import <AppKit/AppKit.h>
#import <IOKit/hidsystem/ev_keymap.h>
#import <dispatch/dispatch.h>

namespace
{
CGEventRef MediaKeyCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon)
{
  __block bool keyHandled = false;
  dispatch_sync(dispatch_get_main_queue(), ^{
    auto tap = (__bridge CMediaKeyTap*)refcon;
    NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
    if (nsEvent.type == NSEventTypeSystemDefined &&
        nsEvent.subtype == NX_SUBTYPE_AUX_CONTROL_BUTTONS)
    {
      const int keyCode = (([nsEvent data1] & 0xFFFF0000) >> 16);
      const int keyFlags = ([nsEvent data1] & 0x0000FFFF);
      const int keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA;
      if (keyState == 1) // if pressed
      {
        if ([tap HandleMediaKey:keyCode])
          keyHandled = true;
      }
    }
  });
  if (keyHandled)
    return nullptr;

  return event;
}
} // namespace

@implementation CMediaKeyTap
{
  CFMachPortRef m_portRef;
  CFRunLoopSourceRef m_sourceRef;
  CFRunLoopRef m_tapThreadURL;
  NSThread* m_mediaKeyTapThread;
}

- (void)dealloc
{
  [self disableMediaKeyTap];
}

- (void)enableMediaKeyTap
{
  if (m_portRef)
    return;

  m_portRef = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
                               CGEventMaskBit(NX_SYSDEFINED), MediaKeyCallback,
                               (__bridge void* __nullable)(self));
  if (!m_portRef)
  {
    CLog::LogF(LOGERROR, "Failed to create media key tap. Check app accessibility permissions.");
    return;
  }

  m_sourceRef = CFMachPortCreateRunLoopSource(kCFAllocatorSystemDefault, m_portRef, 0);
  if (!m_sourceRef)
  {
    CLog::LogF(LOGERROR, "Failed to create media key tap. Check app accessibility permissions.");
    return;
  }

  m_mediaKeyTapThread = [[NSThread alloc] initWithTarget:self
                                                selector:@selector(eventTapThread)
                                                  object:nil];
  [m_mediaKeyTapThread start];
}

- (void)disableMediaKeyTap
{
  if (m_tapThreadURL)
  {
    CFRunLoopStop(m_tapThreadURL);
    m_tapThreadURL = nullptr;
  }

  if (m_portRef)
  {
    CFMachPortInvalidate(m_portRef);
    CFRelease(m_portRef);
    m_portRef = nullptr;
  }

  if (m_sourceRef)
  {
    CFRelease(m_sourceRef);
    m_sourceRef = nullptr;
  }
}

- (void)eventTapThread
{
  m_tapThreadURL = CFRunLoopGetCurrent();
  CFRunLoopAddSource(m_tapThreadURL, m_sourceRef, kCFRunLoopCommonModes);
  CFRunLoopRun();
}

- (bool)HandleMediaKey:(int)keyCode
{
  bool intercepted = true;
  switch (keyCode)
  {
    case NX_KEYTYPE_PLAY:
    {
      [self SendPlayerAction:ACTION_PLAYER_PLAYPAUSE];
      break;
    }
    case NX_KEYTYPE_NEXT:
    {
      [self SendPlayerAction:ACTION_NEXT_ITEM];
      break;
    }
    case NX_KEYTYPE_PREVIOUS:
    {
      [self SendPlayerAction:ACTION_PREV_ITEM];
      break;
    }
    case NX_KEYTYPE_FAST:
    {
      [self SendPlayerAction:ACTION_PLAYER_FORWARD];
      break;
    }
    case NX_KEYTYPE_REWIND:
    {
      [self SendPlayerAction:ACTION_PLAYER_REWIND];
      break;
    }
    default:
    {
      intercepted = false;
      break;
    }
  }
  return intercepted;
}

- (void)SendPlayerAction:(int)actionId
{
  //! @TODO: This shouldn't depend on GUI/Actions (e.g. headless music player can also use mediakeys)
  CAction* action = new CAction(actionId);
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                             static_cast<void*>(action));
}

@end
