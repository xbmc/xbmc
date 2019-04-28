/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/tvos/MainKeyboard.h"

#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/tvos/MainController.h"
#import "platform/darwin/tvos/MainKeyboardView.h"

#import "platform/darwin/AutoPool.h"

KeyboardView* g_pTvosKeyboard = nil;

bool CMainKeyboard::ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput)
{
  // we are in the MCRuntimeLib thread so we need a pool
  CCocoaAutoPool pool;

  @synchronized([KeyboardView class])
  {
    // in case twice open keyboard.
    if (g_pTvosKeyboard)
      return false;

    // assume we are only drawn on the mainscreen ever!
    UIScreen* pCurrentScreen = [UIScreen mainScreen];
    CGRect keyboardFrame = CGRectMake(0, 0, pCurrentScreen.bounds.size.height, pCurrentScreen.bounds.size.width);
//    LOG(@"kb: kb frame: %@", NSStringFromCGRect(keyboardFrame));

    //create the keyboardview
    g_pTvosKeyboard = [[KeyboardView alloc] initWithFrame:keyboardFrame];
    if (!g_pTvosKeyboard)
      return false;
  }

  m_pCharCallback = pCallback;

  // init keyboard stuff
  SetTextToKeyboard(initialString);
  [g_pTvosKeyboard setHidden:bHiddenInput];
  [g_pTvosKeyboard setHeading:[NSString stringWithUTF8String:heading.c_str()]];
  [g_pTvosKeyboard registerKeyboard:this]; // for calling back
  bool confirmed = false;
  if (!m_bCanceled)
  {
    [g_pTvosKeyboard setCancelFlag:&m_bCanceled];
    [g_pTvosKeyboard activate]; // blocks and shows keyboard
    // user is done - get resulted text and confirmation
    confirmed = g_pTvosKeyboard.isConfirmed;
    if (confirmed)
      typedString = [g_pTvosKeyboard._text UTF8String];
  }
  [g_pTvosKeyboard release]; // bye bye native keyboard
  @synchronized([KeyboardView class])
  {
    g_pTvosKeyboard = nil;
  }
  return confirmed;
}

void CMainKeyboard::Cancel()
{
  m_bCanceled = true;
}

bool CMainKeyboard::SetTextToKeyboard(const std::string &text, bool closeKeyboard /* = false */)
{
  if (!g_pTvosKeyboard)
    return false;
  [g_pTvosKeyboard setKeyboardText:[NSString stringWithUTF8String:text.c_str()] closeKeyboard:closeKeyboard?YES:NO];
  return true;
}

//wrap our callback between objc and c++
void CMainKeyboard::fireCallback(const std::string &str)
{
  if(m_pCharCallback)
  {
    m_pCharCallback(this, str);
  }
}
