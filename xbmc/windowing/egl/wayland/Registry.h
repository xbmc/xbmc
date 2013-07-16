#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include <string>

#include <boost/noncopyable.hpp>

#include <wayland-client.h>

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
class IWaylandRegistration
{
public:

  virtual ~IWaylandRegistration() {};

  virtual bool OnCompositorAvailable(struct wl_compositor *) = 0;
  virtual bool OnShellAvailable(struct wl_shell *) = 0;
  virtual bool OnSeatAvailable(struct wl_seat *) = 0;
  virtual bool OnOutputAvailable(struct wl_output *) = 0;
};

class Registry :
  boost::noncopyable
{
public:

  Registry(IDllWaylandClient &clientLibrary,
           struct wl_display   *display,
           IWaylandRegistration &registration);
  ~Registry();

  struct wl_registry * GetWlRegistry();

  static void HandleGlobalCallback(void *, struct wl_registry *,
                                   uint32_t, const char *, uint32_t);
  static void HandleRemoveGlobalCallback(void *, struct wl_registry *,
                                         uint32_t name);

private:

  static const struct wl_registry_listener m_listener;

  IDllWaylandClient &m_clientLibrary;
  struct wl_registry *m_registry;
  IWaylandRegistration &m_registration;

  void HandleGlobal(uint32_t, const char *, uint32_t);
  void HandleRemoveGlobal(uint32_t);
};
}
}
