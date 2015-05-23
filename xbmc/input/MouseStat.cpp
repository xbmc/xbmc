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

#include "MouseStat.h"
#include "input/Key.h"
#include "utils/TimeUtils.h"
#include "windowing/WindowingFactory.h"

CMouseStat::CMouseStat()
{
  m_pointerState = MOUSE_STATE_NORMAL;
  SetEnabled();
  m_speedX = m_speedY = 0;
  m_maxX = m_maxY = 0;
  memset(&m_mouseState, 0, sizeof(m_mouseState));
  m_Key = KEY_MOUSE_NOOP;
}

CMouseStat::~CMouseStat()
{
}

void CMouseStat::Initialize()
{
  // Set the default resolution (PAL)
  SetResolution(720, 576, 1, 1);
}

void CMouseStat::HandleEvent(XBMC_Event& newEvent)
{
  // Save the mouse position and the size of the last move
  int dx, dy;
  if (newEvent.type == XBMC_MOUSEMOTION)
  {
    dx = newEvent.motion.x - m_mouseState.x;
    dy = newEvent.motion.y - m_mouseState.y;
  }
  else if (newEvent.type == XBMC_MOUSEBUTTONDOWN || newEvent.type == XBMC_MOUSEBUTTONUP)
  {
    dx = newEvent.button.x - m_mouseState.x;
    dy = newEvent.button.y - m_mouseState.y;
  }
  else
  {
    return;
  }
  m_mouseState.dx = dx;
  m_mouseState.dy = dy;
  m_mouseState.x  = std::max(0, std::min(m_maxX, m_mouseState.x + dx));
  m_mouseState.y  = std::max(0, std::min(m_maxY, m_mouseState.y + dy));

  // Fill in the public members
  if (newEvent.button.type == XBMC_MOUSEBUTTONDOWN)
  {
    if (newEvent.button.button == XBMC_BUTTON_LEFT) m_mouseState.button[MOUSE_LEFT_BUTTON] = true;
    if (newEvent.button.button == XBMC_BUTTON_RIGHT) m_mouseState.button[MOUSE_RIGHT_BUTTON] = true;
    if (newEvent.button.button == XBMC_BUTTON_MIDDLE) m_mouseState.button[MOUSE_MIDDLE_BUTTON] = true;
    if (newEvent.button.button == XBMC_BUTTON_X1) m_mouseState.button[MOUSE_EXTRA_BUTTON1] = true;
    if (newEvent.button.button == XBMC_BUTTON_X2) m_mouseState.button[MOUSE_EXTRA_BUTTON2] = true;
    if (newEvent.button.button == XBMC_BUTTON_X3) m_mouseState.button[MOUSE_EXTRA_BUTTON3] = true;
    if (newEvent.button.button == XBMC_BUTTON_X4) m_mouseState.button[MOUSE_EXTRA_BUTTON4] = true;
    if (newEvent.button.button == XBMC_BUTTON_WHEELUP) m_mouseState.dz = 1;
    if (newEvent.button.button == XBMC_BUTTON_WHEELDOWN) m_mouseState.dz = -1;
  }
  else if (newEvent.button.type == XBMC_MOUSEBUTTONUP)
  {
    if (newEvent.button.button == XBMC_BUTTON_LEFT) m_mouseState.button[MOUSE_LEFT_BUTTON] = false;
    if (newEvent.button.button == XBMC_BUTTON_RIGHT) m_mouseState.button[MOUSE_RIGHT_BUTTON] = false;
    if (newEvent.button.button == XBMC_BUTTON_MIDDLE) m_mouseState.button[MOUSE_MIDDLE_BUTTON] = false;
    if (newEvent.button.button == XBMC_BUTTON_X1) m_mouseState.button[MOUSE_EXTRA_BUTTON1] = false;
    if (newEvent.button.button == XBMC_BUTTON_X2) m_mouseState.button[MOUSE_EXTRA_BUTTON2] = false;
    if (newEvent.button.button == XBMC_BUTTON_X3) m_mouseState.button[MOUSE_EXTRA_BUTTON3] = false;
    if (newEvent.button.button == XBMC_BUTTON_X4) m_mouseState.button[MOUSE_EXTRA_BUTTON4] = false;
    if (newEvent.button.button == XBMC_BUTTON_WHEELUP) m_mouseState.dz = 0;
    if (newEvent.button.button == XBMC_BUTTON_WHEELDOWN) m_mouseState.dz = 0;
  }

  // Now check the current message and the previous state to find out if
  // this is a click, doubleclick, drag etc
  uint32_t now = CTimeUtils::GetFrameTime();
  bool bNothingDown = true;
  
  for (int i = 0; i < MOUSE_MAX_BUTTON; i++)
  {
    bClick[i] = false;
    bLongClick[i] = false;
    bDoubleClick[i] = false;
    bHold[i] = 0;

    // CButtonState::Update does the hard work of checking the button state
    // and spotting drags, doubleclicks etc
    CButtonState::BUTTON_ACTION action = m_buttonState[i].Update(now, m_mouseState.x, m_mouseState.y, m_mouseState.button[i]);
    switch (action)
    {
    case CButtonState::MB_SHORT_CLICK:
      bClick[i] = true;
      bNothingDown = false;
      break;
    case CButtonState::MB_LONG_CLICK:
      bLongClick[i] = true;
      bNothingDown = false;
      break;
    case CButtonState::MB_DOUBLE_CLICK:
      bDoubleClick[i] = true;
      bNothingDown = false;
      break;
    case CButtonState::MB_DRAG_START:
      bHold[i] = CButtonState::MB_DRAG_START;
      bNothingDown = false;
      break;
    case CButtonState::MB_DRAG:
      bHold[i] = CButtonState::MB_DRAG;
      bNothingDown = false;
      break;
    case CButtonState::MB_DRAG_END:
      bHold[i] = CButtonState::MB_DRAG_END;
      bNothingDown = false;
      break;
    default:
      break;
    }
  }

  // Now work out what action ID to send to XBMC.

  // ignore any mouse messages by default
  m_Key = KEY_MOUSE_NOOP;

  for (int button=0; button<MOUSE_MAX_BUTTON; ++button)
  {
    // The bClick array is set true if CButtonState::Update spots a click
    // i.e. a button down followed by a button up.
    if (bClick[button])
      m_Key = KEY_MOUSE_CLICK + button;
    // The bDoubleClick array is set true if CButtonState::Update spots a
    // button down within double_click_time (500ms) of the last click
    else if (bDoubleClick[button])
      m_Key = KEY_MOUSE_DOUBLE_CLICK + button;
    else if (bLongClick[button])
      m_Key = KEY_MOUSE_LONG_CLICK + button;

    if (m_Key != KEY_MOUSE_NOOP)
      break;
  }

  if (m_Key == KEY_MOUSE_NOOP)
  {
    // The bHold array is set to the drag action
    if (bHold[MOUSE_LEFT_BUTTON] != 0)
    {
      switch (bHold[MOUSE_LEFT_BUTTON])
      {
        case CButtonState::MB_DRAG:
          m_Key = KEY_MOUSE_DRAG;
          break;
        case CButtonState::MB_DRAG_START:
          m_Key = KEY_MOUSE_DRAG_START;
          break;
        case CButtonState::MB_DRAG_END:
          m_Key = KEY_MOUSE_DRAG_END;
          break;
      }
    }
    else if (bHold[MOUSE_RIGHT_BUTTON] != 0)
    {
      switch (bHold[MOUSE_RIGHT_BUTTON])
      {
      case CButtonState::MB_DRAG:
        m_Key = KEY_MOUSE_RDRAG;
        break;
      case CButtonState::MB_DRAG_START:
        m_Key = KEY_MOUSE_RDRAG_START;
        break;
      case CButtonState::MB_DRAG_END:
        m_Key = KEY_MOUSE_RDRAG_END;
        break;
      }
    }

    // dz is +1 on wheel up and -1 on wheel down
    else if (m_mouseState.dz > 0)
      m_Key = KEY_MOUSE_WHEEL_UP;
    else if (m_mouseState.dz < 0)
      m_Key = KEY_MOUSE_WHEEL_DOWN;

    // Check for a mouse move that isn't a drag, ignoring messages with no movement at all
    else if (newEvent.type == XBMC_MOUSEMOTION && (m_mouseState.dx || m_mouseState.dy))
      m_Key = KEY_MOUSE_MOVE;
  }

  // activate the mouse pointer if we have an action or the mouse has moved far enough
  if ((MovedPastThreshold() && m_Key == KEY_MOUSE_MOVE) ||
      (m_Key != KEY_MOUSE_NOOP && m_Key != KEY_MOUSE_MOVE))
    SetActive();

  // reset the mouse state if nothing is held down
  if (bNothingDown)
    SetState(MOUSE_STATE_NORMAL);
}

