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

// C++ Implementation: CKeyboard

// Comment OUT, if not really debugging!!!:
//#define DEBUG_KEYBOARD_GETCHAR

#include "KeyboardStat.h"
#include "KeyboardLayoutConfiguration.h"
#include "XBMC_events.h"
#include "utils/TimeUtils.h"

#if defined(_LINUX) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

CKeyboardStat g_Keyboard;

#define XBMC_NLK_CAPS 0x01
#define XBMC_NLK_NUM  0x02

/* Global keystate information */
static uint8_t  XBMC_KeyState[XBMCK_LAST];
static XBMCMod XBMC_ModState;
static const char *keynames[XBMCK_LAST];	/* Array of keycode names */

static uint8_t XBMC_NoLockKeys;

struct _XBMC_KeyRepeat {
  int firsttime;    /* if we check against the delay or repeat value */
  int delay;        /* the delay before we start repeating */
  int interval;     /* the delay between key repeat events */
  uint32_t timestamp; /* the time the first keydown event occurred */

  XBMC_Event evt;    /* the event we are supposed to repeat */
} XBMC_KeyRepeat;

int XBMC_EnableKeyRepeat(int delay, int interval)
{
  if ( (delay < 0) || (interval < 0) ) {
    return(-1);
  }
  XBMC_KeyRepeat.firsttime = 0;
  XBMC_KeyRepeat.delay = delay;
  XBMC_KeyRepeat.interval = interval;
  XBMC_KeyRepeat.timestamp = 0;
  return(0);
}

struct XBMC_KeyMapping
{
  int   source;
  BYTE  VKey;
  char  Ascii;
  WCHAR Unicode;
};

// Convert control keypresses e.g. ctrl-A from 0x01 to 0x41
static XBMC_KeyMapping g_mapping_ctrlkeys[] =
{ {0x30, 0x60, XBMCK_0, XBMCK_0}
, {0x31, 0x61, XBMCK_1, XBMCK_1}
, {0x32, 0x62, XBMCK_2, XBMCK_2}
, {0x33, 0x63, XBMCK_3, XBMCK_3}
, {0x34, 0x64, XBMCK_4, XBMCK_4}
, {0x35, 0x65, XBMCK_5, XBMCK_5}
, {0x36, 0x66, XBMCK_6, XBMCK_6}
, {0x37, 0x67, XBMCK_7, XBMCK_7}
, {0x38, 0x68, XBMCK_8, XBMCK_8}
, {0x39, 0x69, XBMCK_9, XBMCK_9}
, {0x61, 0x41, XBMCK_a, XBMCK_a}
, {0x62, 0x42, XBMCK_b, XBMCK_b}
, {0x63, 0x43, XBMCK_c, XBMCK_c}
, {0x64, 0x44, XBMCK_d, XBMCK_d}
, {0x65, 0x45, XBMCK_e, XBMCK_e}
, {0x66, 0x46, XBMCK_f, XBMCK_f}
, {0x67, 0x47, XBMCK_g, XBMCK_g}
, {0x68, 0x48, XBMCK_h, XBMCK_h}
, {0x69, 0x49, XBMCK_i, XBMCK_i}
, {0x6a, 0x4a, XBMCK_j, XBMCK_j}
, {0x6b, 0x4b, XBMCK_k, XBMCK_k}
, {0x6c, 0x4c, XBMCK_l, XBMCK_l}
, {0x6d, 0x4d, XBMCK_m, XBMCK_m}
, {0x6e, 0x4e, XBMCK_n, XBMCK_n}
, {0x6f, 0x4f, XBMCK_o, XBMCK_o}
, {0x70, 0x50, XBMCK_p, XBMCK_p}
, {0x71, 0x51, XBMCK_q, XBMCK_q}
, {0x72, 0x52, XBMCK_r, XBMCK_r}
, {0x73, 0x53, XBMCK_s, XBMCK_s}
, {0x74, 0x54, XBMCK_t, XBMCK_t}
, {0x75, 0x55, XBMCK_u, XBMCK_u}
, {0x76, 0x56, XBMCK_v, XBMCK_v}
, {0x77, 0x57, XBMCK_w, XBMCK_w}
, {0x78, 0x58, XBMCK_x, XBMCK_x}
, {0x79, 0x59, XBMCK_y, XBMCK_y}
, {0x7a, 0x5a, XBMCK_z, XBMCK_z}
};

// based on the evdev mapped scancodes in /user/share/X11/xkb/keycodes
static XBMC_KeyMapping g_mapping_evdev[] =
{ { 121, 0xad } // Volume mute
, { 122, 0xae } // Volume down
, { 123, 0xaf } // Volume up
, { 127, 0x20 } // Pause
, { 135, 0x5d } // Right click
, { 136, 0xb2 } // Stop
, { 138, 0x49 } // Info
, { 147, 0x4d } // Menu
, { 150, 0x9f } // Sleep
, { 152, 0xb8 } // Launch file browser
, { 163, 0xb4 } // Launch Mail
, { 164, 0xab } // Browser favorites
, { 166, 0x08 } // Back
, { 167, 0xa7 } // Browser forward
, { 171, 0xb0 } // Next track
, { 172, 0xb3 } // Play_Pause
, { 173, 0xb1 } // Prev track
, { 174, 0xb2 } // Stop
, { 176, 0x52 } // Rewind
, { 179, 0xb9 } // Launch media center
, { 180, 0xac } // Browser home
, { 181, 0xa8 } // Browser refresh
, { 214, 0x1B } // Close
, { 215, 0xb3 } // Play_Pause
, { 216, 0x46 } // Forward
//, {167, 0xb3 } // Record
};

