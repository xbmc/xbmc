#ifndef KEYBOARD_H
#define KEYBOARD_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define DEBUG_KEYBOARD
#include <xkbd.h>

class CLowLevelKeyboard
{
public:
  CLowLevelKeyboard();
  ~CLowLevelKeyboard();

  void Initialize(HWND hWnd);

  void Update();
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  bool GetRAlt() { return m_bRAlt;};
  char GetAscii() { return m_cAscii;}; // FIXME should be replaced completly by GetUnicode() 
  WCHAR GetUnicode() { return GetAscii();}; // FIXME HELPME is there any unicode feature available?
  BYTE GetKey() { return m_VKey;};

private:
  // variables for mouse state
  XINPUT_STATE m_KeyboardState[4*2];     // one for each port
  HANDLE m_hKeyboardDevice[4*2];    // handle to each device
  DWORD m_dwKeyboardPort;      // mask of ports that currently hold a keyboard
  XINPUT_DEBUG_KEYSTROKE m_CurrentKeyStroke;

  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  bool m_bRAlt;
  char m_cAscii;
  BYTE m_VKey;

  bool m_bInitialized;
  bool m_bKeyDown;
};

#endif

