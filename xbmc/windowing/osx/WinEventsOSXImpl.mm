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
#include "application/Application.h"
#include "input/XBMC_keysym.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseStat.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"
#include "windowing/osx/WinSystemOSX.h"

#include <memory>
#include <mutex>
#include <queue>

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h> // kvk_ANSI_ keycodes
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#pragma mark - objc implementation

@implementation CWinEventsOSXImpl
{
  std::queue<XBMC_Event> events;
  CCriticalSection m_inputlock;
  id mLocalMonitorId;

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
    case kVK_LeftArrow:
    case NSLeftArrowFunctionKey:
      return XBMCK_LEFT;
    case kVK_RightArrow:
    case NSRightArrowFunctionKey:
      return XBMCK_RIGHT;
    case kVK_UpArrow:
    case kVK_ANSI_RightBracket:
    case NSUpArrowFunctionKey:
      return XBMCK_UP;
    case kVK_DownArrow:
    case NSDownArrowFunctionKey:
      return XBMCK_DOWN;
    case kVK_Delete:
    case NSDeleteCharacter:
      return XBMCK_BACKSPACE;
    case kVK_Return:
      return XBMCK_RETURN;
    case kVK_ANSI_0:
      return XBMCK_0;
    case kVK_ANSI_1:
      return XBMCK_1;
    case kVK_ANSI_2:
      return XBMCK_2;
    case kVK_ANSI_3:
      return XBMCK_3;
    case kVK_ANSI_4:
      return XBMCK_4;
    case kVK_ANSI_5:
      return XBMCK_5;
    case kVK_ANSI_6:
      return XBMCK_6;
    case kVK_ANSI_7:
      return XBMCK_7;
    case kVK_ANSI_8:
      return XBMCK_8;
    case kVK_ANSI_9:
      return XBMCK_9;
    case kVK_ANSI_A:
      return XBMCK_a;
    case kVK_ANSI_B:
      return XBMCK_b;
    case kVK_ANSI_C:
      return XBMCK_c;
    case kVK_ANSI_D:
      return XBMCK_d;
    case kVK_ANSI_E:
      return XBMCK_e;
    case kVK_ANSI_F:
      return XBMCK_f;
    case kVK_ANSI_G:
      return XBMCK_g;
    case kVK_ANSI_H:
      return XBMCK_h;
    case kVK_ANSI_I:
      return XBMCK_i;
    case kVK_ANSI_J:
      return XBMCK_j;
    case kVK_ANSI_K:
      return XBMCK_k;
    case kVK_ANSI_L:
      return XBMCK_l;
    case kVK_ANSI_M:
      return XBMCK_m;
    case kVK_ANSI_N:
      return XBMCK_n;
    case kVK_ANSI_O:
      return XBMCK_o;
    case kVK_ANSI_P:
      return XBMCK_p;
    case kVK_ANSI_Q:
      return XBMCK_q;
    case kVK_ANSI_R:
      return XBMCK_r;
    case kVK_ANSI_S:
      return XBMCK_s;
    case kVK_ANSI_T:
      return XBMCK_t;
    case kVK_ANSI_U:
      return XBMCK_u;
    case kVK_ANSI_V:
      return XBMCK_v;
    case kVK_ANSI_W:
      return XBMCK_w;
    case kVK_ANSI_X:
      return XBMCK_x;
    case kVK_ANSI_Y:
      return XBMCK_y;
    case kVK_ANSI_Z:
      return XBMCK_z;
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
  // fn/globe
  if (appleModifier & kCGEventFlagMaskSecondaryFn && !(appleModifier & kCGEventFlagMaskNumericPad))
  {
    xbmcModifier |= XBMCKMOD_LMETA;
    xbmcModifier |= XBMCKMOD_MODE;
  }

  return static_cast<XBMCMod>(xbmcModifier);
}