// following scancode infos are
// 1. from ubuntu keyboard shortcut (hex) -> predefined
// 2. from unix tool xev and my keyboards (decimal)
// m_VKey infos from CharProbe tool
// Can we do the same for XBoxKeyboard and DirectInputKeyboard? Can we access the scancode of them? By the way how does SDL do it? I can't find it. (Automagically? But how exactly?)
// Some pairs of scancode and virtual keys are only known half

// special "keys" above F1 till F12 on my MS natural keyboard mapped to virtual keys "F13" till "F24"):
static XBMC_KeyMapping g_mapping_ubuntu[] =
{ { 0xf5, 0x7c } // F13 Launch help browser
, { 0x87, 0x7d } // F14 undo
, { 0x8a, 0x7e } // F15 redo
, { 0x89, 0x7f } // F16 new
, { 0xbf, 0x80 } // F17 open
, { 0xaf, 0x81 } // F18 close
, { 0xe4, 0x82 } // F19 reply
, { 0x8e, 0x83 } // F20 forward
, { 0xda, 0x84 } // F21 send
//, { 0x, 0x85 } // F22 check spell (doesn't work for me with ubuntu)
, { 0xd5, 0x86 } // F23 save
, { 0xb9, 0x87 } // 0x2a?? F24 print
        // end of special keys above F1 till F12

, { 234, 0xa6 } // Browser back
, { 233, 0xa7 } // Browser forward
, { 231, 0xa8 } // Browser refresh
//, { , 0xa9 } // Browser stop
, { 122, 0xaa } // Browser search
, { 0xe5, 0xaa } // Browser search
, { 230, 0xab } // Browser favorites
, { 130, 0xac } // Browser home
, { 0xa0, 0xad } // Volume mute
, { 0xae, 0xae } // Volume down
, { 0xb0, 0xaf } // Volume up
, { 0x99, 0xb0 } // Next track
, { 0x90, 0xb1 } // Prev track
, { 0xa4, 0xb2 } // Stop
, { 0xa2, 0xb3 } // Play_Pause
, { 0xec, 0xb4 } // Launch mail
, { 129, 0xb5 } // Launch media_select
, { 198, 0xb6 } // Launch App1/PC icon
, { 0xa1, 0xb7 } // Launch App2/Calculator
, { 34, 0xba } // OEM 1: [ on us keyboard
, { 51, 0xbf } // OEM 2: additional key on european keyboards between enter and ' on us keyboards
, { 47, 0xc0 } // OEM 3: ; on us keyboards
, { 20, 0xdb } // OEM 4: - on us keyboards (between 0 and =)
, { 49, 0xdc } // OEM 5: ` on us keyboards (below ESC)
, { 21, 0xdd } // OEM 6: =??? on us keyboards (between - and backspace)
, { 48, 0xde } // OEM 7: ' on us keyboards (on right side of ;)
//, { 0, 0xdf } // OEM 8
, { 94, 0xe2 } // OEM 102: additional key on european keyboards between left shift and z on us keyboards
//, { 0xb2, 0x } // Ubuntu default setting for launch browser
//, { 0x76, 0x } // Ubuntu default setting for launch music player
//, { 0xcc, 0x } // Ubuntu default setting for eject
, { 117, 0x5d } // right click
};

// OSX defines unicode values for non-printing keys which breaks the key parser, set m_wUnicode
static XBMC_KeyMapping g_mapping_npc[] =
{ { XBMCK_BACKSPACE, 0x08 }
, { XBMCK_TAB, 0x09 }
, { XBMCK_RETURN, 0x0d }
, { XBMCK_ESCAPE, 0x1b }
, { XBMCK_SPACE, 0x20, ' ', L' ' }
, { XBMCK_MENU, 0x5d }
, { XBMCK_KP0, 0x60 }
, { XBMCK_KP1, 0x61 }
, { XBMCK_KP2, 0x62 }
, { XBMCK_KP3, 0x63 }
, { XBMCK_KP4, 0x64 }
, { XBMCK_KP5, 0x65 }
, { XBMCK_KP6, 0x66 }
, { XBMCK_KP7, 0x67 }
, { XBMCK_KP8, 0x68 }
, { XBMCK_KP9, 0x69 }
, { XBMCK_KP_ENTER, 0x6C }
, { XBMCK_UP, 0x26 }
, { XBMCK_DOWN, 0x28 }
, { XBMCK_LEFT, 0x25 }
, { XBMCK_RIGHT, 0x27 }
, { XBMCK_INSERT, 0x2D }
, { XBMCK_DELETE, 0x2E }
, { XBMCK_HOME, 0x24 }
, { XBMCK_END, 0x23 }
, { XBMCK_F1, 0x70 }
, { XBMCK_F2, 0x71 }
, { XBMCK_F3, 0x72 }
, { XBMCK_F4, 0x73 }
, { XBMCK_F5, 0x74 }
, { XBMCK_F6, 0x75 }
, { XBMCK_F7, 0x76 }
, { XBMCK_F8, 0x77 }
, { XBMCK_F9, 0x78 }
, { XBMCK_F10, 0x79 }
, { XBMCK_F11, 0x7a }
, { XBMCK_F12, 0x7b }
, { XBMCK_KP_PERIOD, 0x6e }
, { XBMCK_KP_MULTIPLY, 0x6a }
, { XBMCK_KP_MINUS, 0x6d }
, { XBMCK_KP_PLUS, 0x6b }
, { XBMCK_KP_DIVIDE, 0x6f }
, { XBMCK_PAGEUP, 0x21 }
, { XBMCK_PAGEDOWN, 0x22 }
, { XBMCK_PRINT, 0x2a }
, { XBMCK_LSHIFT, 0xa0 }
, { XBMCK_RSHIFT, 0xa1 }
};

