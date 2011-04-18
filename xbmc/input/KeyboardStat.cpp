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
#include "windowing/XBMC_events.h"
#include "utils/TimeUtils.h"

#if defined(_LINUX) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

CKeyboardStat g_Keyboard;

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

// Set the vkey value for non-printing keys that do not have vkey or
// ascii equivalent.
// OSX defines unicode values for non-printing keys which breaks the
// key parser, set m_wUnicode
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
, { XBMCK_BROWSER_BACK,        XBMCK_BROWSER_BACK }
, { XBMCK_BROWSER_FORWARD,     XBMCK_BROWSER_FORWARD }
, { XBMCK_BROWSER_REFRESH,     XBMCK_BROWSER_REFRESH }
, { XBMCK_BROWSER_STOP,        XBMCK_BROWSER_STOP }
, { XBMCK_BROWSER_SEARCH,      XBMCK_BROWSER_SEARCH }
, { XBMCK_BROWSER_FAVORITES,   XBMCK_BROWSER_FAVORITES }
, { XBMCK_BROWSER_HOME,        XBMCK_BROWSER_HOME }
, { XBMCK_VOLUME_MUTE,         XBMCK_VOLUME_MUTE }
, { XBMCK_VOLUME_DOWN,         XBMCK_VOLUME_DOWN }
, { XBMCK_VOLUME_UP,           XBMCK_VOLUME_UP }
, { XBMCK_MEDIA_NEXT_TRACK,    XBMCK_MEDIA_NEXT_TRACK }
, { XBMCK_MEDIA_PREV_TRACK,    XBMCK_MEDIA_PREV_TRACK }
, { XBMCK_MEDIA_STOP,          XBMCK_MEDIA_STOP }
, { XBMCK_MEDIA_PLAY_PAUSE,    XBMCK_MEDIA_PLAY_PAUSE }
, { XBMCK_LAUNCH_MAIL,         XBMCK_LAUNCH_MAIL }
, { XBMCK_LAUNCH_MEDIA_SELECT, XBMCK_LAUNCH_MEDIA_SELECT }
, { XBMCK_LAUNCH_APP1,         XBMCK_LAUNCH_APP1 }
, { XBMCK_LAUNCH_APP2,         XBMCK_LAUNCH_APP2 }
};

