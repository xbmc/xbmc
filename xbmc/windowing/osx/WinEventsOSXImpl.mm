/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsOSXImpl.h"

#include "AppInboundProtocol.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "input/XBMC_keysym.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseStat.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "windowing/osx/WinSystemOSX.h"

#include <list>
#include <memory>

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#pragma mark - objc implementation

@implementation CWinEventsOSXImpl
{
  std::list<XBMC_Event> events;
  CCriticalSection g_inputCond;
  id mLocalMonitorId;
}

#pragma mark - init

- (instancetype)init
{
  self = [super init];

  [self enableInputEvents];

  return self;
}

- (void)MessagePush:(XBMC_Event*)newEvent
{
  CSingleLock lock(g_inputCond);
  events.push_back(*newEvent);
}

- (bool)MessagePump
{

  bool ret = false;

  // Do not always loop, only pump the initial queued count events. else if ui keep pushing
  // events the loop won't finish then it will block xbmc main message loop.
  for (size_t pumpEventCount = [self GetQueueSize]; pumpEventCount > 0; --pumpEventCount)
  {
    // Pop up only one event per time since in App::OnEvent it may init modal dialog which init
    // deeper message loop and call the deeper MessagePump from there.
    XBMC_Event pumpEvent = {};
    {
      CSingleLock lock(g_inputCond);
      if (events.size() == 0)
        return ret;
      pumpEvent = events.front();
      events.pop_front();
    }
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      ret |= appPort->OnEvent(pumpEvent);
  }
  return ret;
}

- (size_t)GetQueueSize
{
  CSingleLock lock(g_inputCond);
  return events.size();
}

- (unichar)OsxKey2XbmcKey:(unichar)character
{
  switch (character)
  {
    case 0x1c:
    case 0xf702:
      return XBMCK_LEFT;
    case 0x1d:
    case 0xf703:
      return XBMCK_RIGHT;
    case 0x1e:
    case 0xf700:
      return XBMCK_UP;
    case 0x1f:
    case 0xf701:
      return XBMCK_DOWN;
    case 0x7f:
      return XBMCK_BACKSPACE;
    default:
      return character;
  }
}

- (XBMCMod)OsxMod2XbmcMod:(CGEventFlags)appleModifier
{
  unsigned int xbmcModifier = XBMCKMOD_NONE;
  // shift left
  if (appleModifier & kCGEventFlagMaskAlphaShift)
    xbmcModifier |= XBMCKMOD_LSHIFT;
  // shift right
  if (appleModifier & kCGEventFlagMaskShift)
    xbmcModifier |= XBMCKMOD_RSHIFT;
  // left ctrl
  if (appleModifier & kCGEventFlagMaskControl)
    xbmcModifier |= XBMCKMOD_LCTRL;
  // left alt/option
  if (appleModifier & kCGEventFlagMaskAlternate)
    xbmcModifier |= XBMCKMOD_LALT;
  // left command
  if (appleModifier & kCGEventFlagMaskCommand)
    xbmcModifier |= XBMCKMOD_LMETA;

  return (XBMCMod)xbmcModifier;
}

- (bool)ProcessOSXShortcuts:(XBMC_Event&)event
{
  bool cmd = !!(event.key.keysym.mod & (XBMCKMOD_LMETA | XBMCKMOD_RMETA));
  if (cmd && event.type == XBMC_KEYDOWN)
  {
    switch (event.key.keysym.sym)
    {
      case XBMCK_q: // CMD-q to quit
        if (!g_application.m_bStop)
          KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
        return true;

      case XBMCK_CTRLF: // CMD-f to toggle fullscreen
        KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_TOGGLEFULLSCREEN);
        return true;

      case XBMCK_s: // CMD-s to take a screenshot
      {
        CAction* action = new CAction(ACTION_TAKE_SCREENSHOT);
        KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(
            TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(action));
        return true;
      }
      case XBMCK_h: // CMD-h to hide (but we minimize for now)
      case XBMCK_m: // CMD-m to minimize
        KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_MINIMIZE);
        return true;

      default:
        return false;
    }
  }

  return false;
}

- (void)enableInputEvents
{
  [self disableInputEvents]; // allow only one registration at a time

  // Create an event tap. We are interested in mouse and keyboard events.
  NSEventMask eventMask =
      NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskRightMouseDown |
      NSEventMaskRightMouseUp | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged |
      NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp | NSEventMaskOtherMouseDragged |
      NSEventMaskMouseMoved | NSEventMaskScrollWheel | NSEventMaskKeyDown | NSEventMaskKeyUp;

  mLocalMonitorId = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask
                                                          handler:^(NSEvent* event) {
                                                            return [self InputEventHandler:event];
                                                          }];
}

