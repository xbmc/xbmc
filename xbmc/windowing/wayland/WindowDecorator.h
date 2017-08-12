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

#include <array>
#include <memory>
#include <set>

#include <wayland-client-protocol.hpp>
#include <wayland-cursor.hpp>

#include "Connection.h"
#include "guilib/Geometry.h"
#include "Registry.h"
#include "ShellSurface.h"
#include "threads/CriticalSection.h"
#include "Util.h"
#include "utils/posix/SharedMemory.h"
#include "WindowDecorationHandler.h"

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
class CWindowDecorator
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

  struct Seat
  {
    wayland::seat_t seat;
    wayland::pointer_t pointer;
    wayland::touch_t touch;

    SurfaceIndex currentSurface{SURFACE_COUNT};
    CPoint pointerPosition;

    std::uint32_t pointerEnterSerial;
    std::string cursorName;
    wayland::surface_t cursor;

    explicit Seat(wayland::seat_t seat)
    : seat{std::move(seat)}
    {}
  };
  std::map<std::uint32_t, Seat> m_seats;

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

  void OnSeatAdded(std::uint32_t name, wayland::proxy_t&& seat);
  void OnSeatRemoved(std::uint32_t name);
  void OnSeatCapabilities(std::uint32_t name, wayland::seat_capability capability);
  void HandleSeatPointer(Seat& seat);
  void HandleSeatTouch(Seat& seat);

  void UpdateSeatCursor(Seat& seat);
  void UpdateButtonHoverState();
  void HandleSeatClick(wayland::seat_t seat, SurfaceIndex surface, std::uint32_t serial, std::uint32_t button, CPoint position);
};

}
}
}
