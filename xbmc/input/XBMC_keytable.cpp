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

#include "system.h"
#include "utils/StdString.h"
#include "input/XBMC_keysym.h"
#include "input/XBMC_vkeys.h"
#include "input/XBMC_keytable.h"

// The array of XBMCKEYTABLEs used in XBMC.
// scancode, sym, unicode, ascii, vkey, keyname
static const XBMCKEYTABLE XBMCKeyTable[] =
{ { XBMCK_BACKSPACE, 0,    0, XBMCVK_BACK,   "backspace" }
, { XBMCK_TAB,       0,    0, XBMCVK_TAB,    "tab" }
, { XBMCK_RETURN,    0,    0, XBMCVK_RETURN, "return" }
, { XBMCK_ESCAPE,    0,    0, XBMCVK_ESCAPE, "escape" }
, { 0,               0,    0, XBMCVK_ESCAPE, "esc" } // Allowed abbreviation for "escape"

// Number keys on the main keyboard
, { XBMCK_0,    '0',  '0', XBMCVK_0, "zero" }
, { XBMCK_1,    '1',  '1', XBMCVK_1, "one" }
, { XBMCK_2,    '2',  '2', XBMCVK_2, "two" }
, { XBMCK_3,    '3',  '3', XBMCVK_3, "three" }
, { XBMCK_4,    '4',  '4', XBMCVK_4, "four" }
, { XBMCK_5,    '5',  '5', XBMCVK_5, "five" }
, { XBMCK_6,    '6',  '6', XBMCVK_6, "six" }
, { XBMCK_7,    '7',  '7', XBMCVK_7, "seven" }
, { XBMCK_8,    '8',  '8', XBMCVK_8, "eight" }
, { XBMCK_9,    '9',  '9', XBMCVK_9, "nine" }

// A to Z - note that upper case A-Z don't have a matching name or
// vkey. Only the lower case a-z are used in key mappings.
, { XBMCK_a,    'A',  'A', XBMCVK_A, NULL }
, { XBMCK_b,    'B',  'B', XBMCVK_B, NULL }
, { XBMCK_c,    'C',  'C', XBMCVK_C, NULL }
, { XBMCK_d,    'D',  'D', XBMCVK_D, NULL }
, { XBMCK_e,    'E',  'E', XBMCVK_E, NULL }
, { XBMCK_f,    'F',  'F', XBMCVK_F, NULL }
, { XBMCK_g,    'G',  'G', XBMCVK_G, NULL }
, { XBMCK_h,    'H',  'H', XBMCVK_H, NULL }
, { XBMCK_i,    'I',  'I', XBMCVK_I, NULL }
, { XBMCK_j,    'J',  'J', XBMCVK_J, NULL }
, { XBMCK_k,    'K',  'K', XBMCVK_K, NULL }
, { XBMCK_l,    'L',  'L', XBMCVK_L, NULL }
, { XBMCK_m,    'M',  'M', XBMCVK_M, NULL }
, { XBMCK_n,    'N',  'N', XBMCVK_N, NULL }
, { XBMCK_o,    'O',  'O', XBMCVK_O, NULL }
, { XBMCK_p,    'P',  'P', XBMCVK_P, NULL }
, { XBMCK_q,    'Q',  'Q', XBMCVK_Q, NULL }
, { XBMCK_r,    'R',  'R', XBMCVK_R, NULL }
, { XBMCK_s,    'S',  'S', XBMCVK_S, NULL }
, { XBMCK_t,    'T',  'T', XBMCVK_T, NULL }
, { XBMCK_u,    'U',  'U', XBMCVK_U, NULL }
, { XBMCK_v,    'V',  'V', XBMCVK_V, NULL }
, { XBMCK_w,    'W',  'W', XBMCVK_W, NULL }
, { XBMCK_x,    'X',  'X', XBMCVK_X, NULL }
, { XBMCK_y,    'Y',  'Y', XBMCVK_Y, NULL }
, { XBMCK_z,    'Z',  'Z', XBMCVK_Z, NULL }

, { XBMCK_a,    'a',  'a', XBMCVK_A, "a" }
, { XBMCK_b,    'b',  'b', XBMCVK_B, "b" }
, { XBMCK_c,    'c',  'c', XBMCVK_C, "c" }
, { XBMCK_d,    'd',  'd', XBMCVK_D, "d" }
, { XBMCK_e,    'e',  'e', XBMCVK_E, "e" }
, { XBMCK_f,    'f',  'f', XBMCVK_F, "f" }
, { XBMCK_g,    'g',  'g', XBMCVK_G, "g" }
, { XBMCK_h,    'h',  'h', XBMCVK_H, "h" }
, { XBMCK_i,    'i',  'i', XBMCVK_I, "i" }
, { XBMCK_j,    'j',  'j', XBMCVK_J, "j" }
, { XBMCK_k,    'k',  'k', XBMCVK_K, "k" }
, { XBMCK_l,    'l',  'l', XBMCVK_L, "l" }
, { XBMCK_m,    'm',  'm', XBMCVK_M, "m" }
, { XBMCK_n,    'n',  'n', XBMCVK_N, "n" }
, { XBMCK_o,    'o',  'o', XBMCVK_O, "o" }
, { XBMCK_p,    'p',  'p', XBMCVK_P, "p" }
, { XBMCK_q,    'q',  'q', XBMCVK_Q, "q" }
, { XBMCK_r,    'r',  'r', XBMCVK_R, "r" }
, { XBMCK_s,    's',  's', XBMCVK_S, "s" }
, { XBMCK_t,    't',  't', XBMCVK_T, "t" }
, { XBMCK_u,    'u',  'u', XBMCVK_U, "u" }
, { XBMCK_v,    'v',  'v', XBMCVK_V, "v" }
, { XBMCK_w,    'w',  'w', XBMCVK_W, "w" }
, { XBMCK_x,    'x',  'x', XBMCVK_X, "x" }
, { XBMCK_y,    'y',  'y', XBMCVK_Y, "y" }
, { XBMCK_z,    'z',  'z', XBMCVK_Z, "z" }

// Misc printing characters
, { XBMCK_SPACE,                ' ',  ' ', XBMCVK_SPACE,         "space" }
, { XBMCK_EXCLAIM,              '!',  '!', XBMCVK_EXCLAIM,       "exclaim" }
, { XBMCK_QUOTEDBL,             '"',  '"', XBMCVK_QUOTEDBL,      "doublequote" }
, { XBMCK_HASH,                 '#',  '#', XBMCVK_HASH,          "hash" }
, { XBMCK_DOLLAR,               '$',  '$', XBMCVK_DOLLAR,        "dollar" }
, { XBMCK_PERCENT,              '%',  '%', XBMCVK_PERCENT,       "percent" }
, { XBMCK_AMPERSAND,            '&',  '&', XBMCVK_AMPERSAND,     "ampersand" }
, { XBMCK_QUOTE,                '\'', '\'', XBMCVK_QUOTE,         "quote" }
, { XBMCK_LEFTPAREN,            '(',  '(', XBMCVK_LEFTPAREN,     "leftbracket" }
, { XBMCK_RIGHTPAREN,           ')',  ')', XBMCVK_RIGHTPAREN,    "rightbracket" }
, { XBMCK_ASTERISK,             '*',  '*', XBMCVK_ASTERISK,      "asterisk" }
, { XBMCK_PLUS,                 '+',  '+', XBMCVK_PLUS,          "plus" }
, { XBMCK_COMMA,                ',',  ',', XBMCVK_COMMA,         "comma" }
, { XBMCK_MINUS,                '-',  '-', XBMCVK_MINUS,         "minus" }
, { XBMCK_PERIOD,               '.',  '.', XBMCVK_PERIOD,        "period" }
, { XBMCK_SLASH,                '/',  '/', XBMCVK_SLASH,         "forwardslash" }

, { XBMCK_COLON,                ':',  ':', XBMCVK_COLON,         "colon" }
, { XBMCK_SEMICOLON,            ';',  ';', XBMCVK_SEMICOLON,     "semicolon" }
, { XBMCK_LESS,                 '<',  '<', XBMCVK_LESS,          "lessthan" }
, { XBMCK_EQUALS,               '=',  '=', XBMCVK_EQUALS,        "equals" }
, { XBMCK_GREATER,              '>',  '>', XBMCVK_GREATER,       "greaterthan" }
, { XBMCK_QUESTION,             '?',  '?', XBMCVK_QUESTION,      "questionmark" }
, { XBMCK_AT,                   '@',  '@', XBMCVK_AT,            "at" }

, { XBMCK_LEFTBRACKET,          '[',  '[', XBMCVK_LEFTBRACKET,   "opensquarebracket" }
, { XBMCK_BACKSLASH,            '\\', '\\', XBMCVK_BACKSLASH,     "backslash" }
, { XBMCK_RIGHTBRACKET,         ']',  ']', XBMCVK_RIGHTBRACKET,  "closesquarebracket" }
, { XBMCK_CARET,                '^',  '^', XBMCVK_CARET,         "caret" }
, { XBMCK_UNDERSCORE,           '_',  '_', XBMCVK_UNDERSCORE,    "underline" }
, { XBMCK_BACKQUOTE,            '`',  '`', XBMCVK_BACKQUOTE,     "leftquote" }

, { XBMCK_LEFTBRACE,            '{',  '{', XBMCVK_LEFTBRACE,     "openbrace" }
, { XBMCK_PIPE,                 '|',  '|', XBMCVK_PIPE,          "pipe" }
, { XBMCK_RIGHTBRACE,           '}',  '}', XBMCVK_RIGHTBRACE,    "closebrace" }
, { XBMCK_TILDE,                '~',  '~', XBMCVK_TILDE,         "tilde" }

// Numeric keypad
, { XBMCK_KP0,                  '0',  '0', XBMCVK_NUMPAD0,       "numpadzero"}
, { XBMCK_KP1,                  '1',  '1', XBMCVK_NUMPAD1,       "numpadone"}
, { XBMCK_KP2,                  '2',  '2', XBMCVK_NUMPAD2,       "numpadtwo"}
, { XBMCK_KP3,                  '3',  '3', XBMCVK_NUMPAD3,       "numpadthree"}
, { XBMCK_KP4,                  '4',  '4', XBMCVK_NUMPAD4,       "numpadfour"}
, { XBMCK_KP5,                  '5',  '5', XBMCVK_NUMPAD5,       "numpadfive"}
, { XBMCK_KP6,                  '6',  '6', XBMCVK_NUMPAD6,       "numpadsix"}
, { XBMCK_KP7,                  '7',  '7', XBMCVK_NUMPAD7,       "numpadseven"}
, { XBMCK_KP8,                  '8',  '8', XBMCVK_NUMPAD8,       "numpadeight"}
, { XBMCK_KP9,                  '9',  '9', XBMCVK_NUMPAD9,       "numpadnine"}

, { XBMCK_KP_DIVIDE,            '/',  '/', XBMCVK_NUMPADDIVIDE,  "numpaddivide"}
, { XBMCK_KP_MULTIPLY,          '*',  '*', XBMCVK_NUMPADTIMES,   "numpadtimes"}
, { XBMCK_KP_MINUS,             '-',  '-', XBMCVK_NUMPADMINUS,   "numpadminus"}
, { XBMCK_KP_PLUS,              '+',  '+', XBMCVK_NUMPADPLUS,    "numpadplus"}
, { XBMCK_KP_ENTER,               0,    0, XBMCVK_NUMPADENTER,   "enter"}
, { XBMCK_KP_PERIOD,            '.',  '.', XBMCVK_NUMPADPERIOD,  "numpadperiod"}

// Multimedia keys
, { XBMCK_BROWSER_BACK,           0,    0, XBMCVK_BROWSER_BACK,        "browser_back" }
, { XBMCK_BROWSER_FORWARD,        0,    0, XBMCVK_BROWSER_FORWARD,     "browser_forward" }
, { XBMCK_BROWSER_REFRESH,        0,    0, XBMCVK_BROWSER_REFRESH,     "browser_refresh" }
, { XBMCK_BROWSER_STOP,           0,    0, XBMCVK_BROWSER_STOP,        "browser_stop" }
, { XBMCK_BROWSER_SEARCH,         0,    0, XBMCVK_BROWSER_SEARCH,      "browser_search" }
, { XBMCK_BROWSER_FAVORITES,      0,    0, XBMCVK_BROWSER_FAVORITES,   "browser_favorites" }
, { XBMCK_BROWSER_HOME,           0,    0, XBMCVK_BROWSER_HOME,        "browser_home" }
, { XBMCK_VOLUME_MUTE,            0,    0, XBMCVK_VOLUME_MUTE,         "volume_mute" }
, { XBMCK_VOLUME_DOWN,            0,    0, XBMCVK_VOLUME_DOWN,         "volume_down" }
, { XBMCK_VOLUME_UP,              0,    0, XBMCVK_VOLUME_UP,           "volume_up" }
, { XBMCK_MEDIA_NEXT_TRACK,       0,    0, XBMCVK_MEDIA_NEXT_TRACK,    "next_track" }
, { XBMCK_MEDIA_PREV_TRACK,       0,    0, XBMCVK_MEDIA_PREV_TRACK,    "prev_track" }
, { XBMCK_MEDIA_STOP,             0,    0, XBMCVK_MEDIA_STOP,          "stop" }
, { XBMCK_MEDIA_PLAY_PAUSE,       0,    0, XBMCVK_MEDIA_PLAY_PAUSE,    "play_pause" }
, { XBMCK_LAUNCH_MAIL,            0,    0, XBMCVK_LAUNCH_MAIL,         "launch_mail" }
, { XBMCK_LAUNCH_MEDIA_SELECT,    0,    0, XBMCVK_LAUNCH_MEDIA_SELECT, "launch_media_select" }
, { XBMCK_LAUNCH_APP1,            0,    0, XBMCVK_LAUNCH_APP1,         "launch_app1_pc_icon" }
, { XBMCK_LAUNCH_APP2,            0,    0, XBMCVK_LAUNCH_APP2,         "launch_app2_pc_icon" }
, { XBMCK_LAUNCH_FILE_BROWSER,    0,    0, XBMCVK_LAUNCH_FILE_BROWSER, "launch_file_browser" }
, { XBMCK_LAUNCH_MEDIA_CENTER,    0,    0, XBMCVK_LAUNCH_MEDIA_CENTER, "launch_media_center" }
, { XBMCK_PLAY,                   0,    0, XBMCVK_MEDIA_PLAY_PAUSE,    "play_pause" }
, { XBMCK_STOP,                   0,    0, XBMCVK_MEDIA_STOP,          "stop" }
, { XBMCK_REWIND,                 0,    0, XBMCVK_MEDIA_REWIND,        "rewind" }
, { XBMCK_FASTFORWARD,            0,    0, XBMCVK_MEDIA_FASTFORWARD,   "fastforward" }
, { XBMCK_RECORD,                 0,    0, XBMCVK_MEDIA_RECORD,        "record" }


// Function keys
, { XBMCK_F1,                     0,    0, XBMCVK_F1,            "f1"}
, { XBMCK_F2,                     0,    0, XBMCVK_F2,            "f2"}
, { XBMCK_F3,                     0,    0, XBMCVK_F3,            "f3"}
, { XBMCK_F4,                     0,    0, XBMCVK_F4,            "f4"}
, { XBMCK_F5,                     0,    0, XBMCVK_F5,            "f5"}
, { XBMCK_F6,                     0,    0, XBMCVK_F6,            "f6"}
, { XBMCK_F7,                     0,    0, XBMCVK_F7,            "f7"}
, { XBMCK_F8,                     0,    0, XBMCVK_F8,            "f8"}
, { XBMCK_F9,                     0,    0, XBMCVK_F9,            "f9"}
, { XBMCK_F10,                    0,    0, XBMCVK_F10,           "f10"}
, { XBMCK_F11,                    0,    0, XBMCVK_F11,           "f11"}
, { XBMCK_F12,                    0,    0, XBMCVK_F12,           "f12"}
, { XBMCK_F13,                    0,    0, XBMCVK_F13,           "f13"}
, { XBMCK_F14,                    0,    0, XBMCVK_F14,           "f14"}
, { XBMCK_F15,                    0,    0, XBMCVK_F15,           "f15"}

// Misc non-printing keys
, { XBMCK_UP,                     0,    0, XBMCVK_UP,            "up" }
, { XBMCK_DOWN,                   0,    0, XBMCVK_DOWN,          "down" }
, { XBMCK_RIGHT,                  0,    0, XBMCVK_RIGHT,         "right" }
, { XBMCK_LEFT,                   0,    0, XBMCVK_LEFT,          "left" }
, { XBMCK_INSERT,                 0,    0, XBMCVK_INSERT,        "insert" }
, { XBMCK_DELETE,                 0,    0, XBMCVK_DELETE,        "delete" }
, { XBMCK_HOME,                   0,    0, XBMCVK_HOME,          "home" }
, { XBMCK_END,                    0,    0, XBMCVK_END,           "end" }
, { XBMCK_PAGEUP,                 0,    0, XBMCVK_PAGEUP,        "pageup" }
, { XBMCK_PAGEDOWN,               0,    0, XBMCVK_PAGEDOWN,      "pagedown" }
, { XBMCK_NUMLOCK,                0,    0, XBMCVK_NUMLOCK,       "numlock" }
, { XBMCK_CAPSLOCK,               0,    0, XBMCVK_CAPSLOCK,      "capslock" }
, { XBMCK_RSHIFT,                 0,    0, XBMCVK_RSHIFT,        "rightshift" }
, { XBMCK_LSHIFT,                 0,    0, XBMCVK_LSHIFT,        "leftshift" }
, { XBMCK_RCTRL,                  0,    0, XBMCVK_RCONTROL,      "rightctrl" }
, { XBMCK_LCTRL,                  0,    0, XBMCVK_LCONTROL,      "leftctrl" }
, { XBMCK_LALT,                   0,    0, XBMCVK_LMENU,         "leftalt" }
, { XBMCK_LSUPER,                 0,    0, XBMCVK_LWIN,          "leftwindows" }
, { XBMCK_RSUPER,                 0,    0, XBMCVK_RWIN,          "rightwindows" }
, { XBMCK_MENU,                   0,    0, XBMCVK_MENU,          "menu" }
, { XBMCK_PAUSE,                  0,    0, XBMCVK_PAUSE,         "pause" }
, { XBMCK_SCROLLOCK,              0,    0, XBMCVK_SCROLLLOCK,    "scrolllock" }
, { XBMCK_PRINT,                  0,    0, XBMCVK_PRINTSCREEN,   "printscreen" }
, { XBMCK_POWER,                  0,    0, XBMCVK_POWER,         "power" }
, { XBMCK_SLEEP,                  0,    0, XBMCVK_SLEEP,         "sleep" }
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
