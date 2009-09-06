#ifndef MOUSE_STAT_H
#define MOUSE_STAT_H

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

#include "XBMC_events.h"
#include "Geometry.h"

#define XBMC_BUTTON(X)		(1 << ((X)-1))
#define XBMC_BUTTON_LEFT		1
#define XBMC_BUTTON_MIDDLE	2
#define XBMC_BUTTON_RIGHT	3
#define XBMC_BUTTON_WHEELUP	4
#define XBMC_BUTTON_WHEELDOWN	5
#define XBMC_BUTTON_X1		6
#define XBMC_BUTTON_X2		7
#define XBMC_BUTTON_LMASK	XBMC_BUTTON(XBMC_BUTTON_LEFT)
#define XBMC_BUTTON_MMASK	XBMC_BUTTON(XBMC_BUTTON_MIDDLE)
#define XBMC_BUTTON_RMASK	XBMC_BUTTON(XBMC_BUTTON_RIGHT)
#define XBMC_BUTTON_X1MASK	XBMC_BUTTON(XBMC_BUTTON_X1)
#define XBMC_BUTTON_X2MASK	XBMC_BUTTON(XBMC_BUTTON_X2)

#define MOUSE_MINIMUM_MOVEMENT 2
#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L

enum MOUSE_STATE { MOUSE_STATE_NORMAL = 1, MOUSE_STATE_FOCUS, MOUSE_STATE_DRAG, MOUSE_STATE_CLICK };
enum MOUSE_BUTTON { MOUSE_LEFT_BUTTON = 0, MOUSE_RIGHT_BUTTON, MOUSE_MIDDLE_BUTTON, MOUSE_EXTRA_BUTTON1, MOUSE_EXTRA_BUTTON2 };

struct MouseState
{
  int x;            // x location
  int y;            // y location
  int16_t dx;          // change in x
  int16_t dy;          // change in y
  char dz;          // change in z (wheel)
  bool button[5];   // true if a button is down
  bool active;      // true if the mouse is active
};

class CMouseStat
{
public:

  CMouseStat();
  virtual ~CMouseStat();

  void Initialize(void *appData = NULL);
  void Cleanup();
  void HandleEvent(XBMC_Event& newEvent);
  void Acquire();
  void SetResolution(int maxX, int maxY, float speedX, float speedY);
  bool IsActive();
  bool IsEnabled() const;
  bool HasMoved(bool allMoves = false) const;
  void SetActive(bool active = true);
  void SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point);
  void EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID);
  DWORD GetExclusiveWindowID() const { return m_exclusiveWindowID; };
  DWORD GetExclusiveControlID() const { return m_exclusiveControlID; };
  const CPoint &GetExclusiveOffset() const { return m_exclusiveOffset; };
  void SetState(DWORD state) { m_pointerState = state; };
  void SetEnabled(bool enabled = true);
  DWORD GetState() const { return m_pointerState; };
  CPoint GetLocation() const;
  void SetLocation(const CPoint &point, bool activate=false);
  CPoint GetLastMove() const;
  char GetWheel() const;
  void UpdateMouseWheel(char dir);
  void ResetMouseWheel();
  void Update(XBMC_Event& newEvent);

private:
 
  void UpdateInternal();
 
  // exclusive access to mouse from a control
  DWORD m_exclusiveWindowID;
  DWORD m_exclusiveControlID;
  CPoint m_exclusiveOffset;

  // state of the mouse
  DWORD m_pointerState;
  MouseState m_mouseState;
  bool m_mouseEnabled;
  bool m_lastDown[5];

  // mouse device
  // elis IMouseDevice *m_mouseDevice;

  // mouse limits and speed
  int m_maxX;
  int m_maxY;
  float m_speedX;
  float m_speedY;

  // active/click timers
  DWORD m_lastActiveTime;
  DWORD m_lastClickTime[5];

#ifdef HAS_SDL_XX
  SDL_Cursor *m_visibleCursor;
  SDL_Cursor *m_hiddenCursor;
#endif

public:
  // public access variables to button clicks etc.
  bool bClick[5];
  bool bDoubleClick[5];
  bool bHold[5];
};

extern CMouseStat g_Mouse;

#endif



