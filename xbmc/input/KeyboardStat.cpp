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

// C++ Implementation: CKeyboard

// Comment OUT, if not really debugging!!!:
//#define DEBUG_KEYBOARD_GETCHAR

#include "KeyboardStat.h"
#include "windowing/XBMC_events.h"
#include "input/XBMC_keytable.h"
#include "input/XBMC_vkeys.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/PeripheralHID.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

using namespace std;
using namespace PERIPHERALS;

bool operator==(const XBMC_keysym& lhs, const XBMC_keysym& rhs)
{
  return lhs.mod      == rhs.mod      &&
         lhs.scancode == rhs.scancode &&
         lhs.sym      == rhs.sym      &&
         lhs.unicode  == rhs.unicode;
}

CKeyboardStat::CKeyboardStat()
{
  memset(&m_lastKeysym, 0, sizeof(m_lastKeysym));
  m_lastKeyTime = 0;
}

CKeyboardStat::~CKeyboardStat()
{
}

void CKeyboardStat::Initialize()
{
}

bool CKeyboardStat::LookupSymAndUnicodePeripherals(XBMC_keysym &keysym, uint8_t *key, char *unicode)
{
  vector<CPeripheral *> hidDevices;
  if (g_peripherals.GetPeripheralsWithFeature(hidDevices, FEATURE_HID))
  {
    for (unsigned int iDevicePtr = 0; iDevicePtr < hidDevices.size(); iDevicePtr++)
    {
      CPeripheralHID *hidDevice = (CPeripheralHID *) hidDevices.at(iDevicePtr);
      if (hidDevice && hidDevice->LookupSymAndUnicode(keysym, key, unicode))
        return true;
    }
  }
  return false;
}

CKey CKeyboardStat::TranslateKey(XBMC_keysym& keysym) const
{
  uint8_t vkey;
  wchar_t unicode;
  char ascii;
  uint32_t modifiers;
  unsigned int held;
  XBMCKEYTABLE keytable;

  modifiers = 0;
  if (keysym.mod & XBMCKMOD_CTRL)
    modifiers |= CKey::MODIFIER_CTRL;
  if (keysym.mod & XBMCKMOD_SHIFT)
    modifiers |= CKey::MODIFIER_SHIFT;
  if (keysym.mod & XBMCKMOD_ALT)
    modifiers |= CKey::MODIFIER_ALT;
  if (keysym.mod & XBMCKMOD_SUPER)
    modifiers |= CKey::MODIFIER_SUPER;
  if (keysym.mod & XBMCKMOD_META)
    modifiers |= CKey::MODIFIER_META;

  CLog::Log(LOGDEBUG, "Keyboard: scancode: 0x%02x, sym: 0x%04x, unicode: 0x%04x, modifier: 0x%x", keysym.scancode, keysym.sym, keysym.unicode, keysym.mod);

  // The keysym.unicode is usually valid, even if it is zero. A zero
  // unicode just means this is a non-printing keypress. The ascii and
  // vkey will be set below.
  unicode = keysym.unicode;
  ascii = 0;
  vkey = 0;
  held = 0;

  // Start by check whether any of the HID peripherals wants to translate this keypress
  if (LookupSymAndUnicodePeripherals(keysym, &vkey, &ascii))
  {
    CLog::Log(LOGDEBUG, "%s - keypress translated by a HID peripheral", __FUNCTION__);
  }

  // Continue by trying to match both the sym and unicode. This will identify
  // the majority of keypresses
  else if (KeyTableLookupSymAndUnicode(keysym.sym, keysym.unicode, &keytable))
  {
    vkey = keytable.vkey;
    ascii = keytable.ascii;
  }

  // If we failed to match the sym and unicode try just the unicode. This
  // will match keys like \ that are on different keys on regional keyboards.
  else if (KeyTableLookupUnicode(keysym.unicode, &keytable))
  {
    vkey = keytable.vkey;
    ascii = keytable.ascii;
  }

  // If there is still no match try the sym
  else if (KeyTableLookupSym(keysym.sym, &keytable))
  {
    vkey = keytable.vkey;

    // Occasionally we get non-printing keys that have a non-zero value in
    // the keysym.unicode. Check for this here and replace any rogue unicode
    // values.
    if (keytable.unicode == 0 && unicode != 0)
      unicode = 0;
    else if (keysym.unicode > 32 && keysym.unicode < 128)
      ascii = unicode & 0x7f;
  }

  // The keysym.sym is unknown ...
  else
  {
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

  if (keysym == m_lastKeysym)
  {
    held = XbmcThreads::SystemClockMillis() - m_lastKeyTime;
  }

  // For all shift-X keys except shift-A to shift-Z and shift-F1 to shift-F24 the
  // shift modifier is ignored. This so that, for example, the * keypress (shift-8)
  // is seen as <asterisk> not <asterisk mod="shift">.
  // The A-Z keys are exempted because shift-A-Z is used for navigation in lists.
  // The function keys are exempted because function keys have no shifted value and
  // the Nyxboard remote uses keys like Shift-F3 for some buttons.
  if (modifiers == CKey::MODIFIER_SHIFT)
    if ((unicode < 'A' || unicode > 'Z') && (unicode < 'a' || unicode > 'z') && (vkey < XBMCVK_F1 || vkey > XBMCVK_F24))
      modifiers = 0;

  // Create and return a CKey

  CKey key(vkey, unicode, ascii, modifiers, held);

  return key;
}

void CKeyboardStat::ProcessKeyDown(XBMC_keysym& keysym)
{
  if (!(m_lastKeysym == keysym))
  {
    m_lastKeysym = keysym;
    m_lastKeyTime = XbmcThreads::SystemClockMillis();
  }
}

void CKeyboardStat::ProcessKeyUp(void)
{
  memset(&m_lastKeysym, 0, sizeof(m_lastKeysym));
  m_lastKeyTime = 0;
}

// Return the key name given a key ID
// Used to make the debug log more intelligable
// The KeyID includes the flags for ctrl, alt etc

std::string CKeyboardStat::GetKeyName(int KeyID)
{ int keyid;
  std::string keyname;
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
  if (KeyID & CKey::MODIFIER_META)
    keyname.append("meta-");

// Now get the key name

  keyid = KeyID & 0xFF;
  if (KeyTableLookupVKeyName(keyid, &keytable))
    keyname.append(keytable.keyname);
  else
    keyname += StringUtils::Format("%i", keyid);
  keyname += StringUtils::Format(" (0x%02x)", KeyID);

  return keyname;
}


