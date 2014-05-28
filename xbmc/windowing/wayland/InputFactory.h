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
#include <boost/scoped_ptr.hpp>

#ifndef WAYLAND_CURSORMANAGER_H_INCLUDED
#define WAYLAND_CURSORMANAGER_H_INCLUDED
#include "CursorManager.h"
#endif

#ifndef WAYLAND_SEAT_H_INCLUDED
#define WAYLAND_SEAT_H_INCLUDED
#include "Seat.h"
#endif

#ifndef WAYLAND_POINTER_H_INCLUDED
#define WAYLAND_POINTER_H_INCLUDED
#include "Pointer.h"
#endif

#ifndef WAYLAND_POINTERPROCESSOR_H_INCLUDED
#define WAYLAND_POINTERPROCESSOR_H_INCLUDED
#include "PointerProcessor.h"
#endif

#ifndef WAYLAND_KEYBOARD_H_INCLUDED
#define WAYLAND_KEYBOARD_H_INCLUDED
#include "Keyboard.h"
#endif

#ifndef WAYLAND_KEYBOARDPROCESSOR_H_INCLUDED
#define WAYLAND_KEYBOARDPROCESSOR_H_INCLUDED
#include "KeyboardProcessor.h"
#endif


class IDllWaylandClient;
class IDllXKBCommon;

struct wl_keyboard;
struct wl_pointer;
struct wl_seat;
struct wl_surface;

namespace xbmc
{
/* InputFactory is effectively just a manager class that encapsulates
 * all input related information and ties together a wayland seat with
 * the rest of the XBMC input handling subsystem. It is an internal
 * class just for tying together these two ends. */
class InputFactory :
  public wayland::IInputReceiver,
  public ICursorManager
{
public:

  InputFactory(IDllWaylandClient &clientLibrary,
               IDllXKBCommon &xkbCommonLibrary,
               struct wl_seat *seat,
               IEventListener &dispatch,
               ITimeoutManager &timeouts);

  void SetXBMCSurface(struct wl_surface *s);

private:

  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 double surfaceX,
                 double surfaceY);

  bool InsertPointer(struct wl_pointer *);
  bool InsertKeyboard(struct wl_keyboard *);

  void RemovePointer();
  void RemoveKeyboard();

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;

  PointerProcessor m_pointerProcessor;
  KeyboardProcessor m_keyboardProcessor;

  boost::scoped_ptr<wayland::Seat> m_seat;
  boost::scoped_ptr<wayland::Pointer> m_pointer;
  boost::scoped_ptr<wayland::Keyboard> m_keyboard;
};
}
