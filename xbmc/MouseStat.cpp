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

#include "MouseStat.h"
#include "Key.h"
#include "GraphicContext.h"
#include "WindowingFactory.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

CMouseStat g_Mouse;

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
  return dx*dx + dy*dy <= click_confines*click_confines;
}

CMouseStat::CButtonState::BUTTON_ACTION CMouseStat::CButtonState::Update(unsigned int time, int x, int y, bool down)
{
  if (m_state == STATE_IN_DRAG)
  {
    if (down)
      return MB_DRAG; 
    m_state = STATE_RELEASED;
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
        return MB_DRAG;
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

CMouseStat::CMouseStat()
{
  m_exclusiveWindowID = WINDOW_INVALID;
  m_exclusiveControl = NULL;
  m_pointerState = MOUSE_STATE_NORMAL;
  SetEnabled();
  m_speedX = m_speedY = 0;
  m_maxX = m_maxY = 0;
  memset(&m_mouseState, 0, sizeof(m_mouseState));
}

CMouseStat::~CMouseStat()
{
}

void CMouseStat::Initialize(void *appData)
{
  // Set the default resolution (PAL)
  SetResolution(720, 576, 1, 1);
  
}

void CMouseStat::Cleanup()
{
}

void CMouseStat::HandleEvent(XBMC_Event& newEvent)
{
  int dx = newEvent.motion.x - m_mouseState.x;
  int dy = newEvent.motion.y - m_mouseState.y;
  
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
    if (newEvent.button.button == XBMC_BUTTON_WHEELUP) m_mouseState.dz = 0;
    if (newEvent.button.button == XBMC_BUTTON_WHEELDOWN) m_mouseState.dz = 0;
  }
  UpdateInternal();
}

void CMouseStat::UpdateInternal()
{
  uint32_t now = CTimeUtils::GetFrameTime();
  // update our state from the mouse device
  if (HasMoved() || m_mouseState.dz)
    SetActive();

  // Perform the click mapping (for single, long single, and double click detection)
  bool bNothingDown = true;
  
  for (int i = 0; i < 5; i++)
  {
    bClick[i] = false;
    bDoubleClick[i] = false;
    bHold[i] = false;

    CButtonState::BUTTON_ACTION action = m_buttonState[i].Update(now, m_mouseState.x, m_mouseState.y, m_mouseState.button[i]);
    switch (action)
    {
    case CButtonState::MB_SHORT_CLICK:
    case CButtonState::MB_LONG_CLICK:
      bClick[i] = true;
      bNothingDown = false;
      break;
    case CButtonState::MB_DOUBLE_CLICK:
      bDoubleClick[i] = true;
      bNothingDown = false;
      break;
    case CButtonState::MB_DRAG:
      bHold[i] = true;
      bNothingDown = false;
      break;
    }
  }

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

bool CMouseStat::HasMoved(bool detectAllMoves /* = false */) const
{
  if (detectAllMoves)
    return m_mouseState.dx || m_mouseState.dy;
  return (m_mouseState.dx * m_mouseState.dx + m_mouseState.dy * m_mouseState.dy >= MOUSE_MINIMUM_MOVEMENT * MOUSE_MINIMUM_MOVEMENT);
}

CPoint CMouseStat::GetLocation() const
{
  return CPoint((float)m_mouseState.x, (float)m_mouseState.y);
}

void CMouseStat::SetLocation(const CPoint &point, bool activate)
{
  m_mouseState.x = (int)point.x;
  m_mouseState.y = (int)point.y;
  SetActive();
}

CPoint CMouseStat::GetLastMove() const
{
  return CPoint(m_mouseState.dx, m_mouseState.dy);
}

char CMouseStat::GetWheel() const
{
  return m_mouseState.dz;
}

void CMouseStat::UpdateMouseWheel(char dir)
{
  m_mouseState.dz = dir;
  SetActive();
}

void CMouseStat::SetExclusiveAccess(const CGUIControl *control, int windowID, const CPoint &point)
{
  m_exclusiveControl = control;
  m_exclusiveWindowID = windowID;
  // convert posX, posY to screen coords...
  // NOTE: This relies on the window resolution having been set correctly beforehand in CGUIWindow::OnMouseAction()
  CPoint mouseCoords(GetLocation());
  g_graphicsContext.InvertFinalCoords(mouseCoords.x, mouseCoords.y);
  m_exclusiveOffset = point - mouseCoords;
}

void CMouseStat::EndExclusiveAccess(const CGUIControl *control, int windowID)
{
  if (m_exclusiveControl == control && m_exclusiveWindowID == windowID)
    SetExclusiveAccess(NULL, WINDOW_INVALID, CPoint(0, 0));
}

void CMouseStat::Acquire()
{
}
