#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#pragma once

/*
 *      Copyright (C) 2007-2013 Team XBMC
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

//
// C++ Interface: CKeyboard
//
// Description: Adds features like international keyboard layout mapping on top of the
// platform specific low level keyboard classes.
// Here it must be done only once. Within the other mentioned classes it would have to be done several times.
//
// Keyboards alyways deliver printable characters, logical keys for functional behaviour, modifiers ... alongside
// Based on the same hardware with the same scancodes (also alongside) but delivered with different labels to customers
// the software must solve the mapping to the real labels. This is done here.
// The mapping must be specified by an xml configuration that should be able to access everything available,
// but this allows for double/redundant or ambiguous mapping definition, e.g.
// ASCII/unicode could be derived from scancodes, virtual keys, modifiers and/or other ASCII/unicode.

#include "windowing/XBMC_events.h"
#include "guilib/Key.h"

class CKeyboardStat
{
public:
  CKeyboardStat();
  ~CKeyboardStat();

  void Initialize();

  static CKey TranslateKey(XBMC_keysym& keysym);

  const CKey ProcessKeyDown(XBMC_keysym& keysym);
  void       ProcessKeyUp(void);

  CStdString GetKeyName(int KeyID);

private:
  static bool LookupSymAndUnicodePeripherals(XBMC_keysym &keysym, uint8_t *key, char *unicode);

  XBMC_keysym m_lastKeysym;
  unsigned int m_lastKeyTime;
};

extern CKeyboardStat g_Keyboard;

#endif
