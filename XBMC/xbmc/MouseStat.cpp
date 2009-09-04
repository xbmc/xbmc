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

#include "stdafx.h"
#include "MouseStat.h"

/* elis
#if defined (HAS_SDL)
#include "SDLMouse.h"
#else
#include "DirectInputMouse.h"
#endif
*/

#include "Key.h"
#include "GraphicContext.h"
#include "WindowingFactory.h"

CMouseStat g_Mouse; // global

CMouseStat::CMouseStat()
{
  //m_mouseDevice = NULL;
  m_exclusiveWindowID = WINDOW_INVALID;
  m_exclusiveControlID = WINDOW_INVALID;
  m_pointerState = MOUSE_STATE_NORMAL;
  m_mouseEnabled = true;
  m_speedX = m_speedY = 0;
  m_maxX = m_maxY = 0;
  memset(&m_mouseState, 0, sizeof(m_mouseState));
}

CMouseStat::~CMouseStat()
{
}

void CMouseStat::Initialize(void *appData)
{
#ifdef HAS_SDL_XX 
  //elis
  // save the current cursor so it can be restored
  m_visibleCursor = SDL_GetCursor();

  // create a transparent cursor
  Uint8 data[8];
  Uint8 mask[8];
  memset(data, 0, sizeof(data));
  memset(mask, 0, sizeof(mask));
  m_hiddenCursor = SDL_CreateCursor(data, mask, 8, 8, 0, 0);
  SDL_SetCursor(m_hiddenCursor);
#endif

  // Set the default resolution (PAL)
  SetResolution(720, 576, 1, 1);
  
}

void CMouseStat::Cleanup()
{
#ifdef HAS_SDL_XX
  //elis
  
  SDL_SetCursor(m_visibleCursor);
  if (m_hiddenCursor)
    SDL_FreeCursor(m_hiddenCursor);
#endif
}

void CMouseStat::HandleEvent(XBMC_Event& newEvent)
{
  bool bMouseMoved(false);
  int x=0, y=0;
#ifdef HAS_SDL_XX
  //elis
  
  if (0 == (SDL_GetAppState() & SDL_APPMOUSEFOCUS))
  {
    bMouseMoved = false;
    Update(bMouseMoved);
    return;
  }
#endif
  x = m_mouseState.x - newEvent.motion.x;
  y = m_mouseState.y - newEvent.motion.y;
  m_mouseState.dx = (char)x;
  m_mouseState.dy = (char)y;
  bMouseMoved = x || y ;

  // Check if we have an update...
  if (bMouseMoved)
  {
    m_mouseState.x = newEvent.motion.x;
    if (m_mouseState.x < 0)
      m_mouseState.x = 0;

    m_mouseState.y = newEvent.motion.y;
    if (m_mouseState.y < 0)
      m_mouseState.y = 0;
  }
  else
  {
    m_mouseState.dx = 0;
    m_mouseState.dy = 0;
  }

  // Fill in the public members
  m_mouseState.button[MOUSE_LEFT_BUTTON] = (newEvent.button.button == XBMC_BUTTON_LEFT && newEvent.button.type == XBMC_MOUSEBUTTONDOWN);
  m_mouseState.button[MOUSE_RIGHT_BUTTON] = (newEvent.button.button == XBMC_BUTTON_RIGHT && newEvent.button.type == XBMC_MOUSEBUTTONDOWN);
  m_mouseState.button[MOUSE_MIDDLE_BUTTON] = (newEvent.button.button == XBMC_BUTTON_MIDDLE && newEvent.button.type == XBMC_MOUSEBUTTONDOWN);
  m_mouseState.button[MOUSE_EXTRA_BUTTON1] = (newEvent.button.button == XBMC_BUTTON_X1 && newEvent.button.type == XBMC_MOUSEBUTTONDOWN);
  m_mouseState.button[MOUSE_EXTRA_BUTTON2] = (newEvent.button.button == XBMC_BUTTON_X2 && newEvent.button.type == XBMC_MOUSEBUTTONDOWN);

  UpdateInternal(bMouseMoved);
}


