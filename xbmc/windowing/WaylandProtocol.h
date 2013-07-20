#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <iomanip>
#include <stdexcept>
#include <sstream>

#include "DllWaylandClient.h"

namespace xbmc
{
namespace wayland
{
namespace protocol
{
template <typename Object>
void CallMethodOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               uint32_t opcode)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_marshaller()(proxy,
                                      opcode);
}
template <typename Object,
          typename A1>
void CallMethodOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               uint32_t opcode,
                               A1 arg1)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_marshaller()(proxy,
                                      opcode,
                                      arg1);
}

template <typename Object,
          typename A1,
          typename A2>
void CallMethodOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               uint32_t opcode,
                               A1 arg1,
                               A2 arg2)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_marshaller()(proxy,
                                      opcode,
                                      arg1,
                                      arg2);
}

template <typename Object,
          typename A1,
          typename A2,
          typename A3>
void CallMethodOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               uint32_t opcode,
                               A1 arg1,
                               A2 arg2,
                               A3 arg3)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_marshaller()(proxy,
                                      opcode,
                                      arg1,
                                      arg2,
                                      arg3);
}

template <typename Object,
          typename A1,
          typename A2,
          typename A3,
          typename A4>
void CallMethodOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               uint32_t opcode,
                               A1 arg1,
                               A2 arg2,
                               A3 arg3,
                               A4 arg4)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_marshaller()(proxy,
                                      opcode,
                                      arg1,
                                      arg2,
                                      arg3,
                                      arg4);
}

template <typename Object,
          typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename A5>
void CallMethodOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               uint32_t opcode,
                               A1 arg1,
                               A2 arg2,
                               A3 arg3,
                               A4 arg4,
                               A5 arg5)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_marshaller()(proxy,
                                      opcode,
                                      arg1,
                                      arg2,
                                      arg3,
                                      arg4,
                                      arg5);
}

template <typename Create, typename Factory>
Create CreateWaylandObject(IDllWaylandClient &clientLibrary,
                           Factory factory,
                           struct wl_interface **interface)
{
  struct wl_proxy *pfactory =
    reinterpret_cast<struct wl_proxy *>(factory);
  struct wl_proxy *proxy =
    clientLibrary.wl_proxy_create(pfactory,
                                  reinterpret_cast<struct wl_interface *>(interface));

  if (!proxy)
  {
    std::stringstream ss;
    ss << "Failed to create "
       << typeid(Create).name()
       << " from factory "
       << typeid(Factory).name()
       << " at 0x"
       << std::hex
       << reinterpret_cast<void *>(pfactory)
       << std::dec;
    throw std::runtime_error(ss.str());
  }

  return reinterpret_cast<Create>(proxy);
}

template <typename Object, typename Listener>
int AddListenerOnWaylandObject(IDllWaylandClient &clientLibrary,
                               Object object,
                               Listener listener,
                               void *data)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  
  /* C-style casts are bad, but there is no equavilent to
   * std::remove_const in C++98 and we are reinterpret_cast'ing
   * anyways */
  IDllWaylandClient::wl_proxy_listener_func *listenerFunc =
    (IDllWaylandClient::wl_proxy_listener_func *)((void *)listener);
  return clientLibrary.wl_proxy_add_listener(proxy, listenerFunc, data);
}

template <typename Object>
void DestroyWaylandObject(IDllWaylandClient &clientLibrary,
                          Object *object)
{
  struct wl_proxy *proxy =
    reinterpret_cast<struct wl_proxy *>(object);
  clientLibrary.wl_proxy_destroy(proxy);
}
}
}
}
