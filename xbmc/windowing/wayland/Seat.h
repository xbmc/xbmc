/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SeatSelection.h"

#include <cstdint>
#include <set>

#include <wayland-client-protocol.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CSeat;

/**
 * Handler for raw wl_keyboard events
 *
 * All functions are identical to wl_keyboard, except for the keymap which is
 * retrieved from its fd and put into a string
 */
class IRawInputHandlerKeyboard
{
public:
  virtual void OnKeyboardKeymap(CSeat* seat, wayland::keyboard_keymap_format format, std::string const& keymap) {}
  virtual void OnKeyboardEnter(CSeat* seat,
                               std::uint32_t serial,
                               const wayland::surface_t& surface,
                               const wayland::array_t& keys)
  {
  }
  virtual void OnKeyboardLeave(CSeat* seat, std::uint32_t serial, const wayland::surface_t& surface)
  {
  }
  virtual void OnKeyboardKey(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t key, wayland::keyboard_key_state state) {}
  virtual void OnKeyboardModifiers(CSeat* seat, std::uint32_t serial, std::uint32_t modsDepressed, std::uint32_t modsLatched, std::uint32_t modsLocked, std::uint32_t group) {}
  virtual void OnKeyboardRepeatInfo(CSeat* seat, std::int32_t rate, std::int32_t delay) {}
protected:
  ~IRawInputHandlerKeyboard() = default;
};

/**
 * Handler for raw wl_pointer events
 *
 * All functions are identical to wl_pointer
 */
class IRawInputHandlerPointer
{
public:
  virtual void OnPointerEnter(CSeat* seat,
                              std::uint32_t serial,
                              const wayland::surface_t& surface,
                              double surfaceX,
                              double surfaceY)
  {
  }
  virtual void OnPointerLeave(CSeat* seat, std::uint32_t serial, const wayland::surface_t& surface)
  {
  }
  virtual void OnPointerMotion(CSeat* seat, std::uint32_t time, double surfaceX, double surfaceY) {}
  virtual void OnPointerButton(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state) {}
  virtual void OnPointerAxis(CSeat* seat, std::uint32_t time, wayland::pointer_axis axis, double value) {}
protected:
  ~IRawInputHandlerPointer() = default;
};

/**
 * Handler for raw wl_touch events
 *
 * All functions are identical to wl_touch
 */
class IRawInputHandlerTouch
{
public:
  virtual void OnTouchDown(CSeat* seat,
                           std::uint32_t serial,
                           std::uint32_t time,
                           const wayland::surface_t& surface,
                           std::int32_t id,
                           double x,
                           double y)
  {
  }
  virtual void OnTouchUp(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::int32_t id) {}
  virtual void OnTouchMotion(CSeat* seat, std::uint32_t time, std::int32_t id, double x, double y) {}
  virtual void OnTouchCancel(CSeat* seat) {}
  virtual void OnTouchShape(CSeat* seat, std::int32_t id, double major, double minor) {}
protected:
  ~IRawInputHandlerTouch() = default;
};

/**
 * Handle all events and requests related to one seat (including input and selection)
 *
 * The primary purpose of this class is to act as entry point of Wayland events into
 * the Kodi world and distribute them further as necessary.
 * Input events are forwarded to (potentially multiple) handlers. As the Wayland
 * protocol is not very specific on having multiple wl_seat/wl_pointer instances
 * and how they interact, having one central instance and then handling everything
 * in Kodi with multiple handlers is better than each handler having its own
 * protocol object instance.
 */
class CSeat
{
public:
  /**
   * Construct seat handler
   * \param globalName Wayland numeric global name of the seat
   * \param seat bound seat_t instance
   * \param connection connection for retrieving additional globals
   */
  CSeat(std::uint32_t globalName, wayland::seat_t const& seat, CConnection& connection);
  virtual ~CSeat() noexcept;

  void AddRawInputHandlerKeyboard(IRawInputHandlerKeyboard* rawKeyboardHandler);
  void RemoveRawInputHandlerKeyboard(IRawInputHandlerKeyboard* rawKeyboardHandler);
  void AddRawInputHandlerPointer(IRawInputHandlerPointer* rawPointerHandler);
  void RemoveRawInputHandlerPointer(IRawInputHandlerPointer* rawPointerHandler);
  void AddRawInputHandlerTouch(IRawInputHandlerTouch* rawTouchHandler);
  void RemoveRawInputHandlerTouch(IRawInputHandlerTouch* rawTouchHandler);

  std::uint32_t GetGlobalName() const
  {
    return m_globalName;
  }
  std::string const& GetName() const
  {
    return m_name;
  }
  bool HasPointerCapability() const
  {
    return !!m_pointer;
  }
  bool HasKeyboardCapability() const
  {
    return !!m_keyboard;
  }
  bool HasTouchCapability() const
  {
    return !!m_touch;
  }
  std::string GetSelectionText() const
  {
    return m_selection.GetSelectionText();
  }
  /**
   * Get the wl_seat underlying this seat
   *
   * The wl_seat should only be used when strictly necessary, e.g. when
   * starting a move operation with shell interfaces.
   * It may not be used to derive further wl_pointer etc. instances.
   */
  wayland::seat_t const& GetWlSeat()
  {
    return m_seat;
  }

  /**
   * Set the cursor of the pointer of this seat
   *
   * Parameters are identical wo wl_pointer.set_cursor().
   * If the seat does not currently have the pointer capability, this is a no-op.
   */
  virtual void SetCursor(std::uint32_t serial,
                         wayland::surface_t const& surface,
                         std::int32_t hotspotX,
                         std::int32_t hotspotY);

protected:
  virtual void InstallKeyboardRepeatInfo();

private:
  CSeat(CSeat const& other) = delete;
  CSeat& operator=(CSeat const& other) = delete;

  void HandleOnCapabilities(const wayland::seat_capability& caps);
  void HandlePointerCapability();
  void HandleKeyboardCapability();
  void HandleTouchCapability();

  std::uint32_t m_globalName;
  std::string m_name{"<unknown>"};

  wayland::seat_t m_seat;
  wayland::pointer_t m_pointer;
  wayland::keyboard_t m_keyboard;
  wayland::touch_t m_touch;

  std::set<IRawInputHandlerKeyboard*> m_rawKeyboardHandlers;
  std::set<IRawInputHandlerPointer*> m_rawPointerHandlers;
  std::set<IRawInputHandlerTouch*> m_rawTouchHandlers;

  CSeatSelection m_selection;
};

}
}
}
