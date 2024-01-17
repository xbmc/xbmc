/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Key.h"

#include "KeyIDs.h"

CKey::CKey(void)
{
  Reset();
}

CKey::~CKey(void) = default;

CKey::CKey(uint32_t buttonCode,
           uint8_t leftTrigger,
           uint8_t rightTrigger,
           float leftThumbX,
           float leftThumbY,
           float rightThumbX,
           float rightThumbY,
           float repeat)
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

CKey::CKey(uint32_t keycode,
           uint8_t vkey,
           wchar_t unicode,
           char ascii,
           uint32_t modifiers,
           uint32_t lockingModifiers,
           unsigned int held)
{
  Reset();
  if (vkey) // FIXME: This needs cleaning up - should we always use the unicode key where available?
    m_buttonCode = vkey | KEY_VKEY;
  else
    m_buttonCode = KEY_UNICODE;
  m_buttonCode |= modifiers;
  m_keycode = keycode;
  m_vkey = vkey;
  m_unicode = unicode;
  m_ascii = ascii;
  m_modifiers = modifiers;
  m_lockingModifiers = lockingModifiers;
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
  m_keycode = 0;
  m_vkey = 0;
  m_unicode = 0;
  m_ascii = 0;
  m_modifiers = 0;
  m_lockingModifiers = 0;
  m_held = 0;
}

CKey& CKey::operator=(const CKey& key)
{
  if (&key == this)
    return *this;
  m_leftTrigger = key.m_leftTrigger;
  m_rightTrigger = key.m_rightTrigger;
  m_leftThumbX = key.m_leftThumbX;
  m_leftThumbY = key.m_leftThumbY;
  m_rightThumbX = key.m_rightThumbX;
  m_rightThumbY = key.m_rightThumbY;
  m_repeat = key.m_repeat;
  m_fromService = key.m_fromService;
  m_buttonCode = key.m_buttonCode;
  m_keycode = key.m_keycode;
  m_vkey = key.m_vkey;
  m_unicode = key.m_unicode;
  m_ascii = key.m_ascii;
  m_modifiers = key.m_modifiers;
  m_lockingModifiers = key.m_lockingModifiers;
  m_held = key.m_held;
  return *this;
}

uint8_t CKey::GetLeftTrigger() const
{
  return m_leftTrigger;
}

uint8_t CKey::GetRightTrigger() const
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
  if ((GetButtonCode() > 261 && GetButtonCode() < 270) ||
      (GetButtonCode() > 279 && GetButtonCode() < 284))
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
  if (fromService && (m_buttonCode & KEY_VKEY))
    m_unicode = m_buttonCode - KEY_VKEY;

  m_fromService = fromService;
}
