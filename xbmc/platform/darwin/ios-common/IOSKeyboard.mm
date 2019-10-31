/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/darwin/ios-common/IOSKeyboard.h"

#include "platform/darwin/NSLogDebugHelpers.h"
#include "platform/darwin/ios-common/IOSKeyboardView.h"
#include "platform/darwin/ios/XBMCController.h"

class CIOSKeyboardImpl
{
public:
  KeyboardView* g_pIosKeyboard = nil;
};

CIOSKeyboard::CIOSKeyboard()
  : CGUIKeyboard(), m_pCharCallback{nullptr}, m_bCanceled{false}, m_impl{new CIOSKeyboardImpl}
{
}

bool CIOSKeyboard::ShowAndGetInput(char_callback_t pCallback,
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
      if (m_impl->g_pIosKeyboard)
        return false;

      // assume we are only drawn on the mainscreen ever!
      auto keyboardFrame = [g_xbmcController fullscreenSubviewFrame];

      //create the keyboardview
      m_impl->g_pIosKeyboard = [[KeyboardView alloc] initWithFrame:keyboardFrame];
      if (!m_impl->g_pIosKeyboard)
        return false;

      // inform the controller that the native keyboard is active
      // basically as long as m_impl->g_pIosKeyboard exists...
      [g_xbmcController nativeKeyboardActive:true];
    }

    m_pCharCallback = pCallback;

    // init keyboard stuff
    SetTextToKeyboard(initialString);
    [m_impl->g_pIosKeyboard setHidden:bHiddenInput];
    [m_impl->g_pIosKeyboard setHeading:[NSString stringWithUTF8String:heading.c_str()]];
    [m_impl->g_pIosKeyboard registerKeyboard:this]; // for calling back
    bool confirmed = false;
    if (!m_bCanceled)
    {
      [m_impl->g_pIosKeyboard setCancelFlag:&m_bCanceled];
      [m_impl->g_pIosKeyboard activate]; // blocks and shows keyboard
      // user is done - get resulted text and confirmation
      confirmed = m_impl->g_pIosKeyboard.isConfirmed;
      if (confirmed)
        typedString = [m_impl->g_pIosKeyboard.text UTF8String];
    }
    @synchronized([KeyboardView class])
    {
      m_impl->g_pIosKeyboard = nil;
      [g_xbmcController nativeKeyboardActive:false];
    }
    return confirmed;
  }
}

void CIOSKeyboard::Cancel()
{
  m_bCanceled = true;
}

bool CIOSKeyboard::SetTextToKeyboard(const std::string& text, bool closeKeyboard /* = false */)
{
  if (!m_impl->g_pIosKeyboard)
    return false;
  [m_impl->g_pIosKeyboard setKeyboardText:[NSString stringWithUTF8String:text.c_str()]
                            closeKeyboard:closeKeyboard ? YES : NO];
  return true;
}

//wrap our callback between objc and c++
void CIOSKeyboard::fireCallback(const std::string& str)
{
  if (m_pCharCallback)
  {
    m_pCharCallback(this, str);
  }
}
