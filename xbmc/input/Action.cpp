/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Action.h"
#include "ActionIDs.h"
#include "ButtonTranslator.h"
#include "Key.h"

CAction::CAction(int actionID, float amount1 /* = 1.0f */, float amount2 /* = 0.0f */, const std::string &name /* = "" */, unsigned int holdTime /*= 0*/)
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
  m_holdTime = holdTime;
}

CAction::CAction(int actionID, unsigned int state, float posX, float posY, float offsetX, float offsetY, const std::string &name):
  m_name(name)
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

CAction::CAction(int actionID, const std::string &name, const CKey &key):
  m_name(name)
{
  m_id = actionID;
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

CAction::CAction(int actionID, const std::string &name):
  m_name(name)
{
  m_id = actionID;
  for (unsigned int i = 0; i < max_amounts; i++)
    m_amount[i] = 0;
  m_repeat = 0;
  m_buttonCode = 0;
  m_unicode = 0;
  m_holdTime = 0;
}

CAction& CAction::operator=(const CAction& rhs)
{
  if (this != &rhs)
  {
    m_id = rhs.m_id;
    for (unsigned int i = 0; i < max_amounts; i++)
      m_amount[i] = rhs.m_amount[i];
    m_name = rhs.m_name;
    m_repeat = rhs.m_repeat;
    m_buttonCode = rhs.m_buttonCode;
    m_unicode = rhs.m_unicode;
    m_holdTime = rhs.m_holdTime;
    m_text = rhs.m_text;
  }
  return *this;
}

bool CAction::IsMouse() const
{
  return (m_id >= ACTION_MOUSE_START && m_id <= ACTION_MOUSE_END);
}

bool CAction::IsGesture() const
{
  return (m_id >= ACTION_GESTURE_NOTIFY && m_id <= ACTION_GESTURE_END);
}

bool CAction::IsAnalog() const
{
  return CButtonTranslator::IsAnalog(m_id);
}
