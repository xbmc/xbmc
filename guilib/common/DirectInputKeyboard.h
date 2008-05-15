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

class CKeyboard
{
public:
  CKeyboard();
  ~CKeyboard();

  void Initialize(HWND hWnd);
  void Acquire();
  void Update();
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  char GetAscii() { return m_cAscii;};
  BYTE GetKey() { return m_VKey;};

private:
  inline bool KeyDown(unsigned char key) const { return (m_keystate[key] & 0x80) ? true : false; };
  LPDIRECTINPUTDEVICE m_keyboard;
  unsigned char m_keystate[256];
  unsigned int m_keyDownLastFrame;

  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  char m_cAscii;
  BYTE m_VKey;

  bool m_bInitialized;
  bool m_bKeyDown;
};

extern CKeyboard g_Keyboard;

#endif