- (void)disableInputEvents
{
  // Disable the local Monitor
  if (mLocalMonitorId != nil)
    [NSEvent removeMonitor:(id)mLocalMonitorId];
  mLocalMonitorId = nil;
}

- (NSEvent*)InputEventHandler:(NSEvent*)nsevent
{
  bool passEvent = true;
  CGEventRef event = nsevent.CGEvent;
  CGEventType type = CGEventGetType(event);

  // The incoming mouse position.
  NSPoint location = nsevent.locationInWindow;
  if (location.x < 0 || location.y < 0)
    return nsevent;

  // cocoa world is upside down ...
  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return nsevent;

  NSRect frame = winSystem->GetWindowDimensions();
  location.y = frame.size.height - location.y;

  UniChar unicodeString[10];
  UniCharCount actualStringLength = 10;
  CGKeyCode keycode;
  XBMC_Event newEvent = {};

  switch (type)
  {
    // handle mouse events and transform them into the xbmc event world
    case kCGEventLeftMouseUp:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_LEFT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventLeftMouseDown:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_LEFT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventRightMouseUp:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_RIGHT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventRightMouseDown:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_RIGHT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventOtherMouseUp:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_MIDDLE;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventOtherMouseDown:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_MIDDLE;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventMouseMoved:
    case kCGEventLeftMouseDragged:
    case kCGEventRightMouseDragged:
    case kCGEventOtherMouseDragged:
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.x = location.x;
      newEvent.motion.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case kCGEventScrollWheel:
      // very strange, real scrolls have non-zero deltaY followed by same number of events
      // with a zero deltaY. This reverses our scroll which is WTF? anoying. Trap them out here.
      if (nsevent.deltaY != 0.0)
      {
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.x = location.x;
        newEvent.button.y = location.y;
        newEvent.button.button =
            CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1) > 0
                ? XBMC_BUTTON_WHEELUP
                : XBMC_BUTTON_WHEELDOWN;
        [self MessagePush:&newEvent];

        newEvent.type = XBMC_MOUSEBUTTONUP;
        [self MessagePush:&newEvent];
      }
      break;

    // handle keyboard events and transform them into the xbmc event world
    case kCGEventKeyUp:
      keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
      CGEventKeyboardGetUnicodeString(event, sizeof(unicodeString) / sizeof(*unicodeString),
                                      &actualStringLength, unicodeString);
      if (actualStringLength > 0)
        unicodeString[0] = [self OsxKey2XbmcKey:unicodeString[0]];
      else
        unicodeString[0] = [self OsxKey2XbmcKey:[nsevent.characters characterAtIndex:0]];

      newEvent.type = XBMC_KEYUP;
      newEvent.key.keysym.scancode = keycode;
      newEvent.key.keysym.sym = (XBMCKey)unicodeString[0];
      newEvent.key.keysym.unicode = unicodeString[0];
      if (actualStringLength > 1)
        newEvent.key.keysym.unicode |= (unicodeString[1] << 8);
      newEvent.key.keysym.mod = [self OsxMod2XbmcMod:CGEventGetFlags(event)];
      [self MessagePush:&newEvent];
      passEvent = false;
      break;
    case kCGEventKeyDown:
      keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
      CGEventKeyboardGetUnicodeString(event, sizeof(unicodeString) / sizeof(*unicodeString),
                                      &actualStringLength, unicodeString);
      if (actualStringLength > 0)
        unicodeString[0] = [self OsxKey2XbmcKey:unicodeString[0]];
      else
        unicodeString[0] = [self OsxKey2XbmcKey:[nsevent.characters characterAtIndex:0]];

      newEvent.type = XBMC_KEYDOWN;
      newEvent.key.keysym.scancode = keycode;
      newEvent.key.keysym.sym = (XBMCKey)unicodeString[0];
      newEvent.key.keysym.unicode = unicodeString[0];
      if (actualStringLength > 1)
        newEvent.key.keysym.unicode |= (unicodeString[1] << 8);
      newEvent.key.keysym.mod = [self OsxMod2XbmcMod:CGEventGetFlags(event)];

      if (![self ProcessOSXShortcuts:newEvent])
        [self MessagePush:&newEvent];
      passEvent = false;

      break;
    default:
      return nsevent;
  }
  // We must return the event for it to be useful if not already handled
  if (passEvent)
    return nsevent;
  else
    return nullptr;
}

@end