#define NUM_KEYNAMES 0x100
static const char *keynames[NUM_KEYNAMES] =
{
  NULL,                 // 0x0
  NULL,                 // 0x1
  NULL,                 // 0x2
  NULL,                 // 0x3
  NULL,                 // 0x4
  NULL,                 // 0x5
  NULL,                 // 0x6
  NULL,                 // 0x7
  "backspace",          // 0x8
  "tab",                // 0x9
  NULL,                 // 0xA
  NULL,                 // 0xB
  NULL,                 // 0xC
  "return",             // 0xD
  NULL,                 // 0xE
  NULL,                 // 0xF
  NULL,                 // 0x10
  NULL,                 // 0x11
  NULL,                 // 0x12
  NULL,                 // 0x13
  NULL,                 // 0x14
  NULL,                 // 0x15
  NULL,                 // 0x16
  NULL,                 // 0x17
  NULL,                 // 0x18
  NULL,                 // 0x19
  NULL,                 // 0x1A
  "escape",             // 0x1B
  NULL,                 // 0x1C
  NULL,                 // 0x1D
  NULL,                 // 0x1E
  NULL,                 // 0x1F
  "space",              // 0x20
  "pageup",             // 0x21
  "pagedown",           // 0x22
  "end",                // 0x23
  "home",               // 0x24
  "left",               // 0x25
  "up",                 // 0x26
  "right",              // 0x27
  "down",               // 0x28
  NULL,                 // 0x29
  "printscreen",        // 0x2A
  NULL,                 // 0x2B
  NULL,                 // 0x2C
  "insert",             // 0x2D
  "delete",             // 0x2E
  NULL,                 // 0x2F
  NULL,                 // 0x30
  NULL,                 // 0x31
  NULL,                 // 0x32
  NULL,                 // 0x33
  NULL,                 // 0x34
  NULL,                 // 0x35
  NULL,                 // 0x36
  NULL,                 // 0x37
  NULL,                 // 0x38
  NULL,                 // 0x39
  NULL,                 // 0x3A
  NULL,                 // 0x3B
  NULL,                 // 0x3C
  NULL,                 // 0x3D
  NULL,                 // 0x3E
  NULL,                 // 0x3F
  NULL,                 // 0x40
  "A",                  // 0x41
  "B",                  // 0x42
  "C",                  // 0x43
  "D",                  // 0x44
  "E",                  // 0x45
  "F",                  // 0x46
  "G",                  // 0x47
  "H",                  // 0x48
  "I",                  // 0x49
  "J",                  // 0x4A
  "K",                  // 0x4B
  "L",                  // 0x4C
  "M",                  // 0x4D
  "N",                  // 0x4E
  "O",                  // 0x4F
  "P",                  // 0x50
  "Q",                  // 0x51
  "R",                  // 0x52
  "S",                  // 0x53
  "T",                  // 0x54
  "U",                  // 0x55
  "V",                  // 0x56
  "W",                  // 0x57
  "X",                  // 0x58
  "Y",                  // 0x59
  "Z",                  // 0x5A
  NULL,                 // 0x5B
  NULL,                 // 0x5C
  "menu",               // 0x5D
  NULL,                 // 0x5E
  NULL,                 // 0x5F
  "0",                  // 0x60
  "1",                  // 0x61
  "2",                  // 0x62
  "3",                  // 0x63
  "4",                  // 0x64
  "5",                  // 0x65
  "6",                  // 0x66
  "7",                  // 0x67
  "8",                  // 0x68
  "9",                  // 0x69
  NULL,                 // 0x6A
  NULL,                 // 0x6B
  "enter",              // 0x6C
  NULL,                 // 0x6D
  NULL,                 // 0x6E
  NULL,                 // 0x6F
  "f1",                 // 0x70
  "f2",                 // 0x71
  "f3",                 // 0x72
  "f4",                 // 0x73
  "f5",                 // 0x74
  "f6",                 // 0x75
  "f7",                 // 0x76
  "f8",                 // 0x77
  "f9",                 // 0x78
  "f10",                // 0x79
  "f11",                // 0x7A
  "f12",                // 0x7B
  NULL,                 // 0x7C
  NULL,                 // 0x7D
  NULL,                 // 0x7E
  NULL,                 // 0x7F
  NULL,                 // 0x80
  NULL,                 // 0x81
  NULL,                 // 0x82
  NULL,                 // 0x83
  NULL,                 // 0x84
  NULL,                 // 0x85
  NULL,                 // 0x86
  NULL,                 // 0x87
  NULL,                 // 0x88
  NULL,                 // 0x89
  NULL,                 // 0x8A
  NULL,                 // 0x8B
  NULL,                 // 0x8C
  NULL,                 // 0x8D
  NULL,                 // 0x8E
  NULL,                 // 0x8F
  NULL,                 // 0x90
  NULL,                 // 0x91
  NULL,                 // 0x92
  NULL,                 // 0x93
  NULL,                 // 0x94
  NULL,                 // 0x95
  NULL,                 // 0x96
  NULL,                 // 0x97
  NULL,                 // 0x98
  NULL,                 // 0x99
  NULL,                 // 0x9A
  NULL,                 // 0x9B
  NULL,                 // 0x9C
  NULL,                 // 0x9D
  NULL,                 // 0x9E
  NULL,                 // 0x9F
  "lshift",             // 0xA0
  "rshift",             // 0xA1
  "lcontrol",           // 0xA2
  "rcontrol",           // 0xA3
  "lalt",               // 0xA4
  "ralt",               // 0xA5
  "browser_back",       // 0xA6
  "browser_forward",    // 0xA7
  "browser_refresh",    // 0xA8
  "browser_stop",       // 0xA9
  "browser_search",     // 0xAA
  "browser_favorites",  // 0xAB
  "browser_home",       // 0xAC
  "volume_mute",        // 0xAD
  "volume_down",        // 0xAE
  "volume_up",          // 0xAF
  "next_track",         // 0xB0
  "prev_track",         // 0xB1
  "stop",               // 0xB2
  "play_pause",         // 0xB3
  "launch_mail",        // 0xB4
  "launch_media_select",// 0xB5
  "launch_app1_pc_icon",// 0xB6
  "launch_app2_pc_icon",// 0xB7
  NULL,                 // 0xB8
  NULL,                 // 0xB9
  "semicolon",          // 0xBA
  "equals",             // 0xBB
  "comma",              // 0xBC
  "minus",              // 0xBD
  "period",             // 0xBE
  "slash",              // 0xBF
  "leftquote",          // 0xC0
  NULL,                 // 0xC1
  NULL,                 // 0xC2
  NULL,                 // 0xC3
  NULL,                 // 0xC4
  NULL,                 // 0xC5
  NULL,                 // 0xC6
  NULL,                 // 0xC7
  NULL,                 // 0xC8
  NULL,                 // 0xC9
  NULL,                 // 0xCA
  NULL,                 // 0xCB
  NULL,                 // 0xCC
  NULL,                 // 0xCD
  NULL,                 // 0xCE
  NULL,                 // 0xCF
  NULL,                 // 0xD0
  NULL,                 // 0xD1
  NULL,                 // 0xD2
  NULL,                 // 0xD3
  NULL,                 // 0xD4
  NULL,                 // 0xD5
  NULL,                 // 0xD6
  NULL,                 // 0xD7
  NULL,                 // 0xD8
  NULL,                 // 0xD9
  NULL,                 // 0xDA
  NULL,                 // 0xDB
  NULL,                 // 0xDC
  NULL,                 // 0xDD
  NULL,                 // 0xDE
  NULL,                 // 0xDF
  NULL,                 // 0xE0
  NULL,                 // 0xE1
  NULL,                 // 0xE2
  NULL,                 // 0xE3
  NULL,                 // 0xE4
  NULL,                 // 0xE5
  NULL,                 // 0xE6
  NULL,                 // 0xE7
  NULL,                 // 0xE8
  NULL,                 // 0xE9
  NULL,                 // 0xEA
  "opensquarebracket",  // 0xEB
  "backslash",          // 0xEC
  "closesquarebracket", // 0xED
  "'",                  // 0xEE
  NULL,                 // 0xEF
  NULL,                 // 0xF0
  NULL,                 // 0xF1
  NULL,                 // 0xF2
  NULL,                 // 0xF3
  NULL,                 // 0xF4
  NULL,                 // 0xF5
  NULL,                 // 0xF6
  NULL,                 // 0xF7
  NULL,                 // 0xF8
  NULL,                 // 0xF9
  NULL,                 // 0xFA
  NULL,                 // 0xFB
  NULL,                 // 0xFC
  NULL,                 // 0xFD
  NULL,                 // 0xFE
  NULL                  // 0xFF
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
  memset(&m_lastKeysym, 0, sizeof(m_lastKeysym));
  m_lastKeyTime = 0;

  // In Linux the codes (numbers) for multimedia keys differ depending on
  // what driver is used and the evdev bool switches between the two.
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

const CKey CKeyboardStat::ProcessKeyDown(XBMC_keysym& keysym)
{ uint8_t vkey;
  wchar_t unicode;
  char ascii;
  uint32_t modifiers;
  unsigned int held;

  ascii = 0;
  vkey = 0;
  unicode = keysym.unicode;
  held = 0;

  modifiers = 0;
  if (keysym.mod & XBMCKMOD_CTRL)
    modifiers |= CKey::MODIFIER_CTRL;
  if (keysym.mod & XBMCKMOD_SHIFT)
    modifiers |= CKey::MODIFIER_SHIFT;
  if (keysym.mod & XBMCKMOD_ALT)
    modifiers |= CKey::MODIFIER_ALT;
  if (keysym.mod & XBMCKMOD_RALT)
    modifiers |= CKey::MODIFIER_RALT;
  if (keysym.mod & XBMCKMOD_SUPER)
    modifiers |= CKey::MODIFIER_SUPER;

  CLog::Log(LOGDEBUG, "SDLKeyboard: scancode: %d, sym: %d, unicode: %d, modifier: %x", keysym.scancode, keysym.sym, keysym.unicode, keysym.mod);

  if ((keysym.unicode >= 'A' && keysym.unicode <= 'Z') ||
    (keysym.unicode >= 'a' && keysym.unicode <= 'z'))
  {
    ascii = (char)keysym.unicode;
    vkey = toupper(ascii);
  }
  else if (keysym.unicode >= '0' && keysym.unicode <= '9')
  {
    ascii = (char)keysym.unicode;
    vkey = 0x60 + ascii - '0'; // xbox keyboard routine appears to return 0x60->69 (unverified). Ideally this "fixing"
    // should be done in xbox routine, not in the sdl/directinput routines.
    // we should just be using the unicode/ascii value in all routines (perhaps with some
    // headroom for modifier keys?)
  }
  else
  {
    // see comment above about the weird use of vkey here...
    if (keysym.unicode == ')') { vkey = 0x60; ascii = ')'; }
    else if (keysym.unicode == '!') { vkey = 0x61; ascii = '!'; }
    else if (keysym.unicode == '@') { vkey = 0x62; ascii = '@'; }
    else if (keysym.unicode == '#') { vkey = 0x63; ascii = '#'; }
    else if (keysym.unicode == '$') { vkey = 0x64; ascii = '$'; }
    else if (keysym.unicode == '%') { vkey = 0x65; ascii = '%'; }
    else if (keysym.unicode == '^') { vkey = 0x66; ascii = '^'; }
    else if (keysym.unicode == '&') { vkey = 0x67; ascii = '&'; }
    else if (keysym.unicode == '*') { vkey = 0x68; ascii = '*'; }
    else if (keysym.unicode == '(') { vkey = 0x69; ascii = '('; }
    else if (keysym.unicode == ':') { vkey = 0xba; ascii = ':'; }
    else if (keysym.unicode == ';') { vkey = 0xba; ascii = ';'; }
    else if (keysym.unicode == '=') { vkey = 0xbb; ascii = '='; }
    else if (keysym.unicode == '+') { vkey = 0xbb; ascii = '+'; }
    else if (keysym.unicode == '<') { vkey = 0xbc; ascii = '<'; }
    else if (keysym.unicode == ',') { vkey = 0xbc; ascii = ','; }
    else if (keysym.unicode == '-') { vkey = 0xbd; ascii = '-'; }
    else if (keysym.unicode == '_') { vkey = 0xbd; ascii = '_'; }
    else if (keysym.unicode == '>') { vkey = 0xbe; ascii = '>'; }
    else if (keysym.unicode == '.') { vkey = 0xbe; ascii = '.'; }
    else if (keysym.unicode == '?') { vkey = 0xbf; ascii = '?'; } // 0xbf is OEM 2 Why is it assigned here?
    else if (keysym.unicode == '/') { vkey = 0xbf; ascii = '/'; }
    else if (keysym.unicode == '~') { vkey = 0xc0; ascii = '~'; }
    else if (keysym.unicode == '`') { vkey = 0xc0; ascii = '`'; }
    else if (keysym.unicode == '{') { vkey = 0xeb; ascii = '{'; }
    else if (keysym.unicode == '[') { vkey = 0xeb; ascii = '['; } // 0xeb is not defined by MS. Why is it assigned here?
    else if (keysym.unicode == '|') { vkey = 0xec; ascii = '|'; }
    else if (keysym.unicode == '\\') { vkey = 0xec; ascii = '\\'; }
    else if (keysym.unicode == '}') { vkey = 0xed; ascii = '}'; }
    else if (keysym.unicode == ']') { vkey = 0xed; ascii = ']'; } // 0xed is not defined by MS. Why is it assigned here?
    else if (keysym.unicode == '"') { vkey = 0xee; ascii = '"'; }
    else if (keysym.unicode == '\'') { vkey = 0xee; ascii = '\''; }

    // For control key combinations, e.g. ctrl-P, the UNICODE gets set
    // to 1 for ctrl-A, 2 for ctrl-B etc. This mapping sets the UNICODE
    // back to 'a', 'b', etc.
    // It isn't clear to me if this applies to Linux and Mac as well as
    // Windows.
    if (modifiers & CKey::MODIFIER_CTRL)
    {
      if (!vkey && !ascii)
        LookupKeyMapping(&vkey, NULL, &unicode
                       , keysym.sym
                       , g_mapping_ctrlkeys
                       , sizeof(g_mapping_ctrlkeys)/sizeof(g_mapping_ctrlkeys[0]));
    }

    /* Check for standard non printable keys */
    if (!vkey && !ascii)
      LookupKeyMapping(&vkey, NULL, &unicode
                     , keysym.sym
                     , g_mapping_npc
                     , sizeof(g_mapping_npc)/sizeof(g_mapping_npc[0]));


    if (!vkey && !ascii)
    {
      /* Check for linux defined non printable keys */
        if(m_bEvdev)
          LookupKeyMapping(&vkey, NULL, NULL
                         , keysym.scancode
                         , g_mapping_evdev
                         , sizeof(g_mapping_evdev)/sizeof(g_mapping_evdev[0]));
        else
          LookupKeyMapping(&vkey, NULL, NULL
                         , keysym.scancode
                         , g_mapping_ubuntu
                         , sizeof(g_mapping_ubuntu)/sizeof(g_mapping_ubuntu[0]));
    }

    if (!vkey && !ascii)
    {
      if (keysym.mod & XBMCKMOD_LSHIFT) vkey = 0xa0;
      else if (keysym.mod & XBMCKMOD_RSHIFT) vkey = 0xa1;
      else if (keysym.mod & XBMCKMOD_LALT) vkey = 0xa4;
      else if (keysym.mod & XBMCKMOD_RALT) vkey = 0xa5;
      else if (keysym.mod & XBMCKMOD_LCTRL) vkey = 0xa2;
      else if (keysym.mod & XBMCKMOD_RCTRL) vkey = 0xa3;
      else if (keysym.unicode > 32 && keysym.unicode < 128)
        // only TRUE ASCII! (Otherwise XBMC crashes! No unicode not even latin 1!)
        ascii = (char)(keysym.unicode & 0xff);
    }
  }

  // At this point update the key hold time
  // If XBMC_keysym was a class we could use == but memcmp it is :-(
  if (memcmp(&keysym, &m_lastKeysym, sizeof(XBMC_keysym)) == 0)
  {
    held = CTimeUtils::GetFrameTime() - m_lastKeyTime;
  }
  else
  {
    m_lastKeysym = keysym;
    m_lastKeyTime = CTimeUtils::GetFrameTime();
    held = 0;
  }

  // Create and return a CKey

  CKey key(vkey, unicode, ascii, modifiers, held);
    
  return key;
}

void CKeyboardStat::ProcessKeyUp(void)
{
  memset(&m_lastKeysym, 0, sizeof(m_lastKeysym));
  m_lastKeyTime = 0;
}

// Return the key name given a key ID
// Used to make the debug log more intelligable
// The KeyID includes the flags for ctrl, alt etc

CStdString CKeyboardStat::GetKeyName(int KeyID)
{ int keyid;
  CStdString keyname;

  keyname.clear();

// Get modifiers

  if (KeyID & CKey::MODIFIER_CTRL)
    keyname.append("ctrl-");
  if (KeyID & CKey::MODIFIER_SHIFT)
    keyname.append("shift-");
  if (KeyID & CKey::MODIFIER_ALT)
    keyname.append("alt-");
  if (KeyID & CKey::MODIFIER_SUPER)
    keyname.append("win-");

// Now get the key name
  
  keyid = KeyID & 0x0FFF;
  if (keyid > NUM_KEYNAMES)
    keyname.AppendFormat("%i", keyid);
  else if (!keynames[keyid])
    keyname.AppendFormat("%i", keyid);
  else
    keyname.append(keynames[keyid]);

  keyname.AppendFormat(" (%02x)", KeyID);

  return keyname;
}


