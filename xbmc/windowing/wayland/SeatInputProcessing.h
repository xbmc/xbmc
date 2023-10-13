/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InputProcessorKeyboard.h"
#include "InputProcessorPointer.h"
#include "InputProcessorTouch.h"
#include "Seat.h"

#include <cstdint>
#include <map>
#include <memory>

#include <wayland-client-protocol.hpp>

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
   * \param type input device type that caused the event
   * \param event XBMC event data
   */
  virtual void OnEvent(InputType type, XBMC_Event& event) {}
  /**
   * Handle focus enter
   * \param type input device type for which the surface has gained the focus
   */
  virtual void OnEnter(InputType type) {}
  /**
   * Handle focus leave
   * \param type input device type for which the surface has lost the focus
   */
  virtual void OnLeave(InputType type) {}
  /**
   * Handle request for setting the cursor
   *
   * When the client gains pointer focus for a surface, a cursor image must be
   * attached to the pointer. Otherwise the previous pointer image would
   * be used.
   *
   * This request is sent in addition to \ref OnEnter for \ref InputType::POINTER.
   *
   * \param seatGlobalName numeric Wayland global name of the seat the event occurred on
   * \param pointer pointer instance that needs its cursor set
   * \param serial Wayland protocol message serial that must be sent back in set_cursor
   */
  virtual void OnSetCursor(std::uint32_t seatGlobalName, std::uint32_t serial) {}

  virtual ~IInputHandler() = default;
};

/**
 * Receive events from all registered wl_seats and process them into Kodi events
 *
 * Multi-seat support is not currently implemented completely, but each seat has
 * separate state.
 */
class CSeatInputProcessing final : public IInputHandlerPointer, public IInputHandlerKeyboard
{
public:
  /**
   * Construct a seat input processor
   *
   * \param inputSurface Surface that events should be processed on (all other surfaces are ignored)
   * \param handler Mandatory handler for processed input events
   */
  CSeatInputProcessing(wayland::surface_t const& inputSurface, IInputHandler& handler);
  void AddSeat(CSeat* seat);
  void RemoveSeat(CSeat* seat);

  /**
   * Set the scale the coordinates should be interpreted at
   *
   * Wayland input events are always in surface coordinates, but Kodi only uses
   * buffer coordinates internally. Use this function to set the scaling
   * factor between the two and multiply the surface coordinates accordingly
   * for Kodi events.
   *
   * \param scale new buffer-to-surface pixel ratio
   */
  void SetCoordinateScale(std::int32_t scale);

private:
  wayland::surface_t m_inputSurface;
  IInputHandler& m_handler;

  void OnPointerEnter(std::uint32_t seatGlobalName, std::uint32_t serial) override;
  void OnPointerLeave() override;
  void OnPointerEvent(XBMC_Event& event) override;

  void OnKeyboardEnter() override;
  void OnKeyboardLeave() override;
  void OnKeyboardEvent(XBMC_Event& event) override;

  struct SeatState
  {
    CSeat* seat;
    std::unique_ptr<CInputProcessorKeyboard> keyboardProcessor;
    std::unique_ptr<CInputProcessorPointer> pointerProcessor;
    std::unique_ptr<CInputProcessorTouch> touchProcessor;

    SeatState(CSeat* seat)
    : seat{seat}
    {}
  };
  std::map<std::uint32_t, SeatState> m_seats;
};

}
}
}
