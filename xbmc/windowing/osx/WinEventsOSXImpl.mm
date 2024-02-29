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

#include <mutex>
#include <optional>
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

  m_lastAppCursorVisibilityAction = NSCursorVisibilityBalancer::NONE;
  [self enableInputEvents];
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
    case NSEnterCharacter:
      return XBMCK_RETURN;
    case NSF1FunctionKey:
      return XBMCK_F1;
    case NSF2FunctionKey:
      return XBMCK_F2;
    case NSF3FunctionKey:
      return XBMCK_F3;
    case NSF4FunctionKey:
      return XBMCK_F4;
    case NSF5FunctionKey:
      return XBMCK_F5;
    case NSF6FunctionKey:
      return XBMCK_F6;
    case NSF7FunctionKey:
      return XBMCK_F7;
    case NSF8FunctionKey:
      return XBMCK_F8;
    case NSF9FunctionKey:
      return XBMCK_F9;
    case NSF10FunctionKey:
      return XBMCK_F10;
    case NSF11FunctionKey:
      return XBMCK_F11;
    case NSF12FunctionKey:
      return XBMCK_F12;
    case NSF13FunctionKey:
      return XBMCK_F13;
    case NSF14FunctionKey:
      return XBMCK_F14;
    case NSF15FunctionKey:
      return XBMCK_F15;
    case NSHomeFunctionKey:
      return XBMCK_HOME;
    case NSEndFunctionKey:
      return XBMCK_END;
    case NSPageDownFunctionKey:
      return XBMCK_PAGEDOWN;
    case NSPageUpFunctionKey:
      return XBMCK_PAGEUP;
    case NSPauseFunctionKey:
      return XBMCK_PAUSE;
    case NSInsertCharFunctionKey:
      return XBMCK_INSERT;
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
  XBMC_Event newEvent = {};

  switch (nsEvent.type)
  {
    // handle mouse events and transform them into the xbmc event world
    case NSEventTypeLeftMouseUp:
    {
      [self SendXBMCMouseButtonEvent:nsEvent
                           xbmcEvent:newEvent
                      mouseEventType:XBMC_MOUSEBUTTONUP
                          buttonCode:XBMC_BUTTON_LEFT];
      break;
    }
    case NSEventTypeLeftMouseDown:
    {
      [self SendXBMCMouseButtonEvent:nsEvent
                           xbmcEvent:newEvent
                      mouseEventType:XBMC_MOUSEBUTTONDOWN
                          buttonCode:XBMC_BUTTON_LEFT];
      break;
    }
    case NSEventTypeRightMouseUp:
    {
      [self SendXBMCMouseButtonEvent:nsEvent
                           xbmcEvent:newEvent
                      mouseEventType:XBMC_MOUSEBUTTONUP
                          buttonCode:XBMC_BUTTON_RIGHT];
      break;
    }
    case NSEventTypeRightMouseDown:
    {
      [self SendXBMCMouseButtonEvent:nsEvent
                           xbmcEvent:newEvent
                      mouseEventType:XBMC_MOUSEBUTTONDOWN
                          buttonCode:XBMC_BUTTON_RIGHT];
      break;
    }
    case NSEventTypeOtherMouseUp:
    {
      [self SendXBMCMouseButtonEvent:nsEvent
                           xbmcEvent:newEvent
                      mouseEventType:XBMC_MOUSEBUTTONUP
                          buttonCode:XBMC_BUTTON_MIDDLE];
      break;
    }
    case NSEventTypeOtherMouseDown:
    {
      [self SendXBMCMouseButtonEvent:nsEvent
                           xbmcEvent:newEvent
                      mouseEventType:XBMC_MOUSEBUTTONDOWN
                          buttonCode:XBMC_BUTTON_MIDDLE];
      break;
    }
    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
    {
      auto location = [self TranslateMouseLocation:nsEvent];
      if (location.has_value())
      {
        NSPoint locationCoordinates = location.value();
        newEvent.type = XBMC_MOUSEMOTION;
        newEvent.motion.x = locationCoordinates.x;
        newEvent.motion.y = locationCoordinates.y;
        [self MessagePush:&newEvent];
      }
      break;
    }
    case NSEventTypeScrollWheel:
    {
      // very strange, real scrolls have non-zero deltaY followed by same number of events
      // with a zero deltaY. This reverses our scroll which is WTF? anoying. Trap them out here.
      if (nsEvent.deltaY != 0.0)
      {
        auto button = nsEvent.scrollingDeltaY > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
        if ([self SendXBMCMouseButtonEvent:nsEvent
                                 xbmcEvent:newEvent
                            mouseEventType:XBMC_MOUSEBUTTONDOWN
                                buttonCode:button])
        {
          // scrollwhell need a subsquent button press with no button code
          [self SendXBMCMouseButtonEvent:nsEvent
                               xbmcEvent:newEvent
                          mouseEventType:XBMC_MOUSEBUTTONUP
                              buttonCode:std::nullopt];
        }
      }
      break;
    }

    // handle keyboard events and transform them into the xbmc event world
    case NSEventTypeKeyUp:
    {
      newEvent = [self keyPressEvent:nsEvent];
      newEvent.type = XBMC_KEYUP;

      [self MessagePush:&newEvent];
      passEvent = false;
      break;
    }
    case NSEventTypeKeyDown:
    {
      newEvent = [self keyPressEvent:nsEvent];
      newEvent.type = XBMC_KEYDOWN;

      if (![self ProcessOSXShortcuts:newEvent])
        [self MessagePush:&newEvent];
      passEvent = false;

      break;
    }
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

- (BOOL)SendXBMCMouseButtonEvent:(NSEvent*)nsEvent
                       xbmcEvent:(XBMC_Event&)xbmcEvent
                  mouseEventType:(uint8_t)mouseEventType
                      buttonCode:(std::optional<uint8_t>)buttonCode
{
  auto location = [self TranslateMouseLocation:nsEvent];
  if (location.has_value())
  {
    NSPoint locationCoordinates = location.value();
    xbmcEvent.type = mouseEventType;
    if (buttonCode.has_value())
    {
      xbmcEvent.button.button = buttonCode.value();
    }
    xbmcEvent.button.x = locationCoordinates.x;
    xbmcEvent.button.y = locationCoordinates.y;
    [self MessagePush:&xbmcEvent];
    return true;
  }
  return false;
}

- (std::optional<NSPoint>)TranslateMouseLocation:(NSEvent*)nsEvent
{
  NSPoint location = nsEvent.locationInWindow;
  // ignore events if outside the view bounds
  if (!nsEvent.window || !NSPointInRect(location, nsEvent.window.contentView.frame))
  {
    return std::nullopt;
  }
  // translate the location to backing units
  location = [nsEvent.window convertPointToBacking:location];
  NSRect frame = [nsEvent.window convertRectToBacking:nsEvent.window.contentView.frame];
  // cocoa world is upside down ...
  location.y = frame.size.height - location.y;
  return location;
}

@end
