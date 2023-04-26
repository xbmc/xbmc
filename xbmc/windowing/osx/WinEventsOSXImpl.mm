/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsOSXImpl.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseStat.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "windowing/osx/WinSystemOSX.h"

#include <mutex>
#include <queue>

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#pragma mark - objc implementation

@implementation CWinEventsOSXImpl
{
  std::queue<XBMC_Event> events;
  CCriticalSection m_inputlock;
  bool m_inputEnabled;

  //! macOS requires the calls the NSCursor hide/unhide to be balanced
  enum class NSCursorVisibilityBalancer
  {
    NONE,
    HIDE,
    UNHIDE
  };
  NSCursorVisibilityBalancer m_lastAppCursorVisibilityAction;
}

#pragma mark - init

- (instancetype)init
{
  self = [super init];

  [self enableInputEvents];
  m_lastAppCursorVisibilityAction = NSCursorVisibilityBalancer::NONE;
  m_inputEnabled = true;

  return self;
}

- (void)MessagePush:(XBMC_Event*)newEvent
{
  std::unique_lock<CCriticalSection> lock(m_inputlock);
  events.emplace(*newEvent);
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
      std::unique_lock<CCriticalSection> lock(m_inputlock);
      if (events.size() == 0)
        return ret;
      pumpEvent = events.front();
      events.pop();
    }
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      ret |= appPort->OnEvent(pumpEvent);
  }
  return ret;
}

- (size_t)GetQueueSize
{
  std::unique_lock<CCriticalSection> lock(m_inputlock);
  return events.size();
}

- (unichar)OsxKey2XbmcKey:(unichar)character
{
  switch (character)
  {
    case NSLeftArrowFunctionKey:
      return XBMCK_LEFT;
    case NSRightArrowFunctionKey:
      return XBMCK_RIGHT;
    case NSUpArrowFunctionKey:
      return XBMCK_UP;
    case NSDownArrowFunctionKey:
      return XBMCK_DOWN;
    case NSBackspaceCharacter:
    case NSDeleteCharacter:
      return XBMCK_BACKSPACE;
    case NSCarriageReturnCharacter:
      return XBMCK_RETURN;
    default:
      return character;
  }
}

- (XBMCMod)OsxMod2XbmcMod:(NSEventModifierFlags)appleModifier
{
  unsigned int xbmcModifier = XBMCKMOD_NONE;
  // shift
  if (appleModifier & NSEventModifierFlagShift)
    xbmcModifier |= XBMCKMOD_SHIFT;
  // left ctrl
  if (appleModifier & NSEventModifierFlagControl)
    xbmcModifier |= XBMCKMOD_CTRL;
  // left alt/option
  if (appleModifier & NSEventModifierFlagOption)
    xbmcModifier |= XBMCKMOD_ALT;
  // left command
  if (appleModifier & NSEventModifierFlagCommand)
    xbmcModifier |= XBMCKMOD_LMETA;

  return static_cast<XBMCMod>(xbmcModifier);
}

- (bool)ProcessOSXShortcuts:(XBMC_Event&)event
{
  const auto cmd = (event.key.keysym.mod & (XBMCKMOD_LMETA | XBMCKMOD_RMETA)) != 0;
  if (cmd && event.type == XBMC_KEYDOWN)
  {
    switch (event.key.keysym.sym)
    {
      case XBMCK_s: // CMD-s to take a screenshot
      {
        CAction* action = new CAction(ACTION_TAKE_SCREENSHOT);
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                   static_cast<void*>(action));
        return true;
      }

      default:
        return false;
    }
  }

  return false;
}

- (void)enableInputEvents
{
  m_inputEnabled = true;
}

- (void)disableInputEvents
{
  m_inputEnabled = false;
}

- (void)signalMouseEntered
{
  if (m_lastAppCursorVisibilityAction != NSCursorVisibilityBalancer::HIDE)
  {
    m_lastAppCursorVisibilityAction = NSCursorVisibilityBalancer::HIDE;
    [NSCursor hide];
  }
}

- (void)signalMouseExited
{
  if (m_lastAppCursorVisibilityAction != NSCursorVisibilityBalancer::UNHIDE)
  {
    m_lastAppCursorVisibilityAction = NSCursorVisibilityBalancer::UNHIDE;
    [NSCursor unhide];
  }
}

