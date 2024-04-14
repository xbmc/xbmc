/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Seat.h"

#include "utils/log.h"

#include "platform/posix/utils/FileHandle.h"
#include "platform/posix/utils/Mmap.h"

#include <cassert>
#include <utility>

#include <unistd.h>

using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

namespace
{

/**
 * Handle change of availability of a wl_seat input capability
 *
 * This checks whether the capability is currently available with the wl_seat
 * and whether it was bound to a protocol object. If there is a mismatch between
 * these two, the protocol proxy is released if a capability was removed or bound
 * if a capability was added.
 *
 * \param caps new capabilities
 * \param cap capability to check for
 * \param seatName human-readable name of the seat for log messages
 * \param capName human-readable name of the capability for log messages
 * \param proxy proxy object that should be filled with a new instance or reset
 * \param instanceProvider function that functions as factory for the Wayland
 *                         protocol instance if the capability has been added
 */
template<typename T, typename InstanceProviderT>
bool HandleCapabilityChange(const wayland::seat_capability& caps,
                            const wayland::seat_capability& cap,
                            std::string const& seatName,
                            std::string const& capName,
                            T& proxy,
                            InstanceProviderT instanceProvider)
{
  bool hasCapability = caps & cap;

  if ((!!proxy) != hasCapability)
  {
    // Capability changed

    if (hasCapability)
    {
      // The capability was added
      CLog::Log(LOGDEBUG, "Wayland seat {} gained capability {}", seatName, capName);
      proxy = instanceProvider();
      return true;
    }
    else
    {
      // The capability was removed
      CLog::Log(LOGDEBUG, "Wayland seat {} lost capability {}", seatName, capName);
      proxy.proxy_release();
    }
  }

  return false;
}

}

CSeat::CSeat(std::uint32_t globalName, wayland::seat_t const& seat, CConnection& connection)
: m_globalName{globalName}, m_seat{seat}, m_selection{connection, seat}
{
  m_seat.on_name() = [this](std::string name) { m_name = std::move(name); };
  m_seat.on_capabilities() = std::bind(&CSeat::HandleOnCapabilities, this, std::placeholders::_1);
}

CSeat::~CSeat() noexcept = default;

void CSeat::AddRawInputHandlerKeyboard(KODI::WINDOWING::WAYLAND::IRawInputHandlerKeyboard *rawKeyboardHandler)
{
  assert(rawKeyboardHandler);
  m_rawKeyboardHandlers.emplace(rawKeyboardHandler);
}

void CSeat::RemoveRawInputHandlerKeyboard(KODI::WINDOWING::WAYLAND::IRawInputHandlerKeyboard *rawKeyboardHandler)
{
  m_rawKeyboardHandlers.erase(rawKeyboardHandler);
}

void CSeat::AddRawInputHandlerPointer(IRawInputHandlerPointer* rawPointerHandler)
{
  assert(rawPointerHandler);
  m_rawPointerHandlers.emplace(rawPointerHandler);
}

void CSeat::RemoveRawInputHandlerPointer(KODI::WINDOWING::WAYLAND::IRawInputHandlerPointer *rawPointerHandler)
{
  m_rawPointerHandlers.erase(rawPointerHandler);
}

void CSeat::AddRawInputHandlerTouch(IRawInputHandlerTouch* rawTouchHandler)
{
  assert(rawTouchHandler);
  m_rawTouchHandlers.emplace(rawTouchHandler);
}

void CSeat::RemoveRawInputHandlerTouch(KODI::WINDOWING::WAYLAND::IRawInputHandlerTouch *rawTouchHandler)
{
  m_rawTouchHandlers.erase(rawTouchHandler);
}

void CSeat::HandleOnCapabilities(const wayland::seat_capability& caps)
{
  if (HandleCapabilityChange(caps, wayland::seat_capability::keyboard, GetName(), "keyboard", m_keyboard, std::bind(&wayland::seat_t::get_keyboard, m_seat)))
  {
    HandleKeyboardCapability();
  }
  if (HandleCapabilityChange(caps, wayland::seat_capability::pointer, GetName(), "pointer", m_pointer, std::bind(&wayland::seat_t::get_pointer, m_seat)))
  {
    HandlePointerCapability();
  }
  if (HandleCapabilityChange(caps, wayland::seat_capability::touch, GetName(), "touch", m_touch, std::bind(&wayland::seat_t::get_touch, m_seat)))
  {
    HandleTouchCapability();
  }
}

void CSeat::SetCursor(std::uint32_t serial, wayland::surface_t const &surface, std::int32_t hotspotX, std::int32_t hotspotY)
{
  if (m_pointer)
  {
    m_pointer.set_cursor(serial, surface, hotspotX, hotspotY);
  }
}