void CMouseStat::UpdateInternal(bool bMouseMoved)
{
  // update our state from the mouse device
  if (bMouseMoved)
  {
    // check our position is not out of bounds
    if (m_mouseState.x < 0) m_mouseState.x = 0;
    if (m_mouseState.y < 0) m_mouseState.y = 0;
    if (m_mouseState.x > m_maxX) m_mouseState.x = m_maxX;
    if (m_mouseState.y > m_maxY) m_mouseState.y = m_maxY;
    if (HasMoved())
    {
      m_mouseState.active = true;
      m_lastActiveTime = timeGetTime();
    }
  }
  else
  {
    m_mouseState.dx = 0;
    m_mouseState.dy = 0;
    m_mouseState.dz = 0;
    // check how long we've been inactive
    if (timeGetTime() - m_lastActiveTime > MOUSE_ACTIVE_LENGTH)
      m_mouseState.active = false;
  }

  // Perform the click mapping (for single + double click detection)
  bool bNothingDown = true;
  for (int i = 0; i < 5; i++)
  {
    bClick[i] = false;
    bDoubleClick[i] = false;
    bHold[i] = false;
    if (m_mouseState.button[i])
    {
      if (!m_mouseState.active) // wake up mouse on any click
      {
        m_mouseState.active = true;
        m_lastActiveTime = timeGetTime();
      }
      bNothingDown = false;
      if (m_lastDown[i])
      { // start of hold
        bHold[i] = true;
      }
      else
      {
        if (timeGetTime() - m_lastClickTime[i] < MOUSE_DOUBLE_CLICK_LENGTH)
        { // Double click
          bDoubleClick[i] = true;
        }
        else
        { // Mouse down
        }
      }
    }
    else
    {
      if (m_lastDown[i])
      { // Mouse up
        bNothingDown = false;
        bClick[i] = true;
        m_lastClickTime[i] = timeGetTime();
      }
      else
      { // no change
      }
    }
    m_lastDown[i] = m_mouseState.button[i];
  }
  if (bNothingDown)
  { // reset mouse pointer
    SetState(MOUSE_STATE_NORMAL);
  }

  // update our mouse pointer as necessary - we show the default pointer
  // only if we don't have the mouse on, and the mouse is active
#ifdef HAS_SDL_XX
  //elis
  
  SDL_SetCursor(m_mouseState.active && !m_mouseEnabled);
#endif
}


void CMouseStat::SetResolution(int maxX, int maxY, float speedX, float speedY)
{
  m_maxX = maxX;
  m_maxY = maxY;

  // speed is currently unused
  m_speedX = speedX;
  m_speedY = speedY;

  // reset the coordinates
  m_mouseState.x = m_maxX / 2;
  m_mouseState.y = m_maxY / 2;
  m_mouseState.active = true;

  SDL_ShowCursor(!(m_mouseEnabled || g_Windowing.IsFullScreen()));
}

// IsActive - returns true if we have been active in the last MOUSE_ACTIVE_LENGTH period
bool CMouseStat::IsActive() const
{
  return m_mouseState.active && m_mouseEnabled;
}

void CMouseStat::SetEnabled(bool enabled)
{
  m_mouseEnabled = enabled;
  m_mouseState.active = enabled;
  SDL_ShowCursor(!(m_mouseEnabled || g_Windowing.IsFullScreen()));
}

// IsEnabled - returns true if mouse is enabled
bool CMouseStat::IsEnabled() const
{
  return m_mouseEnabled;
}

// turns off mouse activation
void CMouseStat::SetInactive()
{
  m_mouseState.active = false;
}

bool CMouseStat::HasMoved() const
{
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
  if (activate)
  {
    m_lastActiveTime = timeGetTime();
    m_mouseState.active = true;
  }
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
  m_mouseState.active = true;
  m_lastActiveTime = timeGetTime();
}

void CMouseStat::SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point)
{
  m_exclusiveControlID = dwControlID;
  m_exclusiveWindowID = dwWindowID;
  // convert posX, posY to screen coords...
  // NOTE: This relies on the window resolution having been set correctly beforehand in CGUIWindow::OnMouseAction()
  CPoint mouseCoords(GetLocation());
  g_graphicsContext.InvertFinalCoords(mouseCoords.x, mouseCoords.y);
  m_exclusiveOffset = point - mouseCoords;
}

void CMouseStat::EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID)
{
  if (m_exclusiveControlID == dwControlID && m_exclusiveWindowID == dwWindowID)
    SetExclusiveAccess(WINDOW_INVALID, WINDOW_INVALID, CPoint(0, 0));
}

void CMouseStat::Acquire()
{
  /* elis
  if (m_mouseDevice)
    m_mouseDevice->Acquire();
    */
}
