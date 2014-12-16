#pragma once

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
#include "Pointer.h"

struct wl_surface;

namespace xbmc
{
class IEventListener;
class ICursorManager;

/* PointerProcessor implements IPointerReceiver and transforms input
 * wayland mouse event callbacks into XBMC events. It also handles
 * changing the cursor image on surface entry */
class PointerProcessor :
  public wayland::IPointerReceiver
{
public:

  PointerProcessor(IEventListener &,
                   ICursorManager &);

private:

  void Motion(uint32_t time,
              const float &x,
              const float &y);
  void Button(uint32_t serial,
              uint32_t time,
              uint32_t button,
              enum wl_pointer_button_state state);
  void Axis(uint32_t time,
            uint32_t axis,
            float value);
  void Enter(struct wl_surface *surface,
             double surfaceX,
             double surfaceY);

  IEventListener &m_listener;
  ICursorManager &m_cursorManager;

  uint32_t m_currentlyPressedButton;
  float    m_lastPointerX;
  float    m_lastPointerY;

  /* There is no defined export for these buttons -
   * wayland appears to just be using the evdev codes
   * directly */
  static const unsigned int WaylandLeftButton = 272;
  static const unsigned int WaylandRightButton = 273;
  static const unsigned int WaylandMiddleButton = 274;

  static const unsigned int WheelUpButton = 4;
  static const unsigned int WheelDownButton = 5;
  
};
}
