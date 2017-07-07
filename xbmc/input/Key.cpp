/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "input/Key.h"

CKey::CKey(void)
{
  Reset();
}

CKey::~CKey(void) = default;

CKey::CKey(uint32_t buttonCode, uint8_t leftTrigger, uint8_t rightTrigger, float leftThumbX, float leftThumbY, float rightThumbX, float rightThumbY, float repeat)
{
  Reset();
  m_buttonCode = buttonCode;
  m_leftTrigger = leftTrigger;
  m_rightTrigger = rightTrigger;
  m_leftThumbX = leftThumbX;
  m_leftThumbY = leftThumbY;
  m_rightThumbX = rightThumbX;
  m_rightThumbY = rightThumbY;
  m_repeat = repeat;
}

CKey::CKey(uint32_t buttonCode, unsigned int held)
{
  Reset();
  m_buttonCode = buttonCode;
  m_held = held;
}

CKey::CKey(uint8_t vkey, wchar_t unicode, char ascii, uint32_t modifiers, unsigned int held)
{
  Reset();
  if (vkey) // FIXME: This needs cleaning up - should we always use the unicode key where available?
    m_buttonCode = vkey | KEY_VKEY;
  else
    m_buttonCode = KEY_UNICODE;
  m_buttonCode |= modifiers;
  m_vkey = vkey;
  m_unicode = unicode;
  m_ascii = ascii;
  m_modifiers = modifiers;
  m_held = held;
}

CKey::CKey(const CKey& key)
{
  *this = key;
}

void CKey::Reset()
{
  m_leftTrigger = 0;
  m_rightTrigger = 0;
  m_leftThumbX = 0.0f;
  m_leftThumbY = 0.0f;
  m_rightThumbX = 0.0f;
  m_rightThumbY = 0.0f;
  m_repeat = 0.0f;
  m_fromService = false;
  m_buttonCode = KEY_INVALID;
  m_vkey = 0;
  m_unicode = 0;
  m_ascii = 0;
  m_modifiers = 0;
  m_held = 0;
}

CKey& CKey::operator=(const CKey& key)
{
  if (&key == this) return * this;
  m_leftTrigger  = key.m_leftTrigger;
  m_rightTrigger = key.m_rightTrigger;
  m_leftThumbX   = key.m_leftThumbX;
  m_leftThumbY   = key.m_leftThumbY;
  m_rightThumbX  = key.m_rightThumbX;
  m_rightThumbY  = key.m_rightThumbY;
  m_repeat       = key.m_repeat;
  m_fromService  = key.m_fromService;
  m_buttonCode   = key.m_buttonCode;
  m_vkey         = key.m_vkey;
  m_unicode     = key.m_unicode;
  m_ascii       = key.m_ascii;
  m_modifiers    = key.m_modifiers;
  m_held         = key.m_held;
  return *this;
}

BYTE CKey::GetLeftTrigger() const
{
  return m_leftTrigger;
}

BYTE CKey::GetRightTrigger() const
{
  return m_rightTrigger;
}

float CKey::GetLeftThumbX() const
{
  return m_leftThumbX;
}

float CKey::GetLeftThumbY() const
{
  return m_leftThumbY;
}


float CKey::GetRightThumbX() const
{
  return m_rightThumbX;
}

float CKey::GetRightThumbY() const
{
  return m_rightThumbY;
}

bool CKey::FromKeyboard() const
{
  return (m_buttonCode >= KEY_VKEY && m_buttonCode != KEY_INVALID);
}

bool CKey::IsAnalogButton() const
{
  if ((GetButtonCode() > 261 && GetButtonCode() < 270) || (GetButtonCode() > 279 && GetButtonCode() < 284))
    return true;

  return false;
}

bool CKey::IsIRRemote() const
{
  if (GetButtonCode() < 256)
    return true;
  return false;
}

float CKey::GetRepeat() const
{
  return m_repeat;
}

void CKey::SetFromService(bool fromService)
{
  if (fromService && (m_buttonCode & KEY_ASCII))
    m_unicode = m_buttonCode - KEY_ASCII;
    
  m_fromService = fromService;
}
