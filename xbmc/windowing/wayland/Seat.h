#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <boost/noncopyable.hpp>

#include <wayland-client.h>

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
class IInputReceiver
{
public:

  virtual ~IInputReceiver() {}

  virtual bool InsertPointer(struct wl_pointer *pointer) = 0;
  virtual bool InsertKeyboard(struct wl_keyboard *keyboard) = 0;

  virtual void RemovePointer() = 0;
  virtual void RemoveKeyboard() = 0;
};

class Seat :
  public boost::noncopyable
{
public:

  Seat(IDllWaylandClient &,
       struct wl_seat *,
       IInputReceiver &);
  ~Seat();

  struct wl_seat * GetWlSeat();

  static void HandleCapabilitiesCallback(void *,
                                         struct wl_seat *,
                                         uint32_t);

private:

  static const struct wl_seat_listener m_listener;

  void HandleCapabilities(enum wl_seat_capability);

  IDllWaylandClient &m_clientLibrary;
  struct wl_seat * m_seat;
  IInputReceiver &m_input;

  enum wl_seat_capability m_currentCapabilities;
};
}
}