- (bool)ProcessOSXShortcuts:(XBMC_Event&)event
{
  const auto cmd = (event.key.keysym.mod & (XBMCKMOD_LMETA | XBMCKMOD_RMETA)) != 0;
  const auto isFn = (event.key.keysym.mod & XBMCKMOD_MODE) != 0;
  if (cmd && event.type == XBMC_KEYDOWN)
  {
    switch (event.key.keysym.sym)
    {
      case XBMCK_q: // CMD-q to quit
        if (!g_application.m_bStop)
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
        return true;

      case XBMCK_CTRLF: // CMD-CTRL-f to toggle fullscreen
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
        return true;

      case XBMCK_f: // FN/Globe-f to toggle fullscreen
        if (isFn) // avoid cmd-f to toggle fullscreen
        {
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
          return true;
        }
        break;

      case XBMCK_t: // CMD-t to toggle float on top
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFLOATONTOP);
        return true;

      case XBMCK_s: // CMD-s to take a screenshot
      {
        CAction* action = new CAction(ACTION_TAKE_SCREENSHOT);
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                   static_cast<void*>(action));
        return true;
      }
      case XBMCK_h: // CMD-h to hide (but we minimize for now)
      case XBMCK_m: // CMD-m to minimize
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MINIMIZE);
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

  // clang-format off
  // Create an event tap. We are interested in mouse and keyboard events.
  NSEventMask eventMask =
      NSEventMaskKeyDown | NSEventMaskKeyUp |
      NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp |
      NSEventMaskRightMouseDown | NSEventMaskRightMouseUp |
      NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp |
      NSEventMaskScrollWheel |
      NSEventMaskLeftMouseDragged |
      NSEventMaskRightMouseDragged |
      NSEventMaskOtherMouseDragged |
      NSEventMaskMouseMoved;
  // clang-format on

  mLocalMonitorId = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask
                                                          handler:^(NSEvent* event) {
                                                            return [self InputEventHandler:event];
                                                          }];
}

- (void)disableInputEvents
{
  // Disable the local Monitor
  if (mLocalMonitorId != nil)
    [NSEvent removeMonitor:mLocalMonitorId];
  mLocalMonitorId = nil;
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

- (NSEvent*)InputEventHandler:(NSEvent*)nsevent
{
  bool passEvent = true;
  CGEventRef event = nsevent.CGEvent;
  CGEventType type = CGEventGetType(event);

  // The incoming mouse position.
  NSPoint location = nsevent.locationInWindow;
  if (!nsevent.window || location.x < 0 || location.y < 0)
    return nsevent;

  // cocoa world is upside down ...
  auto winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return nsevent;

  NSRect frame = winSystem->GetWindowDimensions();
  location.y = frame.size.height - location.y;

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
      newEvent = [self keyPressEvent:&event];
      newEvent.type = XBMC_KEYUP;

      [self MessagePush:&newEvent];
      passEvent = false;
      break;
    case kCGEventKeyDown:
      newEvent = [self keyPressEvent:&event];
      newEvent.type = XBMC_KEYDOWN;

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

- (XBMC_Event)keyPressEvent:(CGEventRef*)event
{
  UniCharCount actualStringLength = 0;
  // Get stringlength of event
  CGEventKeyboardGetUnicodeString(*event, 0, &actualStringLength, nullptr);

  // Create array with size of event string
  UniChar unicodeString[actualStringLength];
  memset(unicodeString, 0, sizeof(unicodeString));

  auto keycode =
      static_cast<CGKeyCode>(CGEventGetIntegerValueField(*event, kCGKeyboardEventKeycode));
  CGEventKeyboardGetUnicodeString(*event, sizeof(unicodeString) / sizeof(*unicodeString),
                                  &actualStringLength, unicodeString);

  XBMC_Event newEvent = {};

  // May be possible for actualStringLength > 1. Havent been able to replicate anything
  // larger than 1, but keep in mind for any regressions
  if (actualStringLength == 0)
  {
    return newEvent;
  }
  else if (actualStringLength > 1)
  {
    CLog::Log(LOGERROR, "CWinEventsOSXImpl::keyPressEvent - event string > 1 - size: {}",
              static_cast<int>(actualStringLength));
    return newEvent;
  }

  auto keyModifiers = [self OsxMod2XbmcMod:CGEventGetFlags(*event)];
  // - Unicode control characters (0-31) are usually mapped to key combos the application
  // might use as shortcuts (e.g. ctrl-shift-d for toggledebug).
  // Since they are not actual characters, translate the key from cocoa world to XBMC and
  // propagate the event.
  // see https://en.wikipedia.org/wiki/List_of_Unicode_characters#Control_codes
  // - For all other events, we rely on the unicode char returned by macOS (they map directly
  // to XBMC keys anyway). This is also the case of MacOS shortcuts.
  if (unicodeString[0] <= 31 && !(keyModifiers & XBMCKMOD_LMETA))
  {
    unicodeString[0] = [self OsxKey2XbmcKey:keycode];
  }

  newEvent.key.keysym.scancode = keycode;
  newEvent.key.keysym.sym = static_cast<XBMCKey>(unicodeString[0]);
  newEvent.key.keysym.unicode = unicodeString[0];
  newEvent.key.keysym.mod = keyModifiers;

  return newEvent;
}

@end