void CSeat::HandleKeyboardCapability()
{
  m_keyboard.on_keymap() = [this](wayland::keyboard_keymap_format format, int fd, std::uint32_t size)
  {
    KODI::UTILS::POSIX::CFileHandle fdGuard{fd};
    KODI::UTILS::POSIX::CMmap mmap{nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0};
    std::string keymap{static_cast<const char*> (mmap.Data()), size};
    for (auto handler : m_rawKeyboardHandlers)
    {
      handler->OnKeyboardKeymap(this, format, keymap);
    }
  };
  m_keyboard.on_enter() = [this](std::uint32_t serial, const wayland::surface_t& surface,
                                 const wayland::array_t& keys) {
    for (auto handler : m_rawKeyboardHandlers)
    {
      handler->OnKeyboardEnter(this, serial, surface, keys);
    }
  };
  m_keyboard.on_leave() = [this](std::uint32_t serial, const wayland::surface_t& surface) {
    for (auto handler : m_rawKeyboardHandlers)
    {
      handler->OnKeyboardLeave(this, serial, surface);
    }
  };
  m_keyboard.on_key() = [this](std::uint32_t serial, std::uint32_t time, std::uint32_t key, wayland::keyboard_key_state state)
  {
    for (auto handler : m_rawKeyboardHandlers)
    {
      handler->OnKeyboardKey(this, serial, time, key, state);
    }
  };
  m_keyboard.on_modifiers() = [this](std::uint32_t serial, std::uint32_t modsDepressed, std::uint32_t modsLatched, std::uint32_t modsLocked, std::uint32_t group)
  {
    for (auto handler : m_rawKeyboardHandlers)
    {
      handler->OnKeyboardModifiers(this, serial, modsDepressed, modsLatched, modsLocked, group);
    }
  };
  InstallKeyboardRepeatInfo();
}

void CSeat::InstallKeyboardRepeatInfo()
{
  m_keyboard.on_repeat_info() = [this](std::int32_t rate, std::int32_t delay)
  {
    for (auto handler : m_rawKeyboardHandlers)
    {
      handler->OnKeyboardRepeatInfo(this, rate, delay);
    }
  };
}

void CSeat::HandlePointerCapability()
{
  m_pointer.on_enter() = [this](std::uint32_t serial, const wayland::surface_t& surface,
                                double surfaceX, double surfaceY) {
    for (auto handler : m_rawPointerHandlers)
    {
      handler->OnPointerEnter(this, serial, surface, surfaceX, surfaceY);
    }
  };
  m_pointer.on_leave() = [this](std::uint32_t serial, const wayland::surface_t& surface) {
    for (auto handler : m_rawPointerHandlers)
    {
      handler->OnPointerLeave(this, serial, surface);
    }
  };
  m_pointer.on_motion() = [this](std::uint32_t time, double surfaceX, double surfaceY)
  {
    for (auto handler : m_rawPointerHandlers)
    {
      handler->OnPointerMotion(this, time, surfaceX, surfaceY);
    }
  };
  m_pointer.on_button() = [this](std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state)
  {
    for (auto handler : m_rawPointerHandlers)
    {
      handler->OnPointerButton(this, serial, time, button, state);
    }
  };
  m_pointer.on_axis() = [this](std::uint32_t time, wayland::pointer_axis axis, double value)
  {
    for (auto handler : m_rawPointerHandlers)
    {
      handler->OnPointerAxis(this, time, axis, value);
    }
  };
  // Wayland groups pointer events, but right now there is no benefit in
  // treating them in groups. The main use case for doing so seems to be
  // multi-axis (i.e. diagonal) scrolling, but we do not support this anyway.
  /*m_pointer.on_frame() = [this]()
  {

  };*/
}

void CSeat::HandleTouchCapability()
{
  m_touch.on_down() = [this](std::uint32_t serial, std::uint32_t time,
                             const wayland::surface_t& surface, std::int32_t id, double x,
                             double y) {
    for (auto handler : m_rawTouchHandlers)
    {
      handler->OnTouchDown(this, serial, time, surface, id, x, y);
    }
  };
  m_touch.on_up() = [this](std::uint32_t serial, std::uint32_t time, std::int32_t id)
  {
    for (auto handler : m_rawTouchHandlers)
    {
      handler->OnTouchUp(this, serial, time, id);
    }
  };
  m_touch.on_motion() = [this](std::uint32_t time, std::int32_t id, double x, double y)
  {
    for (auto handler : m_rawTouchHandlers)
    {
      handler->OnTouchMotion(this, time, id, x, y);
    }
  };
  m_touch.on_cancel() = [this]()
  {
    for (auto handler : m_rawTouchHandlers)
    {
      handler->OnTouchCancel(this);
    }
  };
  m_touch.on_shape() = [this](std::int32_t id, double major, double minor)
  {
    for (auto handler : m_rawTouchHandlers)
    {
      handler->OnTouchShape(this, id, major, minor);
    }
  };
}
