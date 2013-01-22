/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#import "AutoPool.h"


bool CIOSKeyboard::ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput)
{
  CCocoaAutoPool pool;
  bool confirmed = false;
  CGRect keyboardFrame;

  // assume we are only drawn on the mainscreen ever!
  UIScreen *pCurrentScreen = [UIScreen mainScreen];
  CGFloat scale = [g_xbmcController getScreenScale:pCurrentScreen];
  int frameHeight = 30;
  keyboardFrame.size.width = pCurrentScreen.bounds.size.height - frameHeight;
  keyboardFrame.size.height = frameHeight;
  keyboardFrame.origin.x = frameHeight / 2;
  keyboardFrame.origin.y = (pCurrentScreen.bounds.size.width/2) - frameHeight*scale + 10;
  //create the keyboardview
  m_pIosKeyboard = [[KeyboardView alloc] initWithFrame:keyboardFrame];
  KeyboardView *keyboard = (KeyboardView*)m_pIosKeyboard;
  m_pCharCallback = pCallback;

  // init keyboard stuff
  [keyboard setText:[NSString stringWithUTF8String:initialString.c_str()]];
  [keyboard SetHiddenInput:bHiddenInput];
  [keyboard SetHeading:[NSString stringWithUTF8String:heading.c_str()]];
  [keyboard RegisterKeyboard:this]; // for calling back
  [keyboard activate];//blocks and loops our application loop (like a modal dialog)
  // user is done - get resulted text and confirmation
  typedString = [[keyboard GetText] UTF8String];
  confirmed = [keyboard GetResult];
  [keyboard release]; // bye bye native keyboard
  m_pIosKeyboard = NULL;
  return confirmed;
}

void CIOSKeyboard::Cancel()
{
  if (m_pIosKeyboard)
  {
    KeyboardView *keyboard = (KeyboardView*)m_pIosKeyboard;
    [keyboard keyboardDidHide:nil];
  }
}

//wrap our callback between objc and c++
void CIOSKeyboard::fireCallback(const std::string &str)
{
  if(m_pCharCallback)
  {
    m_pCharCallback(this, str);
  }
}