static bool LookupKeyMapping(BYTE* VKey, char* Ascii, WCHAR* Unicode, int source, XBMC_KeyMapping* map, int count)
{
  for(int i = 0; i < count; i++)
  {
    if(source == map[i].source)
    {
      if(VKey)
        *VKey    = map[i].VKey;
      if(Ascii)
        *Ascii   = map[i].Ascii;
      if(Unicode)
        *Unicode = map[i].Unicode;
      return true;
    }
  }
  return false;
}

CKeyboardStat::CKeyboardStat()
{
  /* Initialize the tables */
  XBMC_ModState = XBMCKMOD_NONE;
  memset((void*)keynames, 0, sizeof(keynames));
  memset(XBMC_KeyState, 0, sizeof(XBMC_KeyState));

  XBMC_EnableKeyRepeat(0, 0);

  XBMC_NoLockKeys = 0;

  /* Fill in the blanks in keynames */
  keynames[XBMCK_BACKSPACE] = "backspace";
  keynames[XBMCK_TAB] = "tab";
  keynames[XBMCK_CLEAR] = "clear";
  keynames[XBMCK_RETURN] = "return";
  keynames[XBMCK_PAUSE] = "pause";
  keynames[XBMCK_ESCAPE] = "escape";
  keynames[XBMCK_SPACE] = "space";
  keynames[XBMCK_EXCLAIM]  = "!";
  keynames[XBMCK_QUOTEDBL]  = "\"";
  keynames[XBMCK_HASH]  = "#";
  keynames[XBMCK_DOLLAR]  = "$";
  keynames[XBMCK_AMPERSAND]  = "&";
  keynames[XBMCK_QUOTE] = "'";
  keynames[XBMCK_LEFTPAREN] = "(";
  keynames[XBMCK_RIGHTPAREN] = ")";
  keynames[XBMCK_ASTERISK] = "*";
  keynames[XBMCK_PLUS] = "+";
  keynames[XBMCK_COMMA] = ",";
  keynames[XBMCK_MINUS] = "-";
  keynames[XBMCK_PERIOD] = ".";
  keynames[XBMCK_SLASH] = "/";
  keynames[XBMCK_0] = "0";
  keynames[XBMCK_1] = "1";
  keynames[XBMCK_2] = "2";
  keynames[XBMCK_3] = "3";
  keynames[XBMCK_4] = "4";
  keynames[XBMCK_5] = "5";
  keynames[XBMCK_6] = "6";
  keynames[XBMCK_7] = "7";
  keynames[XBMCK_8] = "8";
  keynames[XBMCK_9] = "9";
  keynames[XBMCK_COLON] = ":";
  keynames[XBMCK_SEMICOLON] = ";";
  keynames[XBMCK_LESS] = "<";
  keynames[XBMCK_EQUALS] = "=";
  keynames[XBMCK_GREATER] = ">";
  keynames[XBMCK_QUESTION] = "?";
  keynames[XBMCK_AT] = "@";
  keynames[XBMCK_LEFTBRACKET] = "[";
  keynames[XBMCK_BACKSLASH] = "\\";
  keynames[XBMCK_RIGHTBRACKET] = "]";
  keynames[XBMCK_CARET] = "^";
  keynames[XBMCK_UNDERSCORE] = "_";
  keynames[XBMCK_BACKQUOTE] = "`";
  keynames[XBMCK_a] = "a";
  keynames[XBMCK_b] = "b";
  keynames[XBMCK_c] = "c";
  keynames[XBMCK_d] = "d";
  keynames[XBMCK_e] = "e";
  keynames[XBMCK_f] = "f";
  keynames[XBMCK_g] = "g";
  keynames[XBMCK_h] = "h";
  keynames[XBMCK_i] = "i";
  keynames[XBMCK_j] = "j";
  keynames[XBMCK_k] = "k";
  keynames[XBMCK_l] = "l";
  keynames[XBMCK_m] = "m";
  keynames[XBMCK_n] = "n";
  keynames[XBMCK_o] = "o";
  keynames[XBMCK_p] = "p";
  keynames[XBMCK_q] = "q";
  keynames[XBMCK_r] = "r";
  keynames[XBMCK_s] = "s";
  keynames[XBMCK_t] = "t";
  keynames[XBMCK_u] = "u";
  keynames[XBMCK_v] = "v";
  keynames[XBMCK_w] = "w";
  keynames[XBMCK_x] = "x";
  keynames[XBMCK_y] = "y";
  keynames[XBMCK_z] = "z";
  keynames[XBMCK_DELETE] = "delete";

  keynames[XBMCK_WORLD_0] = "world 0";
  keynames[XBMCK_WORLD_1] = "world 1";
  keynames[XBMCK_WORLD_2] = "world 2";
  keynames[XBMCK_WORLD_3] = "world 3";
  keynames[XBMCK_WORLD_4] = "world 4";
  keynames[XBMCK_WORLD_5] = "world 5";
  keynames[XBMCK_WORLD_6] = "world 6";
  keynames[XBMCK_WORLD_7] = "world 7";
  keynames[XBMCK_WORLD_8] = "world 8";
  keynames[XBMCK_WORLD_9] = "world 9";
  keynames[XBMCK_WORLD_10] = "world 10";
  keynames[XBMCK_WORLD_11] = "world 11";
  keynames[XBMCK_WORLD_12] = "world 12";
  keynames[XBMCK_WORLD_13] = "world 13";
  keynames[XBMCK_WORLD_14] = "world 14";
  keynames[XBMCK_WORLD_15] = "world 15";
  keynames[XBMCK_WORLD_16] = "world 16";
  keynames[XBMCK_WORLD_17] = "world 17";
  keynames[XBMCK_WORLD_18] = "world 18";
  keynames[XBMCK_WORLD_19] = "world 19";
  keynames[XBMCK_WORLD_20] = "world 20";
  keynames[XBMCK_WORLD_21] = "world 21";
  keynames[XBMCK_WORLD_22] = "world 22";
  keynames[XBMCK_WORLD_23] = "world 23";
  keynames[XBMCK_WORLD_24] = "world 24";
  keynames[XBMCK_WORLD_25] = "world 25";
  keynames[XBMCK_WORLD_26] = "world 26";
  keynames[XBMCK_WORLD_27] = "world 27";
  keynames[XBMCK_WORLD_28] = "world 28";
  keynames[XBMCK_WORLD_29] = "world 29";
  keynames[XBMCK_WORLD_30] = "world 30";
  keynames[XBMCK_WORLD_31] = "world 31";
  keynames[XBMCK_WORLD_32] = "world 32";
  keynames[XBMCK_WORLD_33] = "world 33";
  keynames[XBMCK_WORLD_34] = "world 34";
  keynames[XBMCK_WORLD_35] = "world 35";
  keynames[XBMCK_WORLD_36] = "world 36";
  keynames[XBMCK_WORLD_37] = "world 37";
  keynames[XBMCK_WORLD_38] = "world 38";
  keynames[XBMCK_WORLD_39] = "world 39";
  keynames[XBMCK_WORLD_40] = "world 40";
  keynames[XBMCK_WORLD_41] = "world 41";
  keynames[XBMCK_WORLD_42] = "world 42";
  keynames[XBMCK_WORLD_43] = "world 43";
  keynames[XBMCK_WORLD_44] = "world 44";
  keynames[XBMCK_WORLD_45] = "world 45";
  keynames[XBMCK_WORLD_46] = "world 46";
  keynames[XBMCK_WORLD_47] = "world 47";
  keynames[XBMCK_WORLD_48] = "world 48";
  keynames[XBMCK_WORLD_49] = "world 49";
  keynames[XBMCK_WORLD_50] = "world 50";
  keynames[XBMCK_WORLD_51] = "world 51";
  keynames[XBMCK_WORLD_52] = "world 52";
  keynames[XBMCK_WORLD_53] = "world 53";
  keynames[XBMCK_WORLD_54] = "world 54";
  keynames[XBMCK_WORLD_55] = "world 55";
  keynames[XBMCK_WORLD_56] = "world 56";
  keynames[XBMCK_WORLD_57] = "world 57";
  keynames[XBMCK_WORLD_58] = "world 58";
  keynames[XBMCK_WORLD_59] = "world 59";
  keynames[XBMCK_WORLD_60] = "world 60";
  keynames[XBMCK_WORLD_61] = "world 61";
  keynames[XBMCK_WORLD_62] = "world 62";
  keynames[XBMCK_WORLD_63] = "world 63";
  keynames[XBMCK_WORLD_64] = "world 64";
  keynames[XBMCK_WORLD_65] = "world 65";
  keynames[XBMCK_WORLD_66] = "world 66";
  keynames[XBMCK_WORLD_67] = "world 67";
  keynames[XBMCK_WORLD_68] = "world 68";
  keynames[XBMCK_WORLD_69] = "world 69";
  keynames[XBMCK_WORLD_70] = "world 70";
  keynames[XBMCK_WORLD_71] = "world 71";
  keynames[XBMCK_WORLD_72] = "world 72";
  keynames[XBMCK_WORLD_73] = "world 73";
  keynames[XBMCK_WORLD_74] = "world 74";
  keynames[XBMCK_WORLD_75] = "world 75";
  keynames[XBMCK_WORLD_76] = "world 76";
  keynames[XBMCK_WORLD_77] = "world 77";
  keynames[XBMCK_WORLD_78] = "world 78";
  keynames[XBMCK_WORLD_79] = "world 79";
  keynames[XBMCK_WORLD_80] = "world 80";
  keynames[XBMCK_WORLD_81] = "world 81";
  keynames[XBMCK_WORLD_82] = "world 82";
  keynames[XBMCK_WORLD_83] = "world 83";
  keynames[XBMCK_WORLD_84] = "world 84";
  keynames[XBMCK_WORLD_85] = "world 85";
  keynames[XBMCK_WORLD_86] = "world 86";
  keynames[XBMCK_WORLD_87] = "world 87";
  keynames[XBMCK_WORLD_88] = "world 88";
  keynames[XBMCK_WORLD_89] = "world 89";
  keynames[XBMCK_WORLD_90] = "world 90";
  keynames[XBMCK_WORLD_91] = "world 91";
  keynames[XBMCK_WORLD_92] = "world 92";
  keynames[XBMCK_WORLD_93] = "world 93";
  keynames[XBMCK_WORLD_94] = "world 94";
  keynames[XBMCK_WORLD_95] = "world 95";

  keynames[XBMCK_KP0] = "[0]";
  keynames[XBMCK_KP1] = "[1]";
  keynames[XBMCK_KP2] = "[2]";
  keynames[XBMCK_KP3] = "[3]";
  keynames[XBMCK_KP4] = "[4]";
  keynames[XBMCK_KP5] = "[5]";
  keynames[XBMCK_KP6] = "[6]";
  keynames[XBMCK_KP7] = "[7]";
  keynames[XBMCK_KP8] = "[8]";
  keynames[XBMCK_KP9] = "[9]";
  keynames[XBMCK_KP_PERIOD] = "[.]";
  keynames[XBMCK_KP_DIVIDE] = "[/]";
  keynames[XBMCK_KP_MULTIPLY] = "[*]";
  keynames[XBMCK_KP_MINUS] = "[-]";
  keynames[XBMCK_KP_PLUS] = "[+]";
  keynames[XBMCK_KP_ENTER] = "enter";
  keynames[XBMCK_KP_EQUALS] = "equals";

  keynames[XBMCK_UP] = "up";
  keynames[XBMCK_DOWN] = "down";
  keynames[XBMCK_RIGHT] = "right";
  keynames[XBMCK_LEFT] = "left";
  keynames[XBMCK_DOWN] = "down";
  keynames[XBMCK_INSERT] = "insert";
  keynames[XBMCK_HOME] = "home";
  keynames[XBMCK_END] = "end";
  keynames[XBMCK_PAGEUP] = "page up";
  keynames[XBMCK_PAGEDOWN] = "page down";

  keynames[XBMCK_F1] = "f1";
  keynames[XBMCK_F2] = "f2";
  keynames[XBMCK_F3] = "f3";
  keynames[XBMCK_F4] = "f4";
  keynames[XBMCK_F5] = "f5";
  keynames[XBMCK_F6] = "f6";
  keynames[XBMCK_F7] = "f7";
  keynames[XBMCK_F8] = "f8";
  keynames[XBMCK_F9] = "f9";
  keynames[XBMCK_F10] = "f10";
  keynames[XBMCK_F11] = "f11";
  keynames[XBMCK_F12] = "f12";
  keynames[XBMCK_F13] = "f13";
  keynames[XBMCK_F14] = "f14";
  keynames[XBMCK_F15] = "f15";

  keynames[XBMCK_NUMLOCK] = "numlock";
  keynames[XBMCK_CAPSLOCK] = "caps lock";
  keynames[XBMCK_SCROLLOCK] = "scroll lock";
  keynames[XBMCK_RSHIFT] = "right shift";
  keynames[XBMCK_LSHIFT] = "left shift";
  keynames[XBMCK_RCTRL] = "right ctrl";
  keynames[XBMCK_LCTRL] = "left ctrl";
  keynames[XBMCK_RALT] = "right alt";
  keynames[XBMCK_LALT] = "left alt";
  keynames[XBMCK_RMETA] = "right meta";
  keynames[XBMCK_LMETA] = "left meta";
  keynames[XBMCK_LSUPER] = "left super";	/* "Windows" keys */
  keynames[XBMCK_RSUPER] = "right super";
  keynames[XBMCK_MODE] = "alt gr";
  keynames[XBMCK_COMPOSE] = "compose";

  keynames[XBMCK_HELP] = "help";
  keynames[XBMCK_PRINT] = "print screen";
  keynames[XBMCK_SYSREQ] = "sys req";
  keynames[XBMCK_BREAK] = "break";
  keynames[XBMCK_MENU] = "menu";
  keynames[XBMCK_POWER] = "power";
  keynames[XBMCK_EURO] = "euro";
  keynames[XBMCK_UNDO] = "undo";

  Reset();
  m_lastKey = XBMCK_UNKNOWN;
  m_lastKeyTime = 0;
  m_keyHoldTime = 0;
  m_bEvdev = true;
}

