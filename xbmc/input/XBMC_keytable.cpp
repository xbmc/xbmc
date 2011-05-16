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
#include "input/XBMC_keytable.h"

// The array of XBMCKEYTABLEs used in XBMC.
// The entries are in ascending order of sym; just for convenience.
// scancode, sym, unicode, ascii, vkey, keyname
static const XBMCKEYTABLE XBMCKeyTable[] =
{ { 0x0E, 0x0008,      0,    0, 0x08, "backspace" }
, { 0x0f, 0x0009,      0,    0, 0x09, "tab" }
, { 0x1c, 0x000d,      0,    0, 0x0d, "return" }
, { 0x0b, 0x001b,      0,    0, 0x1b, "escape" }
, { 0x0b,      0,      0,    0, 0x1b, "esc" } // Allowed abbreviation for "escape"
, { 0x45, 0x0013,      0,    0, 0x13, "pause" }
, { 0x39, 0x0020, 0x0020, 0x20, 0x20, "space" }

// Number keys on the main keyboard
, { 0x0b, 0x0030,    '0',  '0', 0x30, "zero" }
, { 0x02, 0x0031,    '1',  '1', 0x31, "one" }
, { 0x03, 0x0032,    '2',  '2', 0x32, "two" }
, { 0x04, 0x0033,    '3',  '3', 0x33, "three" }
, { 0x05, 0x0034,    '4',  '4', 0x34, "four" }
, { 0x06, 0x0035,    '5',  '5', 0x35, "five" }
, { 0x07, 0x0036,    '6',  '6', 0x36, "six" }
, { 0x08, 0x0037,    '7',  '7', 0x37, "seven" }
, { 0x09, 0x0038,    '8',  '8', 0x38, "eight" }
, { 0x0a, 0x0039,    '9',  '9', 0x39, "nine" }

// Shifted number keys on a US keyboard
, { 0x0b, 0x0030,    ')',  ')', 0x30, "zero" }
, { 0x02, 0x0031,    '!',  '!', 0x31, "one" }
, { 0x03, 0x0032,    '@',  '@', 0x32, "two" }
, { 0x04, 0x0033,    '#',  '#', 0x33, "three" }
, { 0x05, 0x0034,    '$',  '$', 0x34, "four" }
, { 0x06, 0x0035,    '%',  '%', 0x35, "five" }
, { 0x07, 0x0036,    '^',  '^', 0x36, "six" }
, { 0x08, 0x0037,    '&',  '&', 0x37, "seven" }
, { 0x09, 0x0038,    '*',  '*', 0x38, "eight" }
, { 0x0a, 0x0039,    '(',  '(', 0x39, "nine" }

// A to Z - note that upper case A-Z don't have a matching name or
// vkey. Only the lower case a-z are used in key mappings.
, { 0x1e, 0x0061,    'A',  'A', 0x41, NULL }
, { 0x30, 0x0062,    'B',  'B', 0x42, NULL }
, { 0x2e, 0x0063,    'C',  'C', 0x43, NULL }
, { 0x20, 0x0064,    'D',  'D', 0x44, NULL }
, { 0x12, 0x0065,    'E',  'E', 0x45, NULL }
, { 0x21, 0x0066,    'F',  'F', 0x46, NULL }
, { 0x22, 0x0067,    'G',  'G', 0x47, NULL }
, { 0x23, 0x0068,    'H',  'H', 0x48, NULL }
, { 0x17, 0x0069,    'I',  'I', 0x49, NULL }
, { 0x24, 0x006A,    'J',  'J', 0x4A, NULL }
, { 0x25, 0x006B,    'K',  'K', 0x4B, NULL }
, { 0x26, 0x006C,    'L',  'L', 0x4C, NULL }
, { 0x32, 0x006D,    'M',  'M', 0x4D, NULL }
, { 0x31, 0x006E,    'N',  'N', 0x4E, NULL }
, { 0x18, 0x006F,    'O',  'O', 0x4F, NULL }
, { 0x19, 0x0070,    'P',  'P', 0x50, NULL }
, { 0x10, 0x0071,    'Q',  'Q', 0x51, NULL }
, { 0x13, 0x0072,    'R',  'R', 0x52, NULL }
, { 0x1f, 0x0073,    'S',  'S', 0x53, NULL }
, { 0x14, 0x0074,    'T',  'T', 0x54, NULL }
, { 0x16, 0x0075,    'U',  'U', 0x55, NULL }
, { 0x2f, 0x0076,    'V',  'V', 0x56, NULL }
, { 0x11, 0x0077,    'W',  'W', 0x57, NULL }
, { 0x2d, 0x0078,    'X',  'X', 0x58, NULL }
, { 0x15, 0x0079,    'Y',  'Y', 0x59, NULL }
, { 0x2c, 0x007A,    'Z',  'Z', 0x5A, NULL }

, { 0x1e, 0x0061,    'a',  'a', 0x41, "a" }
, { 0x30, 0x0062,    'b',  'b', 0x42, "b" }
, { 0x2e, 0x0063,    'c',  'c', 0x43, "c" }
, { 0x20, 0x0064,    'd',  'd', 0x44, "d" }
, { 0x12, 0x0065,    'e',  'e', 0x45, "e" }
, { 0x21, 0x0066,    'f',  'f', 0x46, "f" }
, { 0x22, 0x0067,    'g',  'g', 0x47, "g" }
, { 0x23, 0x0068,    'h',  'h', 0x48, "h" }
, { 0x17, 0x0069,    'i',  'i', 0x49, "i" }
, { 0x24, 0x006a,    'j',  'j', 0x4a, "j" }
, { 0x25, 0x006b,    'k',  'k', 0x4b, "k" }
, { 0x26, 0x006c,    'l',  'l', 0x4c, "l" }
, { 0x32, 0x006d,    'm',  'm', 0x4d, "m" }
, { 0x31, 0x006e,    'n',  'n', 0x4e, "n" }
, { 0x18, 0x006f,    'o',  'o', 0x4f, "o" }
, { 0x19, 0x0070,    'p',  'p', 0x50, "p" }
, { 0x10, 0x0071,    'q',  'q', 0x51, "q" }
, { 0x13, 0x0072,    'r',  'r', 0x52, "r" }
, { 0x1f, 0x0073,    's',  's', 0x53, "s" }
, { 0x14, 0x0074,    't',  't', 0x54, "t" }
, { 0x16, 0x0075,    'u',  'u', 0x55, "u" }
, { 0x2f, 0x0076,    'v',  'v', 0x56, "v" }
, { 0x11, 0x0077,    'w',  'w', 0x57, "w" }
, { 0x2d, 0x0078,    'x',  'x', 0x58, "x" }
, { 0x15, 0x0079,    'y',  'y', 0x59, "y" }
, { 0x2c, 0x007a,    'z',  'z', 0x5a, "z" }

// Numeric keypad
, { 0x52, 0x0100,    '0',  '0', 0x60, "numpadzero"}
, { 0x4f, 0x0101,    '1',  '1', 0x61, "numpadone"}
, { 0x50, 0x0102,    '2',  '2', 0x62, "numpadtwo"}
, { 0x51, 0x0103,    '3',  '3', 0x63, "numpadthree"}
, { 0x4b, 0x0104,    '4',  '4', 0x64, "numpadfour"}
, { 0x4c, 0x0105,    '5',  '5', 0x65, "numpadfive"}
, { 0x4d, 0x0106,    '6',  '6', 0x66, "numpadsix"}
, { 0x47, 0x0107,    '7',  '7', 0x67, "numpadseven"}
, { 0x48, 0x0108,    '8',  '8', 0x68, "numpadeight"}
, { 0x49, 0x0109,    '9',  '9', 0x69, "numpadnine"}
, { 0x53, 0x010a,    '.',  '.', 0x6e, "numpadperiod"}
, { 0x35, 0x010b,    '/',  '/', 0x6f, "numpaddivide"}
, { 0x37, 0x010c,    '*',  '*', 0x6a, "numpadtimes"}
, { 0x4a, 0x010d,    '-',  '-', 0x6d, "numpadminus"}
, { 0x4e, 0x010e,    '+',  '+', 0x6b, "numpadplus"}
, { 0x1c, 0x010f,      0,    0, 0x6c, "enter"}

// Misc printing characters
, { 0x28, 0x0027,   '\'', '\'', 0xEE, "quote" }
, { 0x28, 0x0027,    '"',  '"', 0xEE, "doublequote" }
, { 0x33, 0x002c,    ',',  ',', 0xBC, "comma" }
, { 0x33, 0x002c,    '<',  '<', 0xBC, "lessthan" }
, { 0x0c, 0x002d,    '-',  '-', 0xBD, "minus" }
, { 0x0c, 0x002d,    '_',  '_', 0xBD, "underline" }
, { 0x34, 0x002e,    '.',  '.', 0xBE, "period" }
, { 0x34, 0x002e,    '>',  '>', 0xBE, "greaterthan" }
, { 0x35, 0x002f,    '/',  '/', 0xBF, "forwardslash" }
, { 0x35, 0x002f,    '?',  '?', 0xBF, "questionmark" }
, { 0x27, 0x003b,    ';',  ';', 0xBA, "semicolon" }
, { 0x27, 0x003b,    ':',  ':', 0xBA, "colon" }
, { 0x0d, 0x003d,    '=',  '=', 0xBB, "equals" }
, { 0x0d, 0x003d,    '+',  '+', 0xBB, "plus" }
, { 0x56, 0x005c,   '\\', '\\', 0xEC, "backslash" }
, { 0x56, 0x005c,    '|',  '|', 0xEC, "pipe" }
, { 0x1a, 0x005b,    '[',  '[', 0xEB, "opensquarebracket" }
, { 0x1a, 0x005b,    '{',  '{', 0xEB, "openbrace" }
, { 0x1b, 0x005d,    ']',  ']', 0xED, "closesquarebracket" }
, { 0x1b, 0x005d,    '}',  '}', 0xED, "closebrace" }
, { 0x29, 0x0060,    '`',  '`', 0xC0, "leftquote" }
, { 0x29, 0x0060,    '~',  '~', 0xC0, "tilde" }

// Multimedia keys
// The scan codes depend on the OS so the codes in this section of the
// table aren't used.
, { 0x00, 0x00A6,      0,    0, 0xA6, "browser_back" }
, { 0x00, 0x00A7,      0,    0, 0xA7, "browser_forward" }
, { 0x00, 0x00A8,      0,    0, 0xA8, "browser_refresh" }
, { 0x00, 0x00A9,      0,    0, 0xA9, "browser_stop" }
, { 0x00, 0x00AA,      0,    0, 0xAA, "browser_search" }
, { 0x00, 0x00AB,      0,    0, 0xAB, "browser_favorites" }
, { 0x00, 0x00AC,      0,    0, 0xAC, "browser_home" }
, { 0x00, 0x00AD,      0,    0, 0xAD, "volume_mute" }
, { 0x00, 0x00AE,      0,    0, 0xAE, "volume_down" }
, { 0x00, 0x00AF,      0,    0, 0xAF, "volume_up" }
, { 0x00, 0x00B0,      0,    0, 0xB0, "next_track" }
, { 0x00, 0x00B1,      0,    0, 0xB1, "prev_track" }
, { 0x00, 0x00B2,      0,    0, 0xB2, "stop" }
, { 0x00, 0x00B3,      0,    0, 0xB3, "play_pause" }
, { 0x00, 0x00B4,      0,    0, 0xB4, "launch_mail" }
, { 0x00, 0x00B5,      0,    0, 0xB5, "launch_media_select" }
, { 0x00, 0x00B6,      0,    0, 0xB6, "launch_app1_pc_icon" }
, { 0x00, 0x00B7,      0,    0, 0xB7, "launch_app2_pc_icon" }
, { 0x00, 0x00B8,      0,    0, 0xB8, "launch_file_browser" }
, { 0x00, 0x00B9,      0,    0, 0xB9, "launch_media_center" }

// Function keys
, { 0x3b, 0x011a,      0,    0, 0x70, "f1"}
, { 0x3c, 0x011b,      0,    0, 0x71, "f2"}
, { 0x3d, 0x011c,      0,    0, 0x72, "f3"}
, { 0x3e, 0x011d,      0,    0, 0x73, "f4"}
, { 0x3f, 0x011e,      0,    0, 0x74, "f5"}
, { 0x40, 0x011f,      0,    0, 0x75, "f6"}
, { 0x41, 0x0120,      0,    0, 0x76, "f7"}
, { 0x42, 0x0121,      0,    0, 0x77, "f8"}
, { 0x43, 0x0122,      0,    0, 0x78, "f9"}
, { 0x44, 0x0123,      0,    0, 0x79, "f10"}
, { 0x57, 0x0124,      0,    0, 0x7a, "f11"}
, { 0x58, 0x0125,      0,    0, 0x7b, "f12"}
, { 0x5b, 0x0126,      0,    0, 0x7c, "f13"}
, { 0x5c, 0x0127,      0,    0, 0x7d, "f14"}
, { 0x5d, 0x0128,      0,    0, 0x7e, "f15"}

// Misc non-printing keys
, { 0x48, 0x0111,      0,    0, 0x26, "up" }
, { 0x50, 0x0112,      0,    0, 0x28, "down" }
, { 0x4d, 0x0113,      0,    0, 0x27, "right" }
, { 0x4b, 0x0114,      0,    0, 0x25, "left" }
, { 0x52, 0x0115,      0,    0, 0x2d, "insert" }
, { 0x53, 0x007f,      0,    0, 0x2e, "delete" }
, { 0x47, 0x0116,      0,    0, 0x24, "home" }
, { 0x4f, 0x0117,      0,    0, 0x23, "end" }
, { 0x49, 0x0118,      0,    0, 0x21, "pageup" }
, { 0x51, 0x0119,      0,    0, 0x22, "pagedown" }
, { 0x45, 0x012c,      0,    0, 0x90, "numlock" }
, { 0x3a, 0x012d,      0,    0, 0x20, "capslock" }
, { 0x46, 0x012e,      0,    0, 0x91, "scrolllock" }
, { 0x36, 0x012f,      0,    0, 0xA1, "rightshift" }
, { 0x2a, 0x0130,      0,    0, 0xA0, "leftshift" }
, { 0x1d, 0x0131,      0,    0, 0xA3, "rightctrl" }
, { 0x1d, 0x0132,      0,    0, 0xA2, "leftctrl" }
, { 0x38, 0x0134,      0,    0, 0xA4, "leftalt" }
, { 0x5b, 0x0137,      0,    0, 0x5B, "leftwindows" }
, { 0x5c, 0x0138,      0,    0, 0x5C, "rightwindows" }
, { 0x37, 0x013c,      0,    0, 0x2A, "printscreen" }
, { 0x5d, 0x013f,      0,    0, 0x5D, "menu" }
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
