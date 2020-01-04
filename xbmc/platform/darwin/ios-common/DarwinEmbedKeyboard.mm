/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DarwinEmbedKeyboard.h"

#include "platform/darwin/ios-common/DarwinEmbedKeyboardView.h"
#include "platform/darwin/ios/IOSKeyboardView.h"
#include "platform/darwin/ios/XBMCController.h"

struct CDarwinEmbedKeyboardImpl
{
  IOSKeyboardView* g_pKeyboard = nil;
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

      //! @Todo generalise this block for platform
      //create the keyboardview
      IOSKeyboardView* __block keyboardView;
      dispatch_sync(dispatch_get_main_queue(), ^{
        // assume we are only drawn on the mainscreen ever!
        auto keyboardFrame = [g_xbmcController fullscreenSubviewFrame];
        keyboardView = [[IOSKeyboardView alloc] initWithFrame:keyboardFrame];
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
