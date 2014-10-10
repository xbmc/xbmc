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
#include <boost/function.hpp>

#include <wayland-client.h>

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
/* Callback encapsulates a callback object that might be called
 * by the compositor through the client library at an arbitrary point
 * in time. A custom closure can be provided as func to be called
 * whenever this callback is fired
 */
class Callback :
  boost::noncopyable
{
public:

  typedef boost::function<void(uint32_t)> Func;

  Callback(IDllWaylandClient &clientLibrary,
           struct wl_callback *callback,
           const Func &func);
  ~Callback();

  struct wl_callback * GetWlCallback();

  static const struct wl_callback_listener m_listener;

  static void OnCallback(void *,
                         struct wl_callback *,
                         uint32_t);

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_callback *m_callback;
  Func m_func;
};
}
}
