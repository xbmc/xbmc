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
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "EventListener.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "Seat.h"
#include "TimeoutManager.h"
#include "InputFactory.h"

namespace xw = xbmc::wayland;

xbmc::InputFactory::InputFactory(IDllWaylandClient &clientLibrary,
                                 IDllXKBCommon &xkbCommonLibrary,
                                 struct wl_seat *seat,
                                 IEventListener &dispatch,
                                 ITimeoutManager &timeouts) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_pointerProcessor(dispatch, *this),
  m_keyboardProcessor(dispatch, timeouts),
  m_seat(new xw::Seat(clientLibrary, seat, *this))
{
}

void xbmc::InputFactory::SetXBMCSurface(struct wl_surface *s)
{
  m_keyboardProcessor.SetXBMCSurface(s);
}

void xbmc::InputFactory::SetCursor(uint32_t serial,
                             struct wl_surface *surface,
                             double surfaceX,
                             double surfaceY)
{
  m_pointer->SetCursor(serial, surface, surfaceX, surfaceY);
}

bool xbmc::InputFactory::InsertPointer(struct wl_pointer *p)
{
  if (m_pointer.get())
    return false;

  m_pointer.reset(new xw::Pointer(m_clientLibrary,
                                  p,
                                  m_pointerProcessor));
  return true;
}

bool xbmc::InputFactory::InsertKeyboard(struct wl_keyboard *k)
{
  if (m_keyboard.get())
    return false;

  m_keyboard.reset(new xw::Keyboard(m_clientLibrary,
                                    m_xkbCommonLibrary,
                                    k,
                                    m_keyboardProcessor));
  return true;
}

void xbmc::InputFactory::RemovePointer()
{
  m_pointer.reset();
}

void xbmc::InputFactory::RemoveKeyboard()
{
  m_keyboard.reset();
}
