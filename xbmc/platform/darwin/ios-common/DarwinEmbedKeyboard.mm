/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DarwinEmbedKeyboard.h"

#include "utils/log.h"

#include "platform/darwin/ios-common/DarwinEmbedKeyboardView.h"
#if defined(TARGET_DARWIN_IOS)
#include "platform/darwin/ios/IOSKeyboardView.h"
#include "platform/darwin/ios/XBMCController.h"

#import <GameController/GameController.h>
#define KEYBOARDVIEW_CLASS IOSKeyboardView
#elif defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/TVOSKeyboardView.h"
#include "platform/darwin/tvos/XBMCController.h"
#define KEYBOARDVIEW_CLASS TVOSKeyboardView
#endif

#define SHARED_INSTANCE_SELECTOR @selector(sharedInstance)
#define IS_IN_HARDWARE_KEYBOARD_MODE_SELECTOR @selector(isInHardwareKeyboardMode)

struct CDarwinEmbedKeyboardImpl
{
  KEYBOARDVIEW_CLASS* g_pKeyboard = nil;
};

CDarwinEmbedKeyboard::CDarwinEmbedKeyboard()
  : CGUIKeyboard(), m_impl{std::make_unique<CDarwinEmbedKeyboardImpl>()}
{
}

bool CDarwinEmbedKeyboard::ShowAndGetInput(char_callback_t pCallback,
                                           const std::string& initialString,
                                           std::string& typedString,
                                           const std::string& heading,
                                           bool bHiddenInput)
{
  // we are in xbmc main thread or python module thread.
  @autoreleasepool
  {
    @synchronized([KeyboardView class])
    {
      // in case twice open keyboard.
      if (m_impl->g_pKeyboard)
        return false;

      //create the keyboardview
      KEYBOARDVIEW_CLASS* __block keyboardView;
      dispatch_sync(dispatch_get_main_queue(), ^{
        // assume we are only drawn on the mainscreen ever!
        auto keyboardFrame = [g_xbmcController fullscreenSubviewFrame];
        keyboardView = [[KEYBOARDVIEW_CLASS alloc] initWithFrame:keyboardFrame];
      });
      ////////////////////////////////////////////
      if (!keyboardView)
        return false;
      m_impl->g_pKeyboard = keyboardView;

      // inform the controller that the native keyboard is active
      // basically as long as m_impl->g_pKeyboard exists...
      [g_xbmcController nativeKeyboardActive:true];
    }

    m_pCharCallback = pCallback;

    // init keyboard stuff
    SetTextToKeyboard(initialString);
    [m_impl->g_pKeyboard setHidden:bHiddenInput];
    [m_impl->g_pKeyboard setHeading:@(heading.c_str())];
    m_impl->g_pKeyboard.darwinEmbedKeyboard = this; // for calling back
    bool confirmed = false;
    if (!m_canceled)
    {
      [m_impl->g_pKeyboard setCancelFlag:&m_canceled];
      // workaround for multiple keyboardviews running quickly - race condition
      sleep(1);

      [m_impl->g_pKeyboard activate]; // blocks and shows keyboard
      // user is done - get resulted text and confirmation
      confirmed = [m_impl->g_pKeyboard isConfirmed];
      if (confirmed)
        typedString = m_impl->g_pKeyboard.text.UTF8String;
    }
    @synchronized([KeyboardView class])
    {
      m_impl->g_pKeyboard = nil;
      [g_xbmcController nativeKeyboardActive:false];
    }
    return confirmed;
  }
}

void CDarwinEmbedKeyboard::Cancel()
{
  m_canceled = true;
}

bool CDarwinEmbedKeyboard::SetTextToKeyboard(const std::string& text,
                                             bool closeKeyboard /* = false */)
{
  if (!m_impl->g_pKeyboard)
    return false;
  [m_impl->g_pKeyboard setKeyboardText:@(text.c_str()) closeKeyboard:closeKeyboard ? YES : NO];
  return true;
}

//wrap our callback between objc and c++
void CDarwinEmbedKeyboard::fireCallback(const std::string& str)
{
  if (m_pCharCallback)
    m_pCharCallback(this, str);
}

void CDarwinEmbedKeyboard::invalidateCallback()
{
  m_pCharCallback = nullptr;
}

bool CDarwinEmbedKeyboard::hasExternalKeyboard()
{
#if defined(TARGET_DARWIN_IOS)
  if (@available(iOS 14.0, *))
    return GCKeyboard.coalescedKeyboard != nil;

  // https://stackoverflow.com/questions/31991873/how-to-reliably-detect-if-an-external-keyboard-is-connected-on-ios-9
  const auto keyboardClassStr = "UIKeyboardImpl";
  auto keyboardClass = NSClassFromString(@(keyboardClassStr));
  if (!keyboardClass)
  {
    CLog::Log(LOGERROR, "{} {} class doesn't exist", __PRETTY_FUNCTION__, keyboardClassStr);
    return false;
  }

  const auto sharedInstanceSelectorStr = NSStringFromSelector(SHARED_INSTANCE_SELECTOR).UTF8String;
  if (![keyboardClass respondsToSelector:SHARED_INSTANCE_SELECTOR])
  {
    CLog::Log(LOGERROR, "{} {} doesn't respond to {}", __PRETTY_FUNCTION__, keyboardClassStr,
              sharedInstanceSelectorStr);
    return false;
  }

  id keyboard = [keyboardClass performSelector:SHARED_INSTANCE_SELECTOR];
  if (!keyboard)
  {
    CLog::Log(LOGERROR, "{} +[{} {}] returned nil", __PRETTY_FUNCTION__, keyboardClassStr,
              sharedInstanceSelectorStr);
    return false;
  }

  auto methodSignature =
      [keyboardClass instanceMethodSignatureForSelector:IS_IN_HARDWARE_KEYBOARD_MODE_SELECTOR];
  if (!methodSignature)
  {
    CLog::Log(LOGERROR, "{} impossible to retrieve method signature of -[{} {}]",
              __PRETTY_FUNCTION__, keyboardClassStr,
              NSStringFromSelector(IS_IN_HARDWARE_KEYBOARD_MODE_SELECTOR).UTF8String);
    return false;
  }

  BOOL isInHardwareKeyboardMode;
  auto inv = [NSInvocation invocationWithMethodSignature:methodSignature];
  inv.selector = IS_IN_HARDWARE_KEYBOARD_MODE_SELECTOR;
  [inv invokeWithTarget:keyboard];
  [inv getReturnValue:&isInHardwareKeyboardMode];

  bool result = isInHardwareKeyboardMode == YES;
  CLog::Log(LOGDEBUG, "{} hasExternalKeyboard: {}", __PRETTY_FUNCTION__, result);
  return result;
#else
  return false; // not implemented
#endif
}