- (void)ProcessInputEvent:(NSEvent*)nsEvent
{
  if (m_inputEnabled)
  {
    [self InputEventHandler:nsEvent];
  }
}

- (NSEvent*)InputEventHandler:(NSEvent*)nsEvent
{
  bool passEvent = true;
  // The incoming mouse position.
  NSPoint location = nsEvent.locationInWindow;
  if (!nsEvent.window || location.x < 0 || location.y < 0)
    return nsEvent;

  location = [nsEvent.window convertPointToBacking:location];

  // cocoa world is upside down ...
  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return nsEvent;

  NSRect frame = [nsEvent.window convertRectToBacking:winSystem->GetWindowDimensions()];
  location.y = frame.size.height - location.y;

  XBMC_Event newEvent = {};

  switch (nsEvent.type)
  {
    // handle mouse events and transform them into the xbmc event world
    case NSEventTypeLeftMouseUp:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_LEFT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeLeftMouseDown:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_LEFT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeRightMouseUp:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_RIGHT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeRightMouseDown:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_RIGHT;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeOtherMouseUp:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_MIDDLE;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeOtherMouseDown:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_MIDDLE;
      newEvent.button.x = location.x;
      newEvent.button.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.x = location.x;
      newEvent.motion.y = location.y;
      [self MessagePush:&newEvent];
      break;
    case NSEventTypeScrollWheel:
      // very strange, real scrolls have non-zero deltaY followed by same number of events
      // with a zero deltaY. This reverses our scroll which is WTF? anoying. Trap them out here.
      if (nsEvent.deltaY != 0.0)
      {
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.x = location.x;
        newEvent.button.y = location.y;
        newEvent.button.button =
            nsEvent.scrollingDeltaY > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
        [self MessagePush:&newEvent];

        newEvent.type = XBMC_MOUSEBUTTONUP;
        [self MessagePush:&newEvent];
      }
      break;

    // handle keyboard events and transform them into the xbmc event world
    case NSEventTypeKeyUp:
      newEvent = [self keyPressEvent:nsEvent];
      newEvent.type = XBMC_KEYUP;

      [self MessagePush:&newEvent];
      passEvent = false;
      break;
    case NSEventTypeKeyDown:
      newEvent = [self keyPressEvent:nsEvent];
      newEvent.type = XBMC_KEYDOWN;

      if (![self ProcessOSXShortcuts:newEvent])
        [self MessagePush:&newEvent];
      passEvent = false;

      break;
    default:
      return nsEvent;
  }
  // We must return the event for it to be useful if not already handled
  if (passEvent)
    return nsEvent;
  else
    return nullptr;
}

- (XBMC_Event)keyPressEvent:(NSEvent*)nsEvent
{
  XBMC_Event newEvent = {};

  // use characters to propagate the actual unicode character
  NSString* unicode = nsEvent.characters;
  // use charactersIgnoringModifiers to get the corresponding char without modifiers. This will
  // keep shift so, lower case it as kodi shortcuts might depend on it. modifiers are propagated
  // anyway in keysym.mod
  NSString* unicodeWithoutModifiers = [nsEvent.charactersIgnoringModifiers lowercaseString];

  // May be possible for actualStringLength > 1. Havent been able to replicate anything
  // larger than 1, but keep in mind for any regressions
  if (!unicode || unicode.length == 0 || !unicodeWithoutModifiers ||
      unicodeWithoutModifiers.length == 0)
  {
    return newEvent;
  }
  else if (unicode.length > 1)
  {
    CLog::Log(LOGERROR, "CWinEventsOSXImpl::keyPressEvent - event string > 1 - size: {}",
              unicode.length);
    return newEvent;
  }

  newEvent.key.keysym.scancode = nsEvent.keyCode;
  newEvent.key.keysym.sym =
      static_cast<XBMCKey>([self OsxKey2XbmcKey:[unicodeWithoutModifiers characterAtIndex:0]]);
  newEvent.key.keysym.unicode = [unicode characterAtIndex:0];
  newEvent.key.keysym.mod = [self OsxMod2XbmcMod:nsEvent.modifierFlags];

  return newEvent;
}

@end
