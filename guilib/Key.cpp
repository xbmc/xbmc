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
  m_leftTrigger = 0;
  m_rightTrigger = 0;
  m_leftThumbX = 0.0f;
  m_leftThumbY = 0.0f;
  m_rightThumbX = 0.0f;
  m_rightThumbY = 0.0f;
  m_repeat = 0.0f;
  m_fromHttpApi = false;
  m_buttonCode = KEY_INVALID;
  m_VKey = 0;
  m_wUnicode = 0;
  m_cAscii = 0;
  m_bShift = 0;
  m_bCtrl = 0;
  m_bAlt = 0;
  m_bRAlt = 0;
  m_bSuper = 0;
  m_held = 0;
}

CKey::~CKey(void)
{}

/* Commented out temporarily prior to permanent removal
CKey::CKey(uint32_t buttonCode, uint8_t leftTrigger, uint8_t rightTrigger, float leftThumbX, float leftThumbY, float rightThumbX, float rightThumbY, float repeat)
{
  m_leftTrigger = leftTrigger;
  m_rightTrigger = rightTrigger;
  m_leftThumbX = leftThumbX;
  m_leftThumbY = leftThumbY;
  m_rightThumbX = rightThumbX;
  m_rightThumbY = rightThumbY;
  m_repeat = repeat;
  m_fromHttpApi = false;
  m_buttonCode = buttonCode;
  m_VKey = 0;
  m_wUnicode = 0;
  m_cAscii = 0;
  m_bShift = 0;
  m_bCtrl = 0;
  m_bAlt = 0;
  m_bRAlt = 0;
  m_bSuper = 0;
  m_held = 0;
}
*/

CKey::CKey(uint32_t buttonCode, uint8_t leftTrigger, uint8_t rightTrigger, float leftThumbX, float leftThumbY, float rightThumbX, float rightThumbY, float repeat, uint8_t vkey, wchar_t unicode, char ascii, bool shift, bool ctrl, bool alt, bool ralt, bool super, unsigned int held)
{
  m_leftTrigger = leftTrigger;
  m_rightTrigger = rightTrigger;
  m_leftThumbX = leftThumbX;
  m_leftThumbY = leftThumbY;
  m_rightThumbX = rightThumbX;
  m_rightThumbY = rightThumbY;
  m_repeat = repeat;
  m_fromHttpApi = false;
  m_buttonCode = buttonCode;
  m_VKey = vkey;
  m_wUnicode = unicode;
  m_cAscii = ascii;
  m_bShift = shift;
  m_bCtrl = ctrl;
  m_bAlt = alt;
  m_bRAlt = ralt;
  m_bSuper = super;
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
  m_fromHttpApi = false;
  m_buttonCode = KEY_INVALID;
  m_VKey = 0;
  m_wUnicode = 0;
  m_cAscii = 0;
  m_bShift = 0;
  m_bCtrl = 0;
  m_bAlt = 0;
  m_bRAlt = 0;
  m_bSuper = 0;
  m_held = 0;
}

const CKey& CKey::operator=(const CKey& key)
{
  if (&key == this) return * this;
  m_leftTrigger  = key.m_leftTrigger;
  m_rightTrigger = key.m_rightTrigger;
  m_leftThumbX   = key.m_leftThumbX;
  m_leftThumbY   = key.m_leftThumbY;
  m_rightThumbX  = key.m_rightThumbX;
  m_rightThumbY  = key.m_rightThumbY;
  m_repeat       = key.m_repeat;
  m_fromHttpApi  = key.m_fromHttpApi;
  m_buttonCode   = key.m_buttonCode;
  m_VKey         = key.m_VKey;
  m_wUnicode     = key.m_wUnicode;
  m_cAscii       = key.m_cAscii;
  m_bShift       = key.m_bShift;
  m_bCtrl        = key.m_bCtrl;
  m_bAlt         = key.m_bAlt;
  m_bRAlt        = key.m_bRAlt;
  m_bSuper       = m_bSuper;
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

bool CKey::GetFromHttpApi() const
{
  return m_fromHttpApi;
}

void CKey::SetFromHttpApi(bool bFromHttpApi)
{
  m_fromHttpApi = bFromHttpApi;
}

void CKey::SetButtonCode(uint32_t buttoncode)
{
  m_buttonCode = buttoncode;
}

void CKey::SetVKey(uint8_t vkey)
{
  m_VKey = vkey;
}

void CKey::SetAscii(char ascii)
{
  m_cAscii = ascii;
}

void CKey::SetUnicode(wchar_t unicode)
{
  m_wUnicode = unicode;
}

void CKey::SetModifiers(bool ctrl, bool shift, bool alt, bool ralt, bool super)
{
  m_bCtrl  = ctrl;
  m_bShift = shift;
  m_bAlt   = alt;
  m_bRAlt  = ralt;
  m_bSuper = super;
}

void CKey::SetHeld(unsigned int held)
{
  m_held = held;
}

CAction::CAction(int actionID, float amount1 /* = 1.0f */, float amount2 /* = 0.0f */, const CStdString &name /* = "" */)
{
  m_id = actionID;
  m_amount[0] = amount1;
  m_amount[1] = amount2;
  for (unsigned int i = 2; i < max_amounts; i++)
    m_amount[i] = 0;  
  m_name = name;
  m_repeat = 0;
  m_buttonCode = 0;
  m_unicode = 0;
  m_holdTime = 0;
}

CAction::CAction(int actionID, unsigned int state, float posX, float posY, float offsetX, float offsetY)
{
  m_id = actionID;
  m_amount[0] = posX;
  m_amount[1] = posY;
  m_amount[2] = offsetX;
  m_amount[3] = offsetY;
  for (unsigned int i = 4; i < max_amounts; i++)
    m_amount[i] = 0;  
  m_repeat = 0;
  m_buttonCode = 0;
  m_unicode = 0;
  m_holdTime = state;
}

CAction::CAction(int actionID, wchar_t unicode)
{
  m_id = actionID;
  for (unsigned int i = 0; i < max_amounts; i++)
    m_amount[i] = 0;  
  m_repeat = 0;
  m_buttonCode = 0;
  m_unicode = unicode;
  m_holdTime = 0;
}

CAction::CAction(int actionID, const CStdString &name, const CKey &key)
{
  m_id = actionID;
  m_name = name;
  m_amount[0] = 1; // digital button (could change this for repeat acceleration)
  for (unsigned int i = 1; i < max_amounts; i++)
    m_amount[i] = 0;
  m_repeat = key.GetRepeat();
  m_buttonCode = key.GetButtonCode();
  m_unicode = 0;
  m_holdTime = key.GetHeld();
  // get the action amounts of the analog buttons
  if (key.GetButtonCode() == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
    m_amount[0] = (float)key.GetLeftTrigger() / 255.0f;
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
    m_amount[0] = (float)key.GetRightTrigger() / 255.0f;
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK)
  {
    m_amount[0] = key.GetLeftThumbX();
    m_amount[1] = key.GetLeftThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK)
  {
    m_amount[0] = key.GetRightThumbX();
    m_amount[1] = key.GetRightThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_UP)
    m_amount[0] = key.GetLeftThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
    m_amount[0] = -key.GetLeftThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
    m_amount[0] = -key.GetLeftThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
    m_amount[0] = key.GetLeftThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
    m_amount[0] = key.GetRightThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
    m_amount[0] = -key.GetRightThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
    m_amount[0] = -key.GetRightThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
    m_amount[0] = key.GetRightThumbX();
}
