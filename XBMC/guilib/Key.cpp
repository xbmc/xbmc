/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "Key.h"

CKey::CKey(void)
{
  m_buttonCode = KEY_INVALID;
  m_leftTrigger = 0;
  m_rightTrigger = 0;
  m_leftThumbX = 0.0f;
  m_leftThumbY = 0.0f;
  m_rightThumbX = 0.0f;
  m_rightThumbY = 0.0f;
  m_repeat = 0.0f;
  m_fromHttpApi = false;
  m_held = 0;
}

CKey::~CKey(void)
{}

CKey::CKey(uint32_t buttonCode, uint8_t leftTrigger, uint8_t rightTrigger, float leftThumbX, float leftThumbY, float rightThumbX, float rightThumbY, float repeat)
{
  m_leftTrigger = leftTrigger;
  m_rightTrigger = rightTrigger;
  m_leftThumbX = leftThumbX;
  m_leftThumbY = leftThumbY;
  m_rightThumbX = rightThumbX;
  m_rightThumbY = rightThumbY;
  m_buttonCode = buttonCode;
  m_repeat = repeat;
  m_fromHttpApi = false;
  m_held = 0;
}

CKey::CKey(const CKey& key)
{
  *this = key;
}

uint32_t CKey::GetButtonCode() const // for backwards compatibility only
{
  return m_buttonCode;
}

uint32_t CKey::GetUnicode() const
{
  if (m_buttonCode>=KEY_ASCII && m_buttonCode < KEY_UNICODE) // will need to change when Unicode is fully implemented
    return m_buttonCode-KEY_ASCII;
  else
    return 0;
}

const CKey& CKey::operator=(const CKey& key)
{
  if (&key == this) return * this;
  m_leftTrigger = key.m_leftTrigger;
  m_rightTrigger = key.m_rightTrigger;
  m_buttonCode = key.m_buttonCode;
  m_leftThumbX = key.m_leftThumbX;
  m_leftThumbY = key.m_leftThumbY;
  m_rightThumbX = key.m_rightThumbX;
  m_rightThumbY = key.m_rightThumbY;
  m_repeat = key.m_repeat;
  m_fromHttpApi = key.m_fromHttpApi;
  m_held = key.m_held;
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

bool CKey::GetFromHttpApi() const
{
  return m_fromHttpApi;
}

void CKey::SetFromHttpApi(bool bFromHttpApi)
{
  m_fromHttpApi = bFromHttpApi;
}

void CKey::SetHeld(unsigned int held)
{
  m_held = held;
}

unsigned int CKey::GetHeld() const
{
  return m_held;
}

