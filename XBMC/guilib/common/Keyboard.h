#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../system.h" // imports HAS_SDL or others

#ifdef _XBOX
#include "XBoxKeyboard.h"
#elif defined(HAS_SDL)
#include "SDLKeyboard.h"
#else
#include "DirectInputKeyboard.h"
#endif

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
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
class CKeyboard : public CLowLevelKeyboard 
{
public:
  CKeyboard();
  ~CKeyboard();

  char GetAscii(); // for backwards compatibility only
  WCHAR GetUnicode();
};

extern CKeyboard g_Keyboard;

#endif