CKeyboardStat::~CKeyboardStat()
{
}

void CKeyboardStat::Initialize()
{
/* this stuff probably doesn't belong here  *
 * but in some x11 specific WinEvents file  *
 * but for some reason the code to map keys *
 * to specific xbmc vkeys is here           */
#if defined(_LINUX) && !defined(__APPLE__)
  Display* dpy = XOpenDisplay(NULL);
  if (!dpy)
    return;

  XkbDescPtr desc;
  char* symbols;

  desc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
  if(!desc)
  {
    XCloseDisplay(dpy);
    return;
  }

  symbols = XGetAtomName(dpy, desc->names->symbols);
  if(symbols)
  {
    CLog::Log(LOGDEBUG, "CKeyboardStat::Initialize - XKb symbols %s", symbols);
    if(strstr(symbols, "(evdev)"))
      m_bEvdev = true;
    else
      m_bEvdev = false;
  }

  XFree(symbols);
  XkbFreeKeyboard(desc, XkbAllComponentsMask, True);
  XCloseDisplay(dpy);
#endif
}

void CKeyboardStat::Reset()
{
  m_bShift = false;
  m_bCtrl = false;
  m_bAlt = false;
  m_bRAlt = false;
  m_bSuper = false;
  m_cAscii = '\0';
  m_wUnicode = '\0';
  m_VKey = 0;

  ZeroMemory(&XBMC_KeyState, sizeof(XBMC_KeyState));
}

