/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBMCController.h"
#include "IOSKeyboard.h"
#include "IOSKeyboardView.h"
#include "XBMCDebugHelpers.h"
#include "osx/DarwinUtils.h"

#import "AutoPool.h"

KeyboardView *g_pIosKeyboard = nil;

bool CIOSKeyboard::ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput)
{
  // we are in xbmc main thread or python module thread.

  CCocoaAutoPool pool;
  
  @synchronized([KeyboardView class])
  {
    // in case twice open keyboard.
    if (g_pIosKeyboard)
      return false;
    
    // assume we are only drawn on the mainscreen ever!
    UIScreen *pCurrentScreen = [UIScreen mainScreen];
    CGRect keyboardFrame = CGRectMake(0, 0, pCurrentScreen.bounds.size.height, pCurrentScreen.bounds.size.width);
#if __IPHONE_8_0
    if (CDarwinUtils::GetIOSVersion() >= 8.0)
      keyboardFrame = CGRectMake(0, 0, pCurrentScreen.bounds.size.width, pCurrentScreen.bounds.size.height);
#endif
//    LOG(@"kb: kb frame: %@", NSStringFromCGRect(keyboardFrame));
    
    //create the keyboardview
    g_pIosKeyboard = [[KeyboardView alloc] initWithFrame:keyboardFrame];
    if (!g_pIosKeyboard)
      return false;
  }

  m_pCharCallback = pCallback;

  // init keyboard stuff
  [g_pIosKeyboard setDefault:[NSString stringWithUTF8String:initialString.c_str()]];
  [g_pIosKeyboard setHidden:bHiddenInput];
  [g_pIosKeyboard setHeading:[NSString stringWithUTF8String:heading.c_str()]];
  [g_pIosKeyboard registerKeyboard:this]; // for calling back
  bool confirmed = false;
  if (!m_bCanceled)
  {
    [g_pIosKeyboard setCancelFlag:&m_bCanceled];
    [g_pIosKeyboard activate]; // blocks and loops our application loop (like a modal dialog)
    // user is done - get resulted text and confirmation
    confirmed = g_pIosKeyboard.isConfirmed;
    if (confirmed)
      typedString = [g_pIosKeyboard.text UTF8String];
  }
  [g_pIosKeyboard release]; // bye bye native keyboard
  @synchronized([KeyboardView class])
  {
    g_pIosKeyboard = nil;
  }
  return confirmed;
}

void CIOSKeyboard::Cancel()
{
  m_bCanceled = true;
}

bool CIOSKeyboard::SetTextToKeyboard(const std::string &text, bool closeKeyboard /* = false */)
{
  if (!g_pIosKeyboard)
    return false;
  [g_pIosKeyboard setKeyboardText:[NSString stringWithUTF8String:text.c_str()] closeKeyboard:closeKeyboard?YES:NO];
  return true;
}

//wrap our callback between objc and c++
void CIOSKeyboard::fireCallback(const std::string &str)
{
  if(m_pCharCallback)
  {
    m_pCharCallback(this, str);
  }
}