void CMouseStat::SetResolution(int maxX, int maxY, float speedX, float speedY)
{
  m_maxX = maxX;
  m_maxY = maxY;

  // speed is currently unused
  m_speedX = speedX;
  m_speedY = speedY;
}

void CMouseStat::SetActive(bool active /*=true*/)
{
  m_lastActiveTime = CTimeUtils::GetFrameTime();
  m_mouseState.active = active;
  // we show the OS mouse if:
  // 1. The mouse is active (it has been moved) AND
  // 2. The XBMC mouse is disabled in settings AND
  // 3. XBMC is not in fullscreen.
  g_Windowing.ShowOSMouse(m_mouseState.active && !IsEnabled() && !g_Windowing.IsFullScreen());
}

// IsActive - returns true if we have been active in the last MOUSE_ACTIVE_LENGTH period
bool CMouseStat::IsActive()
{
  if (m_mouseState.active && (CTimeUtils::GetFrameTime() - m_lastActiveTime > MOUSE_ACTIVE_LENGTH))
    SetActive(false);
  return (m_mouseState.active && IsEnabled());
}

void CMouseStat::SetEnabled(bool enabled)
{
  m_mouseEnabled = enabled;
  SetActive(enabled);
}

// IsEnabled - returns true if mouse is enabled
bool CMouseStat::IsEnabled() const
{
  return m_mouseEnabled;
}

