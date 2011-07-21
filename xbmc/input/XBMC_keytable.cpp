/*
 *      Copyright (C) 2007-2010 Team XBMC
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

#include "system.h"
#include "utils/StdString.h"
#include "input/XBMC_vkeys.h"
#include "input/XBMC_keytable.h"

// The array of XBMCKEYTABLEs used in XBMC.
// The entries are in ascending order of sym; just for convenience.
// scancode, sym, unicode, ascii, vkey, keyname
static const XBMCKEYTABLE XBMCKeyTable[] =
{ { 0x0008,      0,    0, XBMCVK_BACK,   "backspace" }
, { 0x0009,      0,    0, XBMCVK_TAB,    "tab" }
, { 0x000d,      0,    0, XBMCVK_RETURN, "return" }
, { 0x001b,      0,    0, XBMCVK_ESCAPE, "escape" }
, {      0,      0,    0, XBMCVK_ESCAPE, "esc" } // Allowed abbreviation for "escape"

// Number keys on the main keyboard
, { 0x0030,    '0',  '0', XBMCVK_0, "zero" }
, { 0x0031,    '1',  '1', XBMCVK_1, "one" }
, { 0x0032,    '2',  '2', XBMCVK_2, "two" }
, { 0x0033,    '3',  '3', XBMCVK_3, "three" }
, { 0x0034,    '4',  '4', XBMCVK_4, "four" }
, { 0x0035,    '5',  '5', XBMCVK_5, "five" }
, { 0x0036,    '6',  '6', XBMCVK_6, "six" }
, { 0x0037,    '7',  '7', XBMCVK_7, "seven" }
, { 0x0038,    '8',  '8', XBMCVK_8, "eight" }
, { 0x0039,    '9',  '9', XBMCVK_9, "nine" }

// A to Z - note that upper case A-Z don't have a matching name or
// vkey. Only the lower case a-z are used in key mappings.
, { 0x0061,    'A',  'A', XBMCVK_A, NULL }
, { 0x0062,    'B',  'B', XBMCVK_B, NULL }
, { 0x0063,    'C',  'C', XBMCVK_C, NULL }
, { 0x0064,    'D',  'D', XBMCVK_D, NULL }
, { 0x0065,    'E',  'E', XBMCVK_E, NULL }
, { 0x0066,    'F',  'F', XBMCVK_F, NULL }
, { 0x0067,    'G',  'G', XBMCVK_G, NULL }
, { 0x0068,    'H',  'H', XBMCVK_H, NULL }
, { 0x0069,    'I',  'I', XBMCVK_I, NULL }
, { 0x006A,    'J',  'J', XBMCVK_J, NULL }
, { 0x006B,    'K',  'K', XBMCVK_K, NULL }
, { 0x006C,    'L',  'L', XBMCVK_L, NULL }
, { 0x006D,    'M',  'M', XBMCVK_M, NULL }
, { 0x006E,    'N',  'N', XBMCVK_N, NULL }
, { 0x006F,    'O',  'O', XBMCVK_O, NULL }
, { 0x0070,    'P',  'P', XBMCVK_P, NULL }
, { 0x0071,    'Q',  'Q', XBMCVK_Q, NULL }
, { 0x0072,    'R',  'R', XBMCVK_R, NULL }
, { 0x0073,    'S',  'S', XBMCVK_S, NULL }
, { 0x0074,    'T',  'T', XBMCVK_T, NULL }
, { 0x0075,    'U',  'U', XBMCVK_U, NULL }
, { 0x0076,    'V',  'V', XBMCVK_V, NULL }
, { 0x0077,    'W',  'W', XBMCVK_W, NULL }
, { 0x0078,    'X',  'X', XBMCVK_X, NULL }
, { 0x0079,    'Y',  'Y', XBMCVK_Y, NULL }
, { 0x007A,    'Z',  'Z', XBMCVK_Z, NULL }

, { 0x0061,    'a',  'a', XBMCVK_A, "a" }
, { 0x0062,    'b',  'b', XBMCVK_B, "b" }
, { 0x0063,    'c',  'c', XBMCVK_C, "c" }
, { 0x0064,    'd',  'd', XBMCVK_D, "d" }
, { 0x0065,    'e',  'e', XBMCVK_E, "e" }
, { 0x0066,    'f',  'f', XBMCVK_F, "f" }
, { 0x0067,    'g',  'g', XBMCVK_G, "g" }
, { 0x0068,    'h',  'h', XBMCVK_H, "h" }
, { 0x0069,    'i',  'i', XBMCVK_I, "i" }
, { 0x006a,    'j',  'j', XBMCVK_J, "j" }
, { 0x006b,    'k',  'k', XBMCVK_K, "k" }
, { 0x006c,    'l',  'l', XBMCVK_L, "l" }
, { 0x006d,    'm',  'm', XBMCVK_M, "m" }
, { 0x006e,    'n',  'n', XBMCVK_N, "n" }
, { 0x006f,    'o',  'o', XBMCVK_O, "o" }
, { 0x0070,    'p',  'p', XBMCVK_P, "p" }
, { 0x0071,    'q',  'q', XBMCVK_Q, "q" }
, { 0x0072,    'r',  'r', XBMCVK_R, "r" }
, { 0x0073,    's',  's', XBMCVK_S, "s" }
, { 0x0074,    't',  't', XBMCVK_T, "t" }
, { 0x0075,    'u',  'u', XBMCVK_U, "u" }
, { 0x0076,    'v',  'v', XBMCVK_V, "v" }
, { 0x0077,    'w',  'w', XBMCVK_W, "w" }
, { 0x0078,    'x',  'x', XBMCVK_X, "x" }
, { 0x0079,    'y',  'y', XBMCVK_Y, "y" }
, { 0x007a,    'z',  'z', XBMCVK_Z, "z" }

// Misc printing characters
, { 0x0020, 0x0020, 0x20, XBMCVK_SPACE,         "space" }
, { 0x0027,    '!',  '!', XBMCVK_EXCLAIM,       "exclaim" }
, { 0x0027,    '"',  '"', XBMCVK_QUOTEDBL,      "doublequote" }
, { 0x0027,    '#',  '#', XBMCVK_HASH,          "hash" }
, { 0x0027,    '$',  '$', XBMCVK_DOLLAR,        "dollar" }
, { 0x0027,    '%',  '%', XBMCVK_PERCENT,       "percent" }
, { 0x0027,    '&',  '&', XBMCVK_AMPERSAND,     "ampersand" }
, { 0x0027,   '\'', '\'', XBMCVK_QUOTE,         "quote" }
, { 0x0027,    '(',  '(', XBMCVK_LEFTPAREN,     "leftbracket" }
, { 0x0027,    ')',  ')', XBMCVK_RIGHTPAREN,    "rightbracket" }
, { 0x0027,    '*',  '*', XBMCVK_ASTERISK,      "asterisk" }
, { 0x003d,    '+',  '+', XBMCVK_PLUS,          "plus" }
, { 0x002c,    ',',  ',', XBMCVK_COMMA,         "comma" }
, { 0x002d,    '-',  '-', XBMCVK_MINUS,         "minus" }
, { 0x002e,    '.',  '.', XBMCVK_PERIOD,        "period" }
, { 0x002f,    '/',  '/', XBMCVK_SLASH,         "forwardslash" }

, { 0x003b,    ':',  ':', XBMCVK_COLON,         "colon" }
, { 0x003b,    ';',  ';', XBMCVK_SEMICOLON,     "semicolon" }
, { 0x002c,    '<',  '<', XBMCVK_LESS,          "lessthan" }
, { 0x003d,    '=',  '=', XBMCVK_EQUALS,        "equals" }
, { 0x002e,    '>',  '>', XBMCVK_GREATER,       "greaterthan" }
, { 0x002f,    '?',  '?', XBMCVK_QUESTION,      "questionmark" }
, { 0x002f,    '@',  '@', XBMCVK_AT,            "at" }

, { 0x005b,    '[',  '[', XBMCVK_LEFTBRACKET,   "opensquarebracket" }
, { 0x005c,   '\\', '\\', XBMCVK_BACKSLASH,     "backslash" }
, { 0x005d,    ']',  ']', XBMCVK_RIGHTBRACKET,  "closesquarebracket" }
, {      0,    '^',  '^', XBMCVK_CARET,         "caret" }
, { 0x002d,    '_',  '_', XBMCVK_UNDERSCORE,    "underline" }
, {      0,    '`',  '`', XBMCVK_BACKQUOTE,     "leftquote" }

, { 0x005b,    '{',  '{', XBMCVK_LEFTBRACE,     "openbrace" }
, { 0x005c,    '|',  '|', XBMCVK_PIPE,          "pipe" }
, { 0x005d,    '}',  '}', XBMCVK_RIGHTBRACE,    "closebrace" }
, { 0x0060,    '~',  '~', XBMCVK_TILDE,         "tilde" }

// Numeric keypad
, { 0x0100,    '0',  '0', XBMCVK_NUMPAD0,       "numpadzero"}
, { 0x0101,    '1',  '1', XBMCVK_NUMPAD1,       "numpadone"}
, { 0x0102,    '2',  '2', XBMCVK_NUMPAD2,       "numpadtwo"}
, { 0x0103,    '3',  '3', XBMCVK_NUMPAD3,       "numpadthree"}
, { 0x0104,    '4',  '4', XBMCVK_NUMPAD4,       "numpadfour"}
, { 0x0105,    '5',  '5', XBMCVK_NUMPAD5,       "numpadfive"}
, { 0x0106,    '6',  '6', XBMCVK_NUMPAD6,       "numpadsix"}
, { 0x0107,    '7',  '7', XBMCVK_NUMPAD7,       "numpadseven"}
, { 0x0108,    '8',  '8', XBMCVK_NUMPAD8,       "numpadeight"}
, { 0x0109,    '9',  '9', XBMCVK_NUMPAD9,       "numpadnine"}

, { 0x010b,    '/',  '/', XBMCVK_NUMPADDIVIDE,  "numpaddivide"}
, { 0x010c,    '*',  '*', XBMCVK_NUMPADTIMES,   "numpadtimes"}
, { 0x010d,    '-',  '-', XBMCVK_NUMPADMINUS,   "numpadminus"}
, { 0x010e,    '+',  '+', XBMCVK_NUMPADPLUS,    "numpadplus"}
, { 0x010f,      0,    0, XBMCVK_NUMPADENTER,   "enter"}
, { 0x010a,    '.',  '.', XBMCVK_NUMPADPERIOD,  "numpadperiod"}

// Multimedia keys
, { 0x00A6,      0,    0, XBMCVK_BROWSER_BACK,        "browser_back" }
, { 0x00A7,      0,    0, XBMCVK_BROWSER_FORWARD,     "browser_forward" }
, { 0x00A8,      0,    0, XBMCVK_BROWSER_REFRESH,     "browser_refresh" }
, { 0x00A9,      0,    0, XBMCVK_BROWSER_STOP,        "browser_stop" }
, { 0x00AA,      0,    0, XBMCVK_BROWSER_SEARCH,      "browser_search" }
, { 0x00AB,      0,    0, XBMCVK_BROWSER_FAVORITES,   "browser_favorites" }
, { 0x00AC,      0,    0, XBMCVK_BROWSER_HOME,        "browser_home" }
, { 0x00AD,      0,    0, XBMCVK_VOLUME_MUTE,         "volume_mute" }
, { 0x00AE,      0,    0, XBMCVK_VOLUME_DOWN,         "volume_down" }
, { 0x00AF,      0,    0, XBMCVK_VOLUME_UP,           "volume_up" }
, { 0x00B0,      0,    0, XBMCVK_MEDIA_NEXT_TRACK,    "next_track" }
, { 0x00B1,      0,    0, XBMCVK_MEDIA_PREV_TRACK,    "prev_track" }
, { 0x00B2,      0,    0, XBMCVK_MEDIA_STOP,          "stop" }
, { 0x00B3,      0,    0, XBMCVK_MEDIA_PLAY_PAUSE,    "play_pause" }
, { 0x00B4,      0,    0, XBMCVK_LAUNCH_MAIL,         "launch_mail" }
, { 0x00B5,      0,    0, XBMCVK_LAUNCH_MEDIA_SELECT, "launch_media_select" }
, { 0x00B6,      0,    0, XBMCVK_LAUNCH_APP1,         "launch_app1_pc_icon" }
, { 0x00B7,      0,    0, XBMCVK_LAUNCH_APP2,         "launch_app2_pc_icon" }
, { 0x00B8,      0,    0, XBMCVK_LAUNCH_FILE_BROWSER, "launch_file_browser" }
, { 0x00B9,      0,    0, XBMCVK_LAUNCH_MEDIA_CENTER, "launch_media_center" }

// Function keys
, { 0x011a,      0,    0, XBMCVK_F1,            "f1"}
, { 0x011b,      0,    0, XBMCVK_F2,            "f2"}
, { 0x011c,      0,    0, XBMCVK_F3,            "f3"}
, { 0x011d,      0,    0, XBMCVK_F4,            "f4"}
, { 0x011e,      0,    0, XBMCVK_F5,            "f5"}
, { 0x011f,      0,    0, XBMCVK_F6,            "f6"}
, { 0x0120,      0,    0, XBMCVK_F7,            "f7"}
, { 0x0121,      0,    0, XBMCVK_F8,            "f8"}
, { 0x0122,      0,    0, XBMCVK_F9,            "f9"}
, { 0x0123,      0,    0, XBMCVK_F10,           "f10"}
, { 0x0124,      0,    0, XBMCVK_F11,           "f11"}
, { 0x0125,      0,    0, XBMCVK_F12,           "f12"}
, { 0x0126,      0,    0, XBMCVK_F13,           "f13"}
, { 0x0127,      0,    0, XBMCVK_F14,           "f14"}
, { 0x0128,      0,    0, XBMCVK_F15,           "f15"}

// Misc non-printing keys
, { 0x0111,      0,    0, XBMCVK_UP,            "up" }
, { 0x0112,      0,    0, XBMCVK_DOWN,          "down" }
, { 0x0113,      0,    0, XBMCVK_RIGHT,         "right" }
, { 0x0114,      0,    0, XBMCVK_LEFT,          "left" }
, { 0x0115,      0,    0, XBMCVK_INSERT,        "insert" }
, { 0x007f,      0,    0, XBMCVK_DELETE,        "delete" }
, { 0x0116,      0,    0, XBMCVK_HOME,          "home" }
, { 0x0117,      0,    0, XBMCVK_END,           "end" }
, { 0x0118,      0,    0, XBMCVK_PAGEUP,        "pageup" }
, { 0x0119,      0,    0, XBMCVK_PAGEDOWN,      "pagedown" }
, { 0x012c,      0,    0, XBMCVK_NUMLOCK,       "numlock" }
, { 0x012d,      0,    0, XBMCVK_CAPSLOCK,      "capslock" }
, { 0x012e,      0,    0, XBMCVK_SCROLLLOCK,    "scrolllock" }
, { 0x012f,      0,    0, XBMCVK_RSHIFT,        "rightshift" }
, { 0x0130,      0,    0, XBMCVK_LSHIFT,        "leftshift" }
, { 0x0131,      0,    0, XBMCVK_RCONTROL,      "rightctrl" }
, { 0x0132,      0,    0, XBMCVK_LCONTROL,      "leftctrl" }
, { 0x0134,      0,    0, XBMCVK_LMENU,         "leftalt" }
, { 0x0137,      0,    0, XBMCVK_LWIN,          "leftwindows" }
, { 0x0138,      0,    0, XBMCVK_RWIN,          "rightwindows" }
, { 0x013c,      0,    0, XBMCVK_PRINTSCREEN,   "printscreen" }
, { 0x013f,      0,    0, XBMCVK_APP,           "menu" }
};

static int XBMCKeyTableSize = sizeof(XBMCKeyTable)/sizeof(XBMCKEYTABLE);

bool KeyTableLookupName(const char* keyname, XBMCKEYTABLE* keytable)
{
  // If the name being searched for is null or "" there will be no match
  if (!keyname)
    return false;
  if (keyname[0] == '\0')
    return false;

  // We need the button name to be in lowercase
  CStdString lkeyname = keyname;
  lkeyname.ToLower();

  // Look up the key name in XBMCKeyTable
  for (int i = 0; i < XBMCKeyTableSize; i++)
  { if (XBMCKeyTable[i].keyname)
    { if (strcmp(lkeyname.c_str(), XBMCKeyTable[i].keyname) == 0)
      { *keytable = XBMCKeyTable[i];
        return true;
      }
    }
  }

  // The name wasn't found
  return false;
}

bool KeyTableLookupSym(uint16_t sym, XBMCKEYTABLE* keytable)
{
  // If the sym being searched for is zero there will be no match
  if (sym == 0)
    return false;

  // Look up the sym in XBMCKeyTable
  for (int i = 0; i < XBMCKeyTableSize; i++)
  { if (sym == XBMCKeyTable[i].sym)
    { *keytable = XBMCKeyTable[i];
      return true;
    }
  }

  // The name wasn't found
  return false;
}

bool KeyTableLookupUnicode(uint16_t unicode, XBMCKEYTABLE* keytable)
{
  // If the unicode being searched for is zero there will be no match
  if (unicode == 0)
    return false;

  // Look up the unicode in XBMCKeyTable
  for (int i = 0; i < XBMCKeyTableSize; i++)
  { if (unicode == XBMCKeyTable[i].unicode)
    { *keytable = XBMCKeyTable[i];
      return true;
    }
  }

  // The name wasn't found
  return false;
}

bool KeyTableLookupSymAndUnicode(uint16_t sym, uint16_t unicode, XBMCKEYTABLE* keytable)
{
  // If the sym being searched for is zero there will be no match (the
  // unicode can be zero if the sym is non-zero)
  if (sym == 0)
    return false;

  // Look up the sym and unicode in XBMCKeyTable
  for (int i = 0; i < XBMCKeyTableSize; i++)
  { if (sym == XBMCKeyTable[i].sym && unicode == XBMCKeyTable[i].unicode)
    { *keytable = XBMCKeyTable[i];
      return true;
    }
  }

  // The sym and unicode weren't found
  return false;
}

bool KeyTableLookupVKeyName(uint32_t vkey, XBMCKEYTABLE* keytable)
{
  // If the vkey being searched for is zero there will be no match
  if (vkey == 0)
    return false;

  // Look up the vkey in XBMCKeyTable
  for (int i = 0; i < XBMCKeyTableSize; i++)
  { if (vkey == XBMCKeyTable[i].vkey && XBMCKeyTable[i].keyname)
    { *keytable = XBMCKeyTable[i];
      return true;
    }
  }

  // The name wasn't found
  return false;
}
