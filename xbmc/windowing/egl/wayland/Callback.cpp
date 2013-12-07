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
#include <wayland-client.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Callback.h"

namespace xw = xbmc::wayland;

const wl_callback_listener xw::Callback::m_listener =
{
  Callback::OnCallback
};

xw::Callback::Callback(IDllWaylandClient &clientLibrary,
                       struct wl_callback *callback,
                       const Func &func) :
  m_clientLibrary(clientLibrary),
  m_callback(callback),
  m_func(func)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_callback,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Callback::~Callback()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_callback);
}

struct wl_callback *
xw::Callback::GetWlCallback()
{
  return m_callback;
}

void
xw::Callback::OnCallback(void *data,
                         struct wl_callback *callback,
                         uint32_t time)
{
  static_cast<Callback *>(data)->m_func(time);
}