bool CMouseStat::MovedPastThreshold() const
{
  return (m_mouseState.dx * m_mouseState.dx + m_mouseState.dy * m_mouseState.dy >= MOUSE_MINIMUM_MOVEMENT * MOUSE_MINIMUM_MOVEMENT);
}

uint32_t CMouseStat::GetKey() const
{
  return m_Key;
}

int CMouseStat::GetHold(int ButtonID) const
{
  switch (ButtonID)
  { case MOUSE_LEFT_BUTTON:
      return bHold[MOUSE_LEFT_BUTTON];
  }
  return false;
}

CMouseStat::CButtonState::CButtonState()
{
  m_state = STATE_RELEASED;
  m_time = 0;
  m_x = 0;
  m_y = 0;
}

bool CMouseStat::CButtonState::InClickRange(int x, int y) const
{
  int dx = x - m_x;
  int dy = y - m_y;
  return (unsigned int)(dx*dx + dy*dy) <= click_confines*click_confines;
}

CMouseStat::CButtonState::BUTTON_ACTION CMouseStat::CButtonState::Update(unsigned int time, int x, int y, bool down)
{
  if (m_state == STATE_IN_DRAG)
  {
    if (down)
      return MB_DRAG;
    m_state = STATE_RELEASED;
    return MB_DRAG_END;
  }
  else if (m_state == STATE_RELEASED)
  {
    if (down)
    {
      m_state = STATE_IN_CLICK;
      m_time = time;
      m_x = x;
      m_y = y;
    }
  }
  else if (m_state == STATE_IN_CLICK)
  {
    if (down)
    {
      if (!InClickRange(x,y))
      { // beginning a drag
        m_state = STATE_IN_DRAG;
        return MB_DRAG_START;
      }
    }
    else
    { // button up
      if (time - m_time < short_click_time)
      { // single click
        m_state = STATE_IN_DOUBLE_CLICK;
        m_time = time; // double click time and positioning is measured from the
        m_x = x;       // end of a single click
        m_y = y;
        return MB_SHORT_CLICK;
      }
      else
      { // long click
        m_state = STATE_RELEASED;
        return MB_LONG_CLICK;
      }
    }
  }
  else if (m_state == STATE_IN_DOUBLE_CLICK)
  {
    if (time - m_time > double_click_time || !InClickRange(x,y))
    { // too long, or moved to much - reset to released state and re-update, as we may be starting a new click
      m_state = STATE_RELEASED;
      return Update(time, x, y, down);
    }
    if (down)
    {
      m_state = STATE_IN_DOUBLE_IGNORE;
      return MB_DOUBLE_CLICK;
    }
  }
  else if (m_state == STATE_IN_DOUBLE_IGNORE)
  {
    if (!down)
      m_state = STATE_RELEASED;
  }

  return MB_NONE;
}

