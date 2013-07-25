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
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Keyboard.h"

namespace xw = xbmc::wayland;

const struct wl_keyboard_listener xw::Keyboard::m_listener =
{
  Keyboard::HandleKeymapCallback,
  Keyboard::HandleEnterCallback,
  Keyboard::HandleLeaveCallback,
  Keyboard::HandleKeyCallback,
  Keyboard::HandleModifiersCallback
};

xw::Keyboard::Keyboard(IDllWaylandClient &clientLibrary,
                       struct wl_keyboard *keyboard,
                       IKeyboardReceiver &receiver) :
  m_clientLibrary(clientLibrary),
  m_keyboard(keyboard),
  m_reciever(receiver)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_keyboard,
                                       &m_listener,
                                       this);
}

xw::Keyboard::~Keyboard()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_keyboard);
}

void xw::Keyboard::HandleKeymapCallback(void *data,
                                        struct wl_keyboard *keyboard,
                                        uint32_t format,
                                        int fd,
                                        uint32_t size)
{
  static_cast <Keyboard *>(data)->HandleKeymap(format,
                                               fd,
                                               size);
}

void xw::Keyboard::HandleEnterCallback(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t serial,
                                       struct wl_surface *surface,
                                       struct wl_array *keys)
{
  static_cast<Keyboard *>(data)->HandleEnter(serial,
                                             surface,
                                             keys);
}

void xw::Keyboard::HandleLeaveCallback(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t serial,
                                       struct wl_surface *surface)
{
  static_cast<Keyboard *>(data)->HandleLeave(serial,
                                             surface);
}

void xw::Keyboard::HandleKeyCallback(void *data,
                                     struct wl_keyboard *keyboard,
                                     uint32_t serial,
                                     uint32_t time,
                                     uint32_t key,
                                     uint32_t state)
{
  static_cast<Keyboard *>(data)->HandleKey(serial,
                                           time,
                                           key,
                                           state);
}

void xw::Keyboard::HandleModifiersCallback(void *data,
                                           struct wl_keyboard *keyboard,
                                           uint32_t serial,
                                           uint32_t mods_depressed,
                                           uint32_t mods_latched,
                                           uint32_t mods_locked,
                                           uint32_t group)
{
  static_cast<Keyboard *>(data)->HandleModifiers(serial,
                                                 mods_depressed,
                                                 mods_latched,
                                                 mods_locked,
                                                 group);
}

void xw::Keyboard::HandleKeymap(uint32_t format,
                                int fd,
                                uint32_t size)
{
  m_reciever.UpdateKeymap(format, fd, size);
}

void xw::Keyboard::HandleEnter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  m_reciever.Enter(serial, surface, keys);
}

void xw::Keyboard::HandleLeave(uint32_t serial,
                               struct wl_surface *surface)
{
  m_reciever.Leave(serial, surface);
}

void xw::Keyboard::HandleKey(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             uint32_t state)
{
  m_reciever.Key(serial,
                 time,
                 key,
                 static_cast<enum wl_keyboard_key_state>(state));
}

void xw::Keyboard::HandleModifiers(uint32_t serial,
                                   uint32_t mods_depressed,
                                   uint32_t mods_latched,
                                   uint32_t mods_locked,
                                   uint32_t group)
{
  m_reciever.Modifier(serial,
                      mods_depressed,
                      mods_latched,
                      mods_locked,
                      group);
}
