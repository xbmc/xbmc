#include "Keyboard.h"
#include "KeyboardLayoutConfiguration.h"

//
// C++ Implementation: CKeyboard
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

// Comment OUT, if not really debugging!!!:
//#define DEBUG_KEYBOARD_GETCHAR

CKeyboard g_Keyboard; // global

CKeyboard::CKeyboard()
{
}

CKeyboard::~CKeyboard()
{
}

char CKeyboard::GetAscii()
{
  char lowLevelAscii = CLowLevelKeyboard::GetAscii();
  int translatedAscii = GetUnicode(); 

#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "low level ascii: %c ", lowLevelAscii);
  CLog::Log(LOGDEBUG, "low level ascii code: %d ", lowLevelAscii);
  CLog::Log(LOGDEBUG, "result char: %c ", translatedAscii);
  CLog::Log(LOGDEBUG, "result char code: %d ", translatedAscii);
  CLog::Log(LOGDEBUG, "ralt is pressed bool: %d ", GetRAlt());
  CLog::Log(LOGDEBUG, "shift is pressed bool: %d ", GetShift());
#endif

  if (translatedAscii >= 0 && translatedAscii < 128) // only TRUE ASCII! Otherwise XBMC crashes! No unicode not even latin 1!
    return translatedAscii; // mapping to ASCII is supported only if the result is TRUE ASCII
  else
    return lowLevelAscii; // old style
}

WCHAR CKeyboard::GetUnicode()
{
  // More specific mappings, i.e. with scancodes and/or with one or even more modifiers,
  // must be handled first/prioritized over less specific mappings! Why?
  // Example: an us keyboard has: "]" on one key, the german keyboard has "+" on the same key,
  // additionally the german keyboard has "~" on the same key, but the "~" 
  // can only be reached with the special modifier "AltGr" (right alt).
  // See http://en.wikipedia.org/wiki/Keyboard_layout.
  // If "+" is handled first, the key is already consumed and "~" can never be reached.
  // The least specific mappings, e.g. "regardless modifiers" should be done at last/least prioritized.

  WCHAR lowLevelUnicode = CLowLevelKeyboard::GetUnicode();
  BYTE key = CLowLevelKeyboard::GetKey();

#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "low level unicode char: %c ", lowLevelUnicode);
  CLog::Log(LOGDEBUG, "low level unicode code: %d ", lowLevelUnicode);
  CLog::Log(LOGDEBUG, "low level vkey: %d ", key);
  CLog::Log(LOGDEBUG, "ralt is pressed bool: %d ", GetRAlt());
  CLog::Log(LOGDEBUG, "shift is pressed bool: %d ", GetShift());
#endif

  if (GetRAlt())
  {
    if (g_keyboardLayoutConfiguration.containsDeriveXbmcCharFromVkeyWithRalt(key))
    {
      WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfDeriveXbmcCharFromVkeyWithRalt(key);
#ifdef DEBUG_KEYBOARD_GETCHAR
      CLog::Log(LOGDEBUG, "derived with ralt to code: %d ", resultUnicode);
#endif
      return resultUnicode;
    }
  }

  if (GetShift())
  {
    if (g_keyboardLayoutConfiguration.containsDeriveXbmcCharFromVkeyWithShift(key))
    {
      WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfDeriveXbmcCharFromVkeyWithShift(key);
#ifdef DEBUG_KEYBOARD_GETCHAR
      CLog::Log(LOGDEBUG, "derived with shift to code: %d ", resultUnicode);
#endif
      return resultUnicode;
    }
  }

  if (g_keyboardLayoutConfiguration.containsDeriveXbmcCharFromVkeyRegardlessModifiers(key))
  {
    WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfDeriveXbmcCharFromVkeyRegardlessModifiers(key);
#ifdef DEBUG_KEYBOARD_GETCHAR
    CLog::Log(LOGDEBUG, "derived to code: %d ", resultUnicode);
#endif
    return resultUnicode;
  }

  if (GetRAlt())
  {
    if (g_keyboardLayoutConfiguration.containsChangeXbmcCharWithRalt(lowLevelUnicode))
    {
      WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfChangeXbmcCharWithRalt(lowLevelUnicode);
#ifdef DEBUG_KEYBOARD_GETCHAR
      CLog::Log(LOGDEBUG, "changed char with ralt to code: %d ", resultUnicode);
#endif
      return resultUnicode;
    };
  }

  if (g_keyboardLayoutConfiguration.containsChangeXbmcCharRegardlessModifiers(lowLevelUnicode))
  {
    WCHAR resultUnicode = g_keyboardLayoutConfiguration.valueOfChangeXbmcCharRegardlessModifiers(lowLevelUnicode);
#ifdef DEBUG_KEYBOARD_GETCHAR
    CLog::Log(LOGDEBUG, "changed char to code: %d ", resultUnicode);
#endif
    return resultUnicode;
  };
  
  return lowLevelUnicode;
}
