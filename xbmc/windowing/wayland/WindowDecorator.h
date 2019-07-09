/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Connection.h"
#include "Registry.h"
#include "Seat.h"
#include "ShellSurface.h"
#include "Util.h"
#include "WindowDecorationHandler.h"
#include "threads/CriticalSection.h"
#include "utils/Geometry.h"

#include "platform/posix/utils/SharedMemory.h"

#include <array>
#include <memory>
#include <set>

#include <wayland-client-protocol.hpp>
#include <wayland-cursor.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

enum SurfaceIndex
{
  SURFACE_TOP = 0,
  SURFACE_RIGHT,
  SURFACE_BOTTOM,
  SURFACE_LEFT,
  SURFACE_COUNT
};

/**
 * Paint decorations around windows with subcompositing
 *
 * With Wayland, applications are responsible for drawing borders on their windows
 * themselves (client-side decorations). To keep the impact on the overall architecture
 * low, the Wayland platform implementation uses this very simple renderer to
 * build ugly but fully functional decorations around the Kodi window. Since Kodi as a
 * media center is usually not used in windowed mode anyway (except maybe for
 * development), the impact of the visually not-so-pleasing decorations should
 * be limited. Nice decorations would require more effort and external libraries for
 * proper 2D drawing.
 *
 * If more platforms decide that client-side decorations would be a good idea to
 * implement, the decorations could also be integrated with the Kodi skin system.
 * Then this class could be removed.
 *
 * The decorations are positioned around the main surface automatically.
 */
class CWindowDecorator final : IRawInputHandlerTouch, IRawInputHandlerPointer
{
public:
  /**
   * Construct window decorator
   * \param handler handler for window decoration events
   * \param connection connection to get Wayland globals
   * \param mainSurface main surface that decorations are constructed around
   * \param windowSize full size of the window including decorations
   * \param scale scale to use for buffers
   * \param state surface state for adjusting decoration appearance
   */
  CWindowDecorator(IWindowDecorationHandler& handler, CConnection& connection, wayland::surface_t const& mainSurface);

  /**
   * Set decoration state and size by providing full surface size including decorations
   *
   * Calculates size of the main surface from size of all surfaces combined (including
   * window decorations) by subtracting the decoration size
   *
   * Decorations will be disabled if state includes STATE_FULLSCREEN
   *
   * Call only from main thread
   */
  void SetState(CSizeInt size, int scale, IShellSurface::StateBitset state);
  /**
   * Get calculated size of main surface
   */
  CSizeInt GetMainSurfaceSize() const
  {
    return m_mainSurfaceSize;
  }
  /**
   * Get full geometry of the window, including decorations if active
   */
  CRectInt GetWindowGeometry() const;
  /**
   * Calculate size of main surface given the size of the full area
   * including decorations and a state
   */
  CSizeInt CalculateMainSurfaceSize(CSizeInt size, IShellSurface::StateBitset state) const;
  /**
   * Calculate size of full surface including decorations given the size of the
   * main surface and a state
   */
  CSizeInt CalculateFullSurfaceSize(CSizeInt mainSurfaceSize, IShellSurface::StateBitset state) const;

  bool IsDecorationActive() const;

  void AddSeat(CSeat* seat);
  void RemoveSeat(CSeat* seat);

  struct Buffer
  {
    void* data{};
    std::size_t dataSize{};
    CSizeInt size{};
    wayland::buffer_t wlBuffer;

    Buffer() noexcept {}

    Buffer(void* data, std::size_t dataSize, CSizeInt size, wayland::buffer_t&& buffer)
    : data{data}, dataSize{dataSize}, size{size}, wlBuffer{std::move(buffer)}
    {}

    std::uint32_t* RgbaBuffer()
    {
      return static_cast<std::uint32_t*> (data);
    }
  };

  struct Surface
  {
    wayland::surface_t wlSurface;
    Buffer buffer;
    CSizeInt size;
    int scale{1};
  };

private:
  CWindowDecorator(CWindowDecorator const& other) = delete;
  CWindowDecorator& operator=(CWindowDecorator const& other) = delete;

  // IRawInputHandlerTouch
  void OnTouchDown(CSeat* seat, std::uint32_t serial, std::uint32_t time, wayland::surface_t surface, std::int32_t id, double x, double y) override;
  // IRawInputHandlerPointer
  void OnPointerEnter(CSeat* seat, std::uint32_t serial, wayland::surface_t surface, double surfaceX, double surfaceY) override;
  void OnPointerLeave(CSeat* seat, std::uint32_t serial, wayland::surface_t surface) override;
  void OnPointerMotion(CSeat* seat, std::uint32_t time, double surfaceX, double surfaceY) override;
  void OnPointerButton(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state) override;

  void Reset(bool reallocate);

  // These functions should not be called directly as they may leave internal
  // structures in an inconsistent state when called individually - only call
  // Reset().
  void ResetButtons();
  void ResetSurfaces();
  void ResetShm();
  void ReattachSubsurfaces();
  void PositionButtons();
  void AllocateBuffers();
  void Repaint();
  void CommitAllBuffers();

  bool StateHasWindowDecorations(IShellSurface::StateBitset state) const;

  Buffer GetBuffer(CSizeInt size);

  IWindowDecorationHandler& m_handler;

  CSizeInt m_mainSurfaceSize;
  int m_scale{1};
  IShellSurface::StateBitset m_windowState;

  CRegistry m_registry;
  wayland::shm_t m_shm;
  wayland::shm_pool_t m_shmPool;
  wayland::compositor_t m_compositor;
  wayland::subcompositor_t m_subcompositor;
  wayland::surface_t m_mainSurface;

  std::unique_ptr<KODI::UTILS::POSIX::CSharedMemory> m_memory;
  std::size_t m_memoryAllocatedSize{};

  struct BorderSurface
  {
    Surface surface;
    wayland::subsurface_t subsurface;
    CRectInt geometry;
    /// Region of the surface that should count as being part of the window
    CRectInt windowRect;
  };
  BorderSurface MakeBorderSurface();

  /**
   * Mutex for all surface/button state that is accessed from the event pump thread via seat events
   * and the main thread:
   * m_surfaces, m_buttons, m_windowSize, m_cursorTheme
   *
   * If size etc. is changing, mutex should be locked for the entire duration of the
   * change until the internal state (surface, button size etc.) is consistent again.
   */
  CCriticalSection m_mutex;

  std::array<BorderSurface, 4> m_borderSurfaces;

  std::set<wayland::buffer_t, WaylandCPtrCompare> m_pendingBuffers;
  CCriticalSection m_pendingBuffersMutex;

  struct SeatState
  {
    CSeat* seat;
    SurfaceIndex currentSurface{SURFACE_COUNT};
    CPoint pointerPosition;

    std::uint32_t pointerEnterSerial{};
    std::string cursorName;
    wayland::surface_t cursor;

    explicit SeatState(CSeat* seat)
    : seat{seat}
    {}
  };
  std::map<std::uint32_t, SeatState> m_seats;

  struct Button
  {
    CRectInt position;
    bool hovered{};
    std::function<void(Surface&, CRectInt, bool /* hover */)> draw;
    std::function<void()> onClick;
  };
  std::vector<Button> m_buttons;

  wayland::cursor_theme_t m_cursorTheme;
  std::uint32_t m_buttonColor;

  void LoadCursorTheme();

  void UpdateSeatCursor(SeatState& seatState);
  void UpdateButtonHoverState();
  void HandleSeatClick(SeatState const& seatState, SurfaceIndex surface, std::uint32_t serial, std::uint32_t button, CPoint position);
};

}
}
}
