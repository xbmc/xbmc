#ifndef MOUSE_STAT_H
#define MOUSE_STAT_H

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

#include "windowing/XBMC_events.h"

#define XBMC_BUTTON(X)		(1 << ((X)-1))
#define XBMC_BUTTON_LEFT		1
#define XBMC_BUTTON_MIDDLE	2
#define XBMC_BUTTON_RIGHT	3
#define XBMC_BUTTON_WHEELUP	4
#define XBMC_BUTTON_WHEELDOWN	5
#define XBMC_BUTTON_X1		6
#define XBMC_BUTTON_X2		7
#define XBMC_BUTTON_X3		8
#define XBMC_BUTTON_X4		9
#define XBMC_BUTTON_LMASK	XBMC_BUTTON(XBMC_BUTTON_LEFT)
#define XBMC_BUTTON_MMASK	XBMC_BUTTON(XBMC_BUTTON_MIDDLE)
#define XBMC_BUTTON_RMASK	XBMC_BUTTON(XBMC_BUTTON_RIGHT)
#define XBMC_BUTTON_X1MASK	XBMC_BUTTON(XBMC_BUTTON_X1)
#define XBMC_BUTTON_X2MASK	XBMC_BUTTON(XBMC_BUTTON_X2)
#define XBMC_BUTTON_X3MASK	XBMC_BUTTON(XBMC_BUTTON_X3)
#define XBMC_BUTTON_X4MASK	XBMC_BUTTON(XBMC_BUTTON_X4)

#define MOUSE_MINIMUM_MOVEMENT 2
#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L

#define MOUSE_MAX_BUTTON 7

enum MOUSE_STATE 
{ 
  MOUSE_STATE_NORMAL = 1, /*! < Normal state */
  MOUSE_STATE_FOCUS,      /*! < Control below the mouse is currently in focus */
  MOUSE_STATE_DRAG,       /*! < A drag operation is being performed */
  MOUSE_STATE_CLICK       /*! < A mousebutton is being clicked */
};

enum MOUSE_BUTTON
{
  MOUSE_LEFT_BUTTON = 0,
  MOUSE_RIGHT_BUTTON,
  MOUSE_MIDDLE_BUTTON,
  MOUSE_EXTRA_BUTTON1,
  MOUSE_EXTRA_BUTTON2,
  MOUSE_EXTRA_BUTTON3,
  MOUSE_EXTRA_BUTTON4
};

// this holds everything we know about the current state of the mouse
struct MouseState
{
  int x;              // x location
  int y;              // y location
  int16_t dx;         // change in x
  int16_t dy;         // change in y
  int8_t dz;          // change in z (wheel)
  bool button[MOUSE_MAX_BUTTON];     // current state of the buttons
  bool active;        // true if the mouse is active
};

struct MousePosition
{
  int x;
  int y;
};

class CAction;

class CMouseStat
{
public:
  CMouseStat();
  virtual ~CMouseStat();

  void Initialize();
  void HandleEvent(XBMC_Event& newEvent);
  void SetResolution(int maxX, int maxY, float speedX, float speedY);
  bool IsActive();
  bool IsEnabled() const;

  void SetActive(bool active = true);
  void SetState(MOUSE_STATE state) { m_pointerState = state; };
  void SetEnabled(bool enabled = true);
  MOUSE_STATE GetState() const { return m_pointerState; };
  uint32_t GetKey() const;

  int GetHold(int ButtonID) const;
  inline int GetX(void) const { return m_mouseState.x; }
  inline int GetY(void) const { return m_mouseState.y; }
  inline int GetDX(void) const { return m_mouseState.dx; }
  inline int GetDY(void) const { return m_mouseState.dy; }
  MousePosition GetPosition() { return MousePosition{ m_mouseState.x, m_mouseState.y }; }

private:
  /*! \brief Holds information regarding a particular mouse button state

   The CButtonState class is used to track where in a button event the mouse currently is.
   There is effectively 5 BUTTON_STATE's available, and transitioning between those states
   is handled by the Update() function.

   The actions we detect are:
    * short clicks - down/up press of the mouse within short_click_time ms, where the pointer stays within click_confines pixels
    * long clicks - down/up press of the mouse greater than short_click_time ms, where the pointers stays within click_confines pixels
    * double clicks - a further down press of the mouse within double_click_time of the up press of a short click, where the pointer stays within click_confines pixels
    * drag - the mouse is down and has been moved more than click_confines pixels

   \sa CMouseStat
  */
  class CButtonState
  {
  public:
    /*! \brief enum for the actions to perform as a result of an Update function
     */
    enum BUTTON_ACTION { MB_NONE = 0,      ///< no action should occur
                         MB_SHORT_CLICK,   ///< a short click has occurred (a double click may be in process)
                         MB_LONG_CLICK,    ///< a long click has occurred
                         MB_DOUBLE_CLICK,  ///< a double click has occurred
                         MB_DRAG_START,    ///< a drag action has started
                         MB_DRAG,          ///< a drag action is in progress
                         MB_DRAG_END };    ///< a drag action has finished

    CButtonState();

    /*! \brief Update the button state, with where the mouse is, and whether the button is down or not

     \param time frame time in ms
     \param x horizontal coordinate of the mouse
     \param y vertical coordinate of the mouse
     \param down true if the button is down
     \return action that should be performed
     */
    BUTTON_ACTION Update(unsigned int time, int x, int y, bool down);
  private:
    static const unsigned int click_confines = 5;        ///< number of pixels that the pointer may move while the button is down to trigger a click
    static const unsigned int short_click_time = 1000;   ///< time for mouse down/up to trigger a short click rather than a long click
    static const unsigned int double_click_time = 500;   ///< time for mouse down following a short click to trigger a double click

    bool InClickRange(int x, int y) const;

    enum BUTTON_STATE { STATE_RELEASED = 0,       ///< mouse button is released, no events pending
                        STATE_IN_CLICK,           ///< mouse button is down, a click is pending
                        STATE_IN_DOUBLE_CLICK,    ///< mouse button is released, pending double click
                        STATE_IN_DOUBLE_IGNORE,   ///< mouse button is down following double click
                        STATE_IN_DRAG };          ///< mouse button is down during a drag

    BUTTON_STATE m_state;
    unsigned int m_time;
    int m_x;
    int m_y;
  };

  /*! \brief detect whether the mouse has moved

  Uses a trigger threshold of 2 pixels to detect mouse movement

  \return whether the mouse has moved past the trigger threshold.
   */
  bool MovedPastThreshold() const;

  // state of the mouse
  MOUSE_STATE m_pointerState;
  MouseState m_mouseState;
  bool m_mouseEnabled;
  CButtonState m_buttonState[MOUSE_MAX_BUTTON];

  int m_maxX;
  int m_maxY;
  float m_speedX;
  float m_speedY;

  // active/click timers
  unsigned int m_lastActiveTime;

  bool bClick[MOUSE_MAX_BUTTON];
  bool bDoubleClick[MOUSE_MAX_BUTTON];
  int  bHold[MOUSE_MAX_BUTTON];
  bool bLongClick[MOUSE_MAX_BUTTON];

  uint32_t m_Key;
};

#endif



