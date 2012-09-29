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


bool CIOSKeyboard::ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput)
{
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
  KeyboardView *iosKeyboard = [[KeyboardView alloc] initWithFrame:keyboardFrame];

  m_pCharCallback = pCallback;

  // init keyboard stuff
  [iosKeyboard setText:[NSString stringWithUTF8String:initialString.c_str()]];
  [iosKeyboard SetHiddenInput:bHiddenInput];
  [iosKeyboard SetHeading:[NSString stringWithUTF8String:heading.c_str()]];
  [iosKeyboard RegisterKeyboard:this]; // for calling back
  [iosKeyboard activate];//blocks and loops our application loop (like a modal dialog)
  // user is done - get resulted text and confirmation
  typedString = [[iosKeyboard GetText] UTF8String];
  confirmed = [iosKeyboard GetResult];
  [iosKeyboard release]; // bye bye native keyboard
  return confirmed;
}

//wrap our callback between objc and c++
void CIOSKeyboard::fireCallback(const std::string &str)
{
  if(m_pCharCallback)
  {
    m_pCharCallback(this, str);
  }
}
