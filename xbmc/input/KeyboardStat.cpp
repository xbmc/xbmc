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
#include "input/XBMC_keytable.h"

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
  XBMCKEYTABLE keytable;

  ascii = 0;
  vkey = 0;
  unicode = 0;
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

  CLog::Log(LOGDEBUG, "SDLKeyboard: scancode: %02x, sym: %04x, unicode: %04x, modifier: %x", keysym.scancode, keysym.sym, keysym.unicode, keysym.mod);

  // For control key combinations, e.g. ctrl-P, the UNICODE gets set
  // to 1 for ctrl-A, 2 for ctrl-B etc. To get round this, if the
  // control key is down lookup by sym
  if (modifiers & CKey::MODIFIER_CTRL && KeyTableLookupSym(keysym.sym, &keytable))
  {
    unicode = keytable.unicode;
    ascii   = keytable.ascii;
    vkey    = keytable.vkey;
  }

  // For printing keys look up the unicode
  else if (KeyTableLookupUnicode(keysym.unicode, &keytable))
  {
    unicode = keytable.unicode;
    ascii = keytable.ascii;
    vkey = keytable.vkey;
  }

  // For non-printing keys look up the sym
  else if (KeyTableLookupSym(keysym.sym, &keytable))
  {
    vkey = keytable.vkey;
  }

  else
  {
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
  XBMCKEYTABLE keytable;

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

  keyid = KeyID & 0xFF;
  if (KeyTableLookupVKeyName(keyid, &keytable))
    keyname.append(keytable.keyname);
  else
    keyname.AppendFormat("%i", keyid);
  keyname.AppendFormat(" (%02x)", KeyID);

  return keyname;
}