void CKeyboardStat::ResetState()
{
  Reset();
  XBMC_ModState = XBMCKMOD_NONE;
}

unsigned int CKeyboardStat::KeyHeld() const
{
  return m_keyHoldTime;
}

int CKeyboardStat::HandleEvent(XBMC_Event& newEvent)
{
  int repeatable;
  uint16_t modstate;

  /* Set up the keysym */
  XBMC_keysym *keysym = &newEvent.key.keysym;
  modstate = (uint16_t)XBMC_ModState;

  repeatable = 0;

  int state;
  if(newEvent.type == XBMC_KEYDOWN)
    state = XBMC_PRESSED;
  else if(newEvent.type == XBMC_KEYUP)
    state = XBMC_RELEASED;
  else
    return 0;

  if ( state == XBMC_PRESSED )
  {
    keysym->mod = (XBMCMod)modstate;
    switch (keysym->sym)
    {
      case XBMCK_UNKNOWN:
        break;
      case XBMCK_NUMLOCK:
        modstate ^= XBMCKMOD_NUM;
        if ( XBMC_NoLockKeys & XBMC_NLK_NUM )
          break;
        if ( ! (modstate&XBMCKMOD_NUM) )
          state = XBMC_RELEASED;
        keysym->mod = (XBMCMod)modstate;
        break;
      case XBMCK_CAPSLOCK:
        modstate ^= XBMCKMOD_CAPS;
        if ( XBMC_NoLockKeys & XBMC_NLK_CAPS )
          break;
        if ( ! (modstate&XBMCKMOD_CAPS) )
          state = XBMC_RELEASED;
        keysym->mod = (XBMCMod)modstate;
        break;
      case XBMCK_LCTRL:
        modstate |= XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        modstate |= XBMCKMOD_RCTRL;
        break;
      case XBMCK_LSHIFT:
        modstate |= XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        modstate |= XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LALT:
        modstate |= XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        modstate |= XBMCKMOD_RALT;
        break;
      case XBMCK_LMETA:
        modstate |= XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        modstate |= XBMCKMOD_RMETA;
        break;
      case XBMCK_LSUPER:
        modstate |= XBMCKMOD_LSUPER;
        break;
      case XBMCK_RSUPER:
        modstate |= XBMCKMOD_RSUPER;
        break;
      case XBMCK_MODE:
        modstate |= XBMCKMOD_MODE;
        break;
      default:
        repeatable = 1;
        break;
    }
  }
  else
  {
    switch (keysym->sym)
    {
      case XBMCK_UNKNOWN:
        break;
      case XBMCK_NUMLOCK:
        if ( XBMC_NoLockKeys & XBMC_NLK_NUM )
          break;
        /* Only send keydown events */
        return(0);
      case XBMCK_CAPSLOCK:
        if ( XBMC_NoLockKeys & XBMC_NLK_CAPS )
          break;
        /* Only send keydown events */
        return(0);
      case XBMCK_LCTRL:
        modstate &= ~XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        modstate &= ~XBMCKMOD_RCTRL;
        break;
      case XBMCK_LSHIFT:
        modstate &= ~XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        modstate &= ~XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LALT:
        modstate &= ~XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        modstate &= ~XBMCKMOD_RALT;
        break;
      case XBMCK_LMETA:
        modstate &= ~XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        modstate &= ~XBMCKMOD_RMETA;
        break;
      case XBMCK_LSUPER:
        modstate &= ~XBMCKMOD_LSUPER;
        break;
      case XBMCK_RSUPER:
        modstate &= ~XBMCKMOD_RSUPER;
        break;
      case XBMCK_MODE:
        modstate &= ~XBMCKMOD_MODE;
        break;
      default:
        break;
    }
    keysym->mod = (XBMCMod)modstate;
  }

  if(state == XBMC_RELEASED)
    if ( XBMC_KeyRepeat.timestamp &&
      XBMC_KeyRepeat.evt.key.keysym.sym == keysym->sym )
    {
      XBMC_KeyRepeat.timestamp = 0;
    }

  if ( keysym->sym != XBMCK_UNKNOWN )
  {
    /* Update internal keyboard state */
    XBMC_ModState = (XBMCMod)modstate;
    XBMC_KeyState[keysym->sym] = state;
  }

  newEvent.key.state = state;
  Update(newEvent);

  return 0;
}

