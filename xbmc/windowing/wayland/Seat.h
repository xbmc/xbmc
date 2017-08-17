/*
 *      Copyright (C) 2017 Team XBMC
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
#pragma once

#include <map>
#include <memory>

#include <wayland-client-protocol.hpp>

#include "input/touch/ITouchInputHandler.h"
#include "InputProcessorPointer.h"
#include "InputProcessorKeyboard.h"
#include "InputProcessorTouch.h"
#include "SeatSelection.h"
#include "threads/Timer.h"
#include "windowing/XBMC_events.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

enum class InputType
{
  POINTER,
  KEYBOARD,
  TOUCH
};

/**
 * Handler interface for input events from \ref CSeatInputProcessor
 */
class IInputHandler
{
public:
  /**
   * Handle input event
   * \param seatGlobalName numeric Wayland global name of the seat the event occured on
   * \param type input device type that caused the event
   * \param event XBMC event data
   */
  virtual void OnEvent(std::uint32_t seatGlobalName, InputType type, XBMC_Event& event) {}
  /**
   * Handle focus enter
   * \param seatGlobalName numeric Wayland global name of the seat the event occured on
   * \param type input device type for which the surface has gained the focus
   */
  virtual void OnEnter(std::uint32_t seatGlobalName, InputType type) {}
  /**
   * Handle focus leave
   * \param seatGlobalName numeric Wayland global name of the seat the event occured on
   * \param type input device type for which the surface has lost the focus
   */
  virtual void OnLeave(std::uint32_t seatGlobalName, InputType type) {}
  /**
   * Handle request for setting the cursor
   *
   * When the client gains pointer focus for a surface, a cursor image must be
   * attached to the pointer. Otherwise the previous pointer image would
   * be used.
   *
   * This request is sent in addition to \ref OnEnter for \ref InputType::POINTER.
   *
   * \param pointer pointer instance that needs its cursor set
   * \param serial Wayland protocol message serial that must be sent back in set_cursor
   */
  virtual void OnSetCursor(wayland::pointer_t& pointer, std::uint32_t serial) {}

  virtual ~IInputHandler() = default;
};

/**
 * Handle all wl_seat-related events and process them into Kodi events
 */
class CSeat : IInputHandlerPointer, IInputHandlerKeyboard
{
public:
  /**
   * Construct seat handler
   * \param globalName Wayland numeric global name of the seat
   * \param seat bound seat_t instance
   * \param inputSurface surface that receives the input, used for matching
   *                     pointer focus enter/leave events
   * \param connection connection for retrieving additional globals
   * \param handler handler that receives events from this seat, must not be null
   */
  CSeat(std::uint32_t globalName, wayland::seat_t const& seat, wayland::surface_t const& inputSurface, CConnection& connection, IInputHandler& handler);
  ~CSeat() noexcept;

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
  void SetCoordinateScale(std::int32_t scale);

private:
  CSeat(CSeat const& other) = delete;
  CSeat& operator=(CSeat const& other) = delete;

  void HandleOnCapabilities(wayland::seat_capability caps);
  void HandlePointerCapability(wayland::pointer_t const& pointer);
  void HandleKeyboardCapability(wayland::keyboard_t const& keyboard);
  void HandleTouchCapability(wayland::touch_t const& touch);

  void OnKeyboardEnter() override;
  void OnKeyboardLeave() override;
  void OnKeyboardEvent(XBMC_Event& event) override;

  void OnPointerEnter(wayland::pointer_t& pointer, std::uint32_t serial) override;
  void OnPointerLeave() override;
  void OnPointerEvent(XBMC_Event& event) override;

  void UpdateCoordinateScale();

  std::uint32_t m_globalName;
  wayland::seat_t m_seat;
  wayland::surface_t m_inputSurface;
  std::string m_name{"<unknown>"};
  std::int32_t m_coordinateScale{1};

  IInputHandler& m_handler;

  std::unique_ptr<CInputProcessorPointer> m_pointer;
  std::unique_ptr<CInputProcessorKeyboard> m_keyboard;
  std::unique_ptr<CInputProcessorTouch> m_touch;
  CSeatSelection m_selection;
};

}
}
}
