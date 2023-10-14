/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Action.h"

#include "ActionIDs.h"
#include "ActionTranslator.h"
#include "input/Key.h"

CAction::CAction() : m_id(ACTION_NONE)
{
}

CAction::CAction(int actionID,
                 float amount1 /* = 1.0f */,
                 float amount2 /* = 0.0f */,
                 const std::string& name /* = "" */,
                 unsigned int holdTime /*= 0*/,
                 unsigned int buttonCode /*= 0*/)
  : m_name(name)
{
  m_id = actionID;
  m_amount[0] = amount1;
  m_amount[1] = amount2;
  m_repeat = 0;
  m_buttonCode = buttonCode;
  m_unicode = 0;
  m_holdTime = holdTime;
}

CAction::CAction(int actionID,
                 unsigned int state,
                 float posX,
                 float posY,
                 float offsetX,
                 float offsetY,
                 float velocityX,
                 float velocityY,
                 const std::string& name)
  : m_name(name)
{
  m_id = actionID;
  m_amount[0] = posX;
  m_amount[1] = posY;
  m_amount[2] = offsetX;
  m_amount[3] = offsetY;
  m_amount[4] = velocityX;
  m_amount[5] = velocityY;
  m_repeat = 0;
  m_buttonCode = 0;
  m_unicode = 0;
  m_holdTime = state;
}

CAction::CAction(int actionID, wchar_t unicode)
{
  m_id = actionID;
  m_repeat = 0;
  m_buttonCode = 0;
  m_unicode = unicode;
  m_holdTime = 0;
}

CAction::CAction(int actionID, const std::string& name, const CKey& key) : m_name(name)
{
  m_id = actionID;
  m_amount[0] = 1; // digital button (could change this for repeat acceleration)
  m_repeat = key.GetRepeat();
  m_buttonCode = key.GetButtonCode();
  m_unicode = key.GetUnicode();
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

CAction::CAction(int actionID, const std::string& name) : m_name(name)
{
  m_id = actionID;
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

void CAction::ClearAmount()
{
  for (float& amount : m_amount)
    amount = 0;
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
  return CActionTranslator::IsAnalog(m_id);
}