void CKeyboardStat::Update(XBMC_Event& event)
{
  if (event.type == XBMC_KEYDOWN)
  {
    unsigned int now = CTimeUtils::GetFrameTime();
    if (m_lastKey == event.key.keysym.sym)
      m_keyHoldTime += now - m_lastKeyTime;
    else
      m_keyHoldTime = 0;
    m_lastKey = event.key.keysym.sym;
    m_lastKeyTime = now;

    m_cAscii = 0;
    m_VKey = 0;

    m_wUnicode = event.key.keysym.unicode;

    m_bCtrl = (event.key.keysym.mod & XBMCKMOD_CTRL) != 0;
    m_bShift = (event.key.keysym.mod & XBMCKMOD_SHIFT) != 0;
    m_bAlt = (event.key.keysym.mod & XBMCKMOD_ALT) != 0;
    m_bRAlt = (event.key.keysym.mod & XBMCKMOD_RALT) != 0;
    m_bSuper = (event.key.keysym.mod & XBMCKMOD_SUPER) != 0;

    CLog::Log(LOGDEBUG, "SDLKeyboard: scancode: %d, sym: %d, unicode: %d, modifier: %x", event.key.keysym.scancode, event.key.keysym.sym, event.key.keysym.unicode, event.key.keysym.mod);

    if ((event.key.keysym.unicode >= 'A' && event.key.keysym.unicode <= 'Z') ||
      (event.key.keysym.unicode >= 'a' && event.key.keysym.unicode <= 'z'))
    {
      m_cAscii = (char)event.key.keysym.unicode;
      m_VKey = toupper(m_cAscii);
    }
    else if (event.key.keysym.unicode >= '0' && event.key.keysym.unicode <= '9')
    {
      m_cAscii = (char)event.key.keysym.unicode;
      m_VKey = 0x60 + m_cAscii - '0'; // xbox keyboard routine appears to return 0x60->69 (unverified). Ideally this "fixing"
      // should be done in xbox routine, not in the sdl/directinput routines.
      // we should just be using the unicode/ascii value in all routines (perhaps with some
      // headroom for modifier keys?)
    }
    else
    {
      // see comment above about the weird use of m_VKey here...
      if (event.key.keysym.unicode == ')') { m_VKey = 0x60; m_cAscii = ')'; }
      else if (event.key.keysym.unicode == '!') { m_VKey = 0x61; m_cAscii = '!'; }
      else if (event.key.keysym.unicode == '@') { m_VKey = 0x62; m_cAscii = '@'; }
      else if (event.key.keysym.unicode == '#') { m_VKey = 0x63; m_cAscii = '#'; }
      else if (event.key.keysym.unicode == '$') { m_VKey = 0x64; m_cAscii = '$'; }
      else if (event.key.keysym.unicode == '%') { m_VKey = 0x65; m_cAscii = '%'; }
      else if (event.key.keysym.unicode == '^') { m_VKey = 0x66; m_cAscii = '^'; }
      else if (event.key.keysym.unicode == '&') { m_VKey = 0x67; m_cAscii = '&'; }
      else if (event.key.keysym.unicode == '*') { m_VKey = 0x68; m_cAscii = '*'; }
      else if (event.key.keysym.unicode == '(') { m_VKey = 0x69; m_cAscii = '('; }
      else if (event.key.keysym.unicode == ':') { m_VKey = 0xba; m_cAscii = ':'; }
      else if (event.key.keysym.unicode == ';') { m_VKey = 0xba; m_cAscii = ';'; }
      else if (event.key.keysym.unicode == '=') { m_VKey = 0xbb; m_cAscii = '='; }
      else if (event.key.keysym.unicode == '+') { m_VKey = 0xbb; m_cAscii = '+'; }
      else if (event.key.keysym.unicode == '<') { m_VKey = 0xbc; m_cAscii = '<'; }
      else if (event.key.keysym.unicode == ',') { m_VKey = 0xbc; m_cAscii = ','; }
      else if (event.key.keysym.unicode == '-') { m_VKey = 0xbd; m_cAscii = '-'; }
      else if (event.key.keysym.unicode == '_') { m_VKey = 0xbd; m_cAscii = '_'; }
      else if (event.key.keysym.unicode == '>') { m_VKey = 0xbe; m_cAscii = '>'; }
      else if (event.key.keysym.unicode == '.') { m_VKey = 0xbe; m_cAscii = '.'; }
      else if (event.key.keysym.unicode == '?') { m_VKey = 0xbf; m_cAscii = '?'; } // 0xbf is OEM 2 Why is it assigned here?
      else if (event.key.keysym.unicode == '/') { m_VKey = 0xbf; m_cAscii = '/'; }
      else if (event.key.keysym.unicode == '~') { m_VKey = 0xc0; m_cAscii = '~'; }
      else if (event.key.keysym.unicode == '`') { m_VKey = 0xc0; m_cAscii = '`'; }
      else if (event.key.keysym.unicode == '{') { m_VKey = 0xeb; m_cAscii = '{'; }
      else if (event.key.keysym.unicode == '[') { m_VKey = 0xeb; m_cAscii = '['; } // 0xeb is not defined by MS. Why is it assigned here?
      else if (event.key.keysym.unicode == '|') { m_VKey = 0xec; m_cAscii = '|'; }
      else if (event.key.keysym.unicode == '\\') { m_VKey = 0xec; m_cAscii = '\\'; }
      else if (event.key.keysym.unicode == '}') { m_VKey = 0xed; m_cAscii = '}'; }
      else if (event.key.keysym.unicode == ']') { m_VKey = 0xed; m_cAscii = ']'; } // 0xed is not defined by MS. Why is it assigned here?
      else if (event.key.keysym.unicode == '"') { m_VKey = 0xee; m_cAscii = '"'; }
      else if (event.key.keysym.unicode == '\'') { m_VKey = 0xee; m_cAscii = '\''; }

      // For control key combinations, e.g. ctrl-P, the UNICODE gets set
      // to 1 for ctrl-A, 2 for ctrl-B etc. This mapping sets the UNICODE
      // back to 'a', 'b', etc.
      // It isn't clear to me if this applies to Linux and Mac as well as
      // Windows.
      if (m_bCtrl)
      {
        if (!m_VKey && !m_cAscii)
          LookupKeyMapping(&m_VKey, NULL, &m_wUnicode
                         , event.key.keysym.sym
                         , g_mapping_ctrlkeys
                         , sizeof(g_mapping_ctrlkeys)/sizeof(g_mapping_ctrlkeys[0]));
      }

      /* Check for standard non printable keys */
      if (!m_VKey && !m_cAscii)
        LookupKeyMapping(&m_VKey, NULL, &m_wUnicode
                       , event.key.keysym.sym
                       , g_mapping_npc
                       , sizeof(g_mapping_npc)/sizeof(g_mapping_npc[0]));


      if (!m_VKey && !m_cAscii)
      {
        /* Check for linux defined non printable keys */
        if(m_bEvdev)
          LookupKeyMapping(&m_VKey, NULL, NULL
                         , event.key.keysym.scancode
                         , g_mapping_evdev
                         , sizeof(g_mapping_evdev)/sizeof(g_mapping_evdev[0]));
        else
          LookupKeyMapping(&m_VKey, NULL, NULL
                         , event.key.keysym.scancode
                         , g_mapping_ubuntu
                         , sizeof(g_mapping_ubuntu)/sizeof(g_mapping_ubuntu[0]));
      }

      if (!m_VKey && !m_cAscii)
      {
        if (event.key.keysym.mod & XBMCKMOD_LSHIFT) m_VKey = 0xa0;
        else if (event.key.keysym.mod & XBMCKMOD_RSHIFT) m_VKey = 0xa1;
        else if (event.key.keysym.mod & XBMCKMOD_LALT) m_VKey = 0xa4;
        else if (event.key.keysym.mod & XBMCKMOD_RALT) m_VKey = 0xa5;
        else if (event.key.keysym.mod & XBMCKMOD_LCTRL) m_VKey = 0xa2;
        else if (event.key.keysym.mod & XBMCKMOD_RCTRL) m_VKey = 0xa3;
        else if (event.key.keysym.unicode > 32 && event.key.keysym.unicode < 128)
          // only TRUE ASCII! (Otherwise XBMC crashes! No unicode not even latin 1!)
          m_cAscii = (char)(event.key.keysym.unicode & 0xff);
      }
    }
  }
  else
  { // key up event
    Reset();
    m_lastKey = XBMCK_UNKNOWN;
    m_keyHoldTime = 0;
  }
}

char CKeyboardStat::GetAscii()
{
  char lowLevelAscii = m_cAscii;
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

WCHAR CKeyboardStat::GetUnicode()
{
  // More specific mappings, i.e. with scancodes and/or with one or even more modifiers,
  // must be handled first/prioritized over less specific mappings! Why?
  // Example: an us keyboard has: "]" on one key, the german keyboard has "+" on the same key,
  // additionally the german keyboard has "~" on the same key, but the "~"
  // can only be reached with the special modifier "AltGr" (right alt).
  // See http://en.wikipedia.org/wiki/Keyboard_layout.
  // If "+" is handled first, the key is already consumed and "~" can never be reached.
  // The least specific mappings, e.g. "regardless modifiers" should be done at last/least prioritized.

  WCHAR lowLevelUnicode = m_wUnicode;
  BYTE key = m_VKey;

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
