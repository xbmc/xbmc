/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowDecorator.h"

#include "Util.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <linux/input.h>

using namespace KODI::UTILS::POSIX;
using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

namespace
{

/// Bytes per pixel in shm storage
constexpr int BYTES_PER_PIXEL{4};
/// Width of the visible border around the whole window
constexpr int VISIBLE_BORDER_WIDTH{5};
/// Width of the invisible border around the whole window for easier resizing
constexpr int RESIZE_BORDER_WIDTH{10};
/// Total width of the border around the window
constexpr int BORDER_WIDTH{VISIBLE_BORDER_WIDTH + RESIZE_BORDER_WIDTH};
/// Height of the top bar
constexpr int TOP_BAR_HEIGHT{33};
/// Maximum distance from the window corner to consider position valid for resize
constexpr int RESIZE_MAX_CORNER_DISTANCE{BORDER_WIDTH};
/// Distance of buttons from edges of the top bar
constexpr int BUTTONS_EDGE_DISTANCE{6};
/// Distance from button inner edge to button content
constexpr int BUTTON_INNER_SEPARATION{4};
/// Button size
constexpr int BUTTON_SIZE{21};

constexpr std::uint32_t TRANSPARENT{0x00000000u};
constexpr std::uint32_t BORDER_COLOR{0xFF000000u};
constexpr std::uint32_t BUTTON_COLOR_ACTIVE{0xFFFFFFFFu};
constexpr std::uint32_t BUTTON_COLOR_INACTIVE{0xFF777777u};
constexpr std::uint32_t BUTTON_HOVER_COLOR{0xFF555555u};

static_assert(BUTTON_SIZE <= TOP_BAR_HEIGHT - BUTTONS_EDGE_DISTANCE * 2, "Buttons must fit in top bar");

/*
 * Decorations consist of four surfaces, one for each edge of the window. It would
 * also be possible to position one single large surface behind the main surface
 * instead, but that would waste a lot of memory on big/high-density screens.
 *
 * The four surfaces are laid out as follows: Top and bottom surfaces go over the
 * whole width of the main surface plus the left and right borders.
 * Left and right surfaces only go over the height of the main surface without
 * the top and bottom borders.
 *
 * ---------------------------------------------
 * |                   TOP                     |
 * ---------------------------------------------
 * |   |                                   |   |
 * | L |                                   | R |
 * | E |                                   | I |
 * | F |           Main surface            | G |
 * | T |                                   | H |
 * |   |                                   | T |
 * |   |                                   |   |
 * ---------------------------------------------
 * |                 BOTTOM                    |
 * ---------------------------------------------
 */


CRectInt SurfaceGeometry(SurfaceIndex type, CSizeInt mainSurfaceSize)
{
  // Coordinates are relative to main surface
  switch (type)
  {
    case SURFACE_TOP:
      return {
        CPointInt{-BORDER_WIDTH, -(BORDER_WIDTH + TOP_BAR_HEIGHT)},
        CSizeInt{mainSurfaceSize.Width() + 2 * BORDER_WIDTH, TOP_BAR_HEIGHT + BORDER_WIDTH}
      };
    case SURFACE_RIGHT:
      return {
        CPointInt{mainSurfaceSize.Width(), 0},
        CSizeInt{BORDER_WIDTH, mainSurfaceSize.Height()}
      };
    case SURFACE_BOTTOM:
      return {
        CPointInt{-BORDER_WIDTH, mainSurfaceSize.Height()},
        CSizeInt{mainSurfaceSize.Width() + 2 * BORDER_WIDTH, BORDER_WIDTH}
      };
    case SURFACE_LEFT:
      return {
        CPointInt{-BORDER_WIDTH, 0},
        CSizeInt{BORDER_WIDTH, mainSurfaceSize.Height()}
      };
    default:
      throw std::logic_error("Invalid surface type");
  }
}

CRectInt SurfaceOpaqueRegion(SurfaceIndex type, CSizeInt mainSurfaceSize)
{
  // Coordinates are relative to main surface
  auto size = SurfaceGeometry(type, mainSurfaceSize).ToSize();
  switch (type)
  {
    case SURFACE_TOP:
      return {
        CPointInt{RESIZE_BORDER_WIDTH, RESIZE_BORDER_WIDTH},
        size - CSizeInt{RESIZE_BORDER_WIDTH * 2, RESIZE_BORDER_WIDTH}
      };
    case SURFACE_RIGHT:
      return {
        CPointInt{},
        size - CSizeInt{RESIZE_BORDER_WIDTH, 0}
      };
    case SURFACE_BOTTOM:
      return {
        CPointInt{RESIZE_BORDER_WIDTH, 0},
        size - CSizeInt{RESIZE_BORDER_WIDTH * 2, RESIZE_BORDER_WIDTH}
      };
    case SURFACE_LEFT:
      return {
        CPointInt{RESIZE_BORDER_WIDTH, 0},
        size - CSizeInt{RESIZE_BORDER_WIDTH, 0}
      };
    default:
      throw std::logic_error("Invalid surface type");
  }
}

CRectInt SurfaceWindowRegion(SurfaceIndex type, CSizeInt mainSurfaceSize)
{
  return SurfaceOpaqueRegion(type, mainSurfaceSize);
}

/**
 * Full size of decorations to be added to the main surface size
 */
CSizeInt DecorationSize()
{
  return {2 * VISIBLE_BORDER_WIDTH, 2 * VISIBLE_BORDER_WIDTH + TOP_BAR_HEIGHT};
}

std::size_t MemoryBytesForSize(CSizeInt windowSurfaceSize, int scale)
{
  std::size_t size{};

  for (auto surface : { SURFACE_TOP, SURFACE_RIGHT, SURFACE_BOTTOM, SURFACE_LEFT })
  {
    size += SurfaceGeometry(surface, windowSurfaceSize).Area();
  }

  size *= scale * scale;

  size *= BYTES_PER_PIXEL;

  return size;
}

std::size_t PositionInBuffer(CWindowDecorator::Buffer& buffer, CPointInt position)
{
  if (position.x < 0 || position.y < 0)
  {
    throw std::invalid_argument("Position out of bounds");
  }
  std::size_t offset{static_cast<std::size_t> (buffer.size.Width() * position.y + position.x)};
  if (offset * BYTES_PER_PIXEL >= buffer.dataSize)
  {
    throw std::invalid_argument("Position out of bounds");
  }
  return offset;
}

void DrawHorizontalLine(CWindowDecorator::Buffer& buffer, std::uint32_t color, CPointInt position, int length)
{
  auto offsetStart = PositionInBuffer(buffer, position);
  auto offsetEnd = PositionInBuffer(buffer, position + CPointInt{length - 1, 0});
  if (offsetEnd < offsetStart)
  {
    throw std::invalid_argument("Invalid drawing coordinates");
  }
  std::fill(buffer.RgbaBuffer() + offsetStart, buffer.RgbaBuffer() + offsetEnd + 1, Endian_SwapLE32(color));
}

void DrawLineWithStride(CWindowDecorator::Buffer& buffer, std::uint32_t color, CPointInt position, int length, int stride)
{
  auto offsetStart = PositionInBuffer(buffer, position);
  auto offsetEnd = offsetStart + stride * (length - 1);
  if (offsetEnd * BYTES_PER_PIXEL >= buffer.dataSize)
  {
    throw std::invalid_argument("Position out of bounds");
  }
  if (offsetEnd < offsetStart)
  {
    throw std::invalid_argument("Invalid drawing coordinates");
  }
  auto* memory = buffer.RgbaBuffer();
  for (std::size_t offset = offsetStart; offset <= offsetEnd; offset += stride)
  {
    *(memory + offset) = Endian_SwapLE32(color);
  }
}

void DrawVerticalLine(CWindowDecorator::Buffer& buffer, std::uint32_t color, CPointInt position, int length)
{
  DrawLineWithStride(buffer, color, position, length, buffer.size.Width());
}

/**
 * Draw rectangle inside the specified buffer coordinates
 */
void DrawRectangle(CWindowDecorator::Buffer& buffer, std::uint32_t color, CRectInt rect)
{
  DrawHorizontalLine(buffer, color, rect.P1(), rect.Width());
  DrawVerticalLine(buffer, color, rect.P1(), rect.Height());
  DrawHorizontalLine(buffer, color, rect.P1() + CPointInt{1, rect.Height() - 1}, rect.Width() - 1);
  DrawVerticalLine(buffer, color, rect.P1() + CPointInt{rect.Width() - 1, 1}, rect.Height() - 1);
}

void FillRectangle(CWindowDecorator::Buffer& buffer, std::uint32_t color, CRectInt rect)
{
  for (int y{rect.y1}; y <= rect.y2; y++)
  {
    DrawHorizontalLine(buffer, color, {rect.x1, y}, rect.Width() + 1);
  }
}

void DrawHorizontalLine(CWindowDecorator::Surface& surface, std::uint32_t color, CPointInt position, int length)
{
  for (int i{0}; i < surface.scale; i++)
  {
    DrawHorizontalLine(surface.buffer, color, position * surface.scale + CPointInt{0, i}, length * surface.scale);
  }
}

void DrawAngledLine(CWindowDecorator::Surface& surface, std::uint32_t color, CPointInt position, int length, int stride)
{
  for (int i{0}; i < surface.scale; i++)
  {
    DrawLineWithStride(surface.buffer, color, position * surface.scale + CPointInt{i, 0}, length * surface.scale, surface.buffer.size.Width() + stride);
  }
}

void DrawVerticalLine(CWindowDecorator::Surface& surface, std::uint32_t color, CPointInt position, int length)
{
  DrawAngledLine(surface, color, position, length, 0);
}

/**
 * Draw rectangle inside the specified surface coordinates
 */
void DrawRectangle(CWindowDecorator::Surface& surface, std::uint32_t color, CRectInt rect)
{
  for (int i{0}; i < surface.scale; i++)
  {
    DrawRectangle(surface.buffer, color, {rect.P1() * surface.scale + CPointInt{i, i}, (rect.P2() + CPointInt{1, 1}) * surface.scale - CPointInt{i, i} - CPointInt{1, 1}});
  }
}

void FillRectangle(CWindowDecorator::Surface& surface, std::uint32_t color, CRectInt rect)
{
  FillRectangle(surface.buffer, color, {rect.P1() * surface.scale, (rect.P2() + CPointInt{1, 1}) * surface.scale - CPointInt{1, 1}});
}

void DrawButton(CWindowDecorator::Surface& surface, std::uint32_t lineColor, CRectInt rect, bool hover)
{
  if (hover)
  {
    FillRectangle(surface, BUTTON_HOVER_COLOR, rect);
  }
  DrawRectangle(surface, lineColor, rect);
}

wayland::shell_surface_resize ResizeEdgeForPosition(SurfaceIndex surface, CSizeInt surfaceSize, CPointInt position)
{
  switch (surface)
  {
    case SURFACE_TOP:
      if (position.y <= RESIZE_MAX_CORNER_DISTANCE)
      {
        if (position.x <= RESIZE_MAX_CORNER_DISTANCE)
        {
          return wayland::shell_surface_resize::top_left;
        }
        else if (position.x >= surfaceSize.Width() - RESIZE_MAX_CORNER_DISTANCE)
        {
          return wayland::shell_surface_resize::top_right;
        }
        else
        {
          return wayland::shell_surface_resize::top;
        }
      }
      else
      {
        if (position.x <= RESIZE_MAX_CORNER_DISTANCE)
        {
          return wayland::shell_surface_resize::left;
        }
        else if (position.x >= surfaceSize.Width() - RESIZE_MAX_CORNER_DISTANCE)
        {
          return wayland::shell_surface_resize::right;
        }
        else
        {
          // Inside title bar, not resizing
          return wayland::shell_surface_resize::none;
        }
      }
    case SURFACE_RIGHT:
      if (position.y >= surfaceSize.Height() - RESIZE_MAX_CORNER_DISTANCE)
      {
        return wayland::shell_surface_resize::bottom_right;
      }
      else
      {
        return wayland::shell_surface_resize::right;
      }
    case SURFACE_BOTTOM:
      if (position.x <= RESIZE_MAX_CORNER_DISTANCE)
      {
        return wayland::shell_surface_resize::bottom_left;
      }
      else if (position.x >= surfaceSize.Width() - RESIZE_MAX_CORNER_DISTANCE)
      {
        return wayland::shell_surface_resize::bottom_right;
      }
      else
      {
        return wayland::shell_surface_resize::bottom;
      }
    case SURFACE_LEFT:
      if (position.y >= surfaceSize.Height() - RESIZE_MAX_CORNER_DISTANCE)
      {
        return wayland::shell_surface_resize::bottom_left;
      }
      else
      {
        return wayland::shell_surface_resize::left;
      }

    default:
      return wayland::shell_surface_resize::none;
  }
}

/**
 * Get name for resize cursor according to xdg cursor-spec
 */
std::string CursorForResizeEdge(wayland::shell_surface_resize edge)
{
  if (edge == wayland::shell_surface_resize::top)
    return "n-resize";
  else if (edge == wayland::shell_surface_resize::bottom)
    return "s-resize";
  else if (edge == wayland::shell_surface_resize::left)
    return "w-resize";
  else if (edge == wayland::shell_surface_resize::top_left)
    return "nw-resize";
  else if (edge == wayland::shell_surface_resize::bottom_left)
    return "sw-resize";
  else if (edge == wayland::shell_surface_resize::right)
    return "e-resize";
  else if (edge == wayland::shell_surface_resize::top_right)
    return "ne-resize";
  else if (edge == wayland::shell_surface_resize::bottom_right)
    return "se-resize";
  else
    return "";
}

template<typename T, typename InstanceProviderT>
bool HandleCapabilityChange(const wayland::seat_capability& caps,
                            const wayland::seat_capability& cap,
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
      proxy = instanceProvider();
      return true;
    }
    else
    {
      // The capability was removed
      proxy.proxy_release();
    }
  }

  return false;
}

}

CWindowDecorator::CWindowDecorator(IWindowDecorationHandler& handler, CConnection& connection, wayland::surface_t const& mainSurface)
: m_handler{handler}, m_registry{connection}, m_mainSurface{mainSurface}, m_buttonColor{BUTTON_COLOR_ACTIVE}
{
  static_assert(std::tuple_size<decltype(m_borderSurfaces)>::value == SURFACE_COUNT, "SURFACE_COUNT must match surfaces array size");

  m_registry.RequestSingleton(m_compositor, 1, 4);
  m_registry.RequestSingleton(m_subcompositor, 1, 1, false);
  m_registry.RequestSingleton(m_shm, 1, 1);

  m_registry.Bind();
}

void CWindowDecorator::AddSeat(CSeat* seat)
{
  m_seats.emplace(std::piecewise_construct, std::forward_as_tuple(seat->GetGlobalName()), std::forward_as_tuple(seat));
  seat->AddRawInputHandlerTouch(this);
  seat->AddRawInputHandlerPointer(this);
}

void CWindowDecorator::RemoveSeat(CSeat* seat)
{
  seat->RemoveRawInputHandlerTouch(this);
  seat->RemoveRawInputHandlerPointer(this);
  m_seats.erase(seat->GetGlobalName());
  UpdateButtonHoverState();
}

void CWindowDecorator::OnPointerEnter(CSeat* seat,
                                      std::uint32_t serial,
                                      const wayland::surface_t& surface,
                                      double surfaceX,
                                      double surfaceY)
{
  auto seatStateI = m_seats.find(seat->GetGlobalName());
  if (seatStateI == m_seats.end())
  {
    return;
  }
  auto& seatState = seatStateI->second;
  // Reset first so we ignore events for surfaces we don't handle
  seatState.currentSurface = SURFACE_COUNT;
  std::unique_lock<CCriticalSection> lock(m_mutex);
  for (std::size_t i{0}; i < m_borderSurfaces.size(); i++)
  {
    if (m_borderSurfaces[i].surface.wlSurface == surface)
    {
      seatState.pointerEnterSerial = serial;
      seatState.currentSurface = static_cast<SurfaceIndex> (i);
      seatState.pointerPosition = {static_cast<float> (surfaceX), static_cast<float> (surfaceY)};
      UpdateSeatCursor(seatState);
      UpdateButtonHoverState();
      break;
    }
  }
}

void CWindowDecorator::OnPointerLeave(CSeat* seat,
                                      std::uint32_t serial,
                                      const wayland::surface_t& surface)
{
  auto seatStateI = m_seats.find(seat->GetGlobalName());
  if (seatStateI == m_seats.end())
  {
    return;
  }
  auto& seatState = seatStateI->second;
  seatState.currentSurface = SURFACE_COUNT;
  // Recreate cursor surface on reenter
  seatState.cursorName.clear();
  seatState.cursor.proxy_release();
  UpdateButtonHoverState();
}

void CWindowDecorator::OnPointerMotion(CSeat* seat, std::uint32_t time, double surfaceX, double surfaceY)
{
  auto seatStateI = m_seats.find(seat->GetGlobalName());
  if (seatStateI == m_seats.end())
  {
    return;
  }
  auto& seatState = seatStateI->second;
  if (seatState.currentSurface != SURFACE_COUNT)
  {
    seatState.pointerPosition = {static_cast<float> (surfaceX), static_cast<float> (surfaceY)};
    UpdateSeatCursor(seatState);
    UpdateButtonHoverState();
  }
}

void CWindowDecorator::OnPointerButton(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state)
{
  auto seatStateI = m_seats.find(seat->GetGlobalName());
  if (seatStateI == m_seats.end())
  {
    return;
  }
  const auto& seatState = seatStateI->second;
  if (seatState.currentSurface != SURFACE_COUNT && state == wayland::pointer_button_state::pressed)
  {
    HandleSeatClick(seatState, seatState.currentSurface, serial, button, seatState.pointerPosition);
  }
}

void CWindowDecorator::OnTouchDown(CSeat* seat,
                                   std::uint32_t serial,
                                   std::uint32_t time,
                                   const wayland::surface_t& surface,
                                   std::int32_t id,
                                   double x,
                                   double y)
{
  auto seatStateI = m_seats.find(seat->GetGlobalName());
  if (seatStateI == m_seats.end())
  {
    return;
  }
  auto& seatState = seatStateI->second;
  std::unique_lock<CCriticalSection> lock(m_mutex);
  for (std::size_t i{0}; i < m_borderSurfaces.size(); i++)
  {
    if (m_borderSurfaces[i].surface.wlSurface == surface)
    {
      HandleSeatClick(seatState, static_cast<SurfaceIndex> (i), serial, BTN_LEFT, {static_cast<float> (x), static_cast<float> (y)});
    }
  }
}

void CWindowDecorator::UpdateSeatCursor(SeatState& seatState)
{
  if (seatState.currentSurface == SURFACE_COUNT)
  {
    // Don't set anything if not on any surface
    return;
  }

  LoadCursorTheme();

  std::string cursorName{"default"};

  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    auto resizeEdge = ResizeEdgeForPosition(seatState.currentSurface, SurfaceGeometry(seatState.currentSurface, m_mainSurfaceSize).ToSize(), CPointInt{seatState.pointerPosition});
    if (resizeEdge != wayland::shell_surface_resize::none)
    {
      cursorName = CursorForResizeEdge(resizeEdge);
    }
  }

  if (cursorName == seatState.cursorName)
  {
    // Don't reload cursor all the time when nothing is changing
    return;
  }
  seatState.cursorName = cursorName;

  wayland::cursor_t cursor;
  try
  {
    cursor = CCursorUtil::LoadFromTheme(m_cursorTheme, cursorName);
  }
  catch (std::exception const& e)
  {
    CLog::LogF(LOGERROR, "Could not get required cursor {} from cursor theme: {}", cursorName,
               e.what());
    return;
  }
  auto cursorImage = cursor.image(0);

  if (!seatState.cursor)
  {
    seatState.cursor = m_compositor.create_surface();
  }
  int calcScale{seatState.cursor.can_set_buffer_scale() ? m_scale : 1};

  seatState.seat->SetCursor(seatState.pointerEnterSerial, seatState.cursor, cursorImage.hotspot_x() / calcScale, cursorImage.hotspot_y() / calcScale);
  seatState.cursor.attach(cursorImage.get_buffer(), 0, 0);
  seatState.cursor.damage(0, 0, cursorImage.width() / calcScale, cursorImage.height() / calcScale);
  if (seatState.cursor.can_set_buffer_scale())
  {
    seatState.cursor.set_buffer_scale(m_scale);
  }
  seatState.cursor.commit();
}

void CWindowDecorator::UpdateButtonHoverState()
{
  std::vector<CPoint> pointerPositions;

  std::unique_lock<CCriticalSection> lock(m_mutex);

  for (auto const& seatPair : m_seats)
  {
    auto const& seat = seatPair.second;
    if (seat.currentSurface == SURFACE_TOP)
    {
      pointerPositions.emplace_back(seat.pointerPosition);
    }
  }

  bool changed{false};
  for (auto& button : m_buttons)
  {
    bool wasHovered{button.hovered};
    button.hovered = std::any_of(pointerPositions.cbegin(), pointerPositions.cend(), [&](CPoint point) { return button.position.PtInRect(CPointInt{point}); });
    changed = changed || (button.hovered != wasHovered);
  }

  if (changed)
  {
    // Repaint!
    Reset(false);
  }
}

void CWindowDecorator::HandleSeatClick(SeatState const& seatState, SurfaceIndex surface, std::uint32_t serial, std::uint32_t button, CPoint position)
{
  switch (button)
  {
    case BTN_LEFT:
    {
      std::unique_lock<CCriticalSection> lock(m_mutex);
      auto resizeEdge = ResizeEdgeForPosition(surface, SurfaceGeometry(surface, m_mainSurfaceSize).ToSize(), CPointInt{position});
      if (resizeEdge == wayland::shell_surface_resize::none)
      {
        for (auto const& button : m_buttons)
        {
          if (button.position.PtInRect(CPointInt{position}))
          {
            button.onClick();
            return;
          }
        }

        m_handler.OnWindowMove(seatState.seat->GetWlSeat(), serial);
      }
      else
      {
        m_handler.OnWindowResize(seatState.seat->GetWlSeat(), serial, resizeEdge);
      }
    }
    break;
    case BTN_RIGHT:
      if (surface == SURFACE_TOP)
      {
        m_handler.OnWindowShowContextMenu(seatState.seat->GetWlSeat(), serial, CPointInt{position} - CPointInt{BORDER_WIDTH, BORDER_WIDTH + TOP_BAR_HEIGHT});
      }
      break;
  }
}

CWindowDecorator::BorderSurface CWindowDecorator::MakeBorderSurface()
{
  Surface surface;
  surface.wlSurface = m_compositor.create_surface();
  auto subsurface = m_subcompositor.get_subsurface(surface.wlSurface, m_mainSurface);

  CWindowDecorator::BorderSurface boarderSurface;
  boarderSurface.surface = surface;
  boarderSurface.subsurface = subsurface;

  return boarderSurface;
}

bool CWindowDecorator::IsDecorationActive() const
{
  return StateHasWindowDecorations(m_windowState);
}

bool CWindowDecorator::StateHasWindowDecorations(IShellSurface::StateBitset state) const
{
  // No decorations possible if subcompositor not available
  return m_subcompositor && !state.test(IShellSurface::STATE_FULLSCREEN);
}

CSizeInt CWindowDecorator::CalculateMainSurfaceSize(CSizeInt size, IShellSurface::StateBitset state) const
{
  if (StateHasWindowDecorations(state))
  {
    // Subtract decorations
    return size - DecorationSize();
  }
  else
  {
    // Fullscreen -> no decorations
    return size;
  }
}

CSizeInt CWindowDecorator::CalculateFullSurfaceSize(CSizeInt size, IShellSurface::StateBitset state) const
{
  if (StateHasWindowDecorations(state))
  {
    // Add decorations
    return size + DecorationSize();
  }
  else
  {
    // Fullscreen -> no decorations
    return size;
  }
}

void CWindowDecorator::SetState(CSizeInt size, int scale, IShellSurface::StateBitset state)
{
  CSizeInt mainSurfaceSize{CalculateMainSurfaceSize(size, state)};
  std::unique_lock<CCriticalSection> lock(m_mutex);
  if (mainSurfaceSize == m_mainSurfaceSize && scale == m_scale && state == m_windowState)
  {
    return;
  }

  bool wasDecorations{IsDecorationActive()};
  m_windowState = state;

  m_buttonColor = m_windowState.test(IShellSurface::STATE_ACTIVATED) ? BUTTON_COLOR_ACTIVE : BUTTON_COLOR_INACTIVE;

  CLog::Log(LOGDEBUG,
            "CWindowDecorator::SetState: Setting full surface size {}x{} scale {} (main surface "
            "size {}x{}), decorations active: {}",
            size.Width(), size.Height(), scale, mainSurfaceSize.Width(), mainSurfaceSize.Height(),
            IsDecorationActive());

  if (mainSurfaceSize != m_mainSurfaceSize || scale != m_scale || wasDecorations != IsDecorationActive())
  {
    if (scale != m_scale)
    {
      // Reload cursor theme
      CLog::Log(LOGDEBUG, "CWindowDecorator::SetState: Buffer scale changed, reloading cursor theme");
      m_cursorTheme = wayland::cursor_theme_t{};
      for (auto& seat : m_seats)
      {
        UpdateSeatCursor(seat.second);
      }
    }

    m_mainSurfaceSize = mainSurfaceSize;
    m_scale = scale;
    CLog::Log(LOGDEBUG, "CWindowDecorator::SetState: Resetting decorations");
    Reset(true);
  }
  else if (IsDecorationActive())
  {
    CLog::Log(LOGDEBUG, "CWindowDecorator::SetState: Repainting decorations");
    // Only state differs, no reallocation needed
    Reset(false);
  }
}

void CWindowDecorator::Reset(bool reallocate)
{
  // The complete reset operation should be seen as one atomic update to the
  // internal state, otherwise buffer/surface state might be mismatched
  std::unique_lock<CCriticalSection> lock(m_mutex);

  if (reallocate)
  {
    ResetButtons();
    ResetSurfaces();
    ResetShm();
    if (IsDecorationActive())
    {
      AllocateBuffers();
      ReattachSubsurfaces();
      PositionButtons();
    }
  }

  if (IsDecorationActive())
  {
    Repaint();
  }
}

void CWindowDecorator::ResetButtons()
{
  if (IsDecorationActive())
  {
    if (m_buttons.empty())
    {
      // Minimize
      m_buttons.emplace_back();
      Button& minimize = m_buttons.back();
      minimize.draw = [this](Surface& surface, CRectInt position, bool hover)
      {
        DrawButton(surface, m_buttonColor, position, hover);
        DrawHorizontalLine(surface, m_buttonColor, position.P1() + CPointInt{BUTTON_INNER_SEPARATION, position.Height() - BUTTON_INNER_SEPARATION - 1}, position.Width() - 2 * BUTTON_INNER_SEPARATION);
      };
      minimize.onClick = [this] { m_handler.OnWindowMinimize(); };

      // Maximize
      m_buttons.emplace_back();
      Button& maximize = m_buttons.back();
      maximize.draw = [this](Surface& surface, CRectInt position, bool hover)
      {
        DrawButton(surface, m_buttonColor, position, hover);
        DrawRectangle(surface, m_buttonColor, {position.P1() + CPointInt{BUTTON_INNER_SEPARATION, BUTTON_INNER_SEPARATION}, position.P2() - CPointInt{BUTTON_INNER_SEPARATION, BUTTON_INNER_SEPARATION}});
        DrawHorizontalLine(surface, m_buttonColor, position.P1() + CPointInt{BUTTON_INNER_SEPARATION, BUTTON_INNER_SEPARATION + 1}, position.Width() - 2 * BUTTON_INNER_SEPARATION);
      };
      maximize.onClick = [this] { m_handler.OnWindowMaximize(); };

      // Close
      m_buttons.emplace_back();
      Button& close = m_buttons.back();
      close.draw = [this](Surface& surface, CRectInt position, bool hover)
      {
        DrawButton(surface, m_buttonColor, position, hover);
        auto height = position.Height() - 2 * BUTTON_INNER_SEPARATION;
        DrawAngledLine(surface, m_buttonColor, position.P1() + CPointInt{BUTTON_INNER_SEPARATION, BUTTON_INNER_SEPARATION}, height, 1);
        DrawAngledLine(surface, m_buttonColor, position.P1() + CPointInt{position.Width() - BUTTON_INNER_SEPARATION - 1, BUTTON_INNER_SEPARATION}, height, -1);
      };
      close.onClick = [this] { m_handler.OnWindowClose(); };
    }
  }
  else
  {
    m_buttons.clear();
  }
}

void CWindowDecorator::ResetSurfaces()
{
  if (IsDecorationActive())
  {
    if (!m_borderSurfaces.front().surface.wlSurface)
    {
      std::generate(m_borderSurfaces.begin(), m_borderSurfaces.end(), std::bind(&CWindowDecorator::MakeBorderSurface, this));
    }
  }
  else
  {
    for (auto& borderSurface : m_borderSurfaces)
    {
      auto& wlSurface = borderSurface.surface.wlSurface;
      if (wlSurface)
      {
        // Destroying the surface would cause some flicker because it takes effect
        // immediately, before the next commit on the main surface - just make it
        // invisible by attaching a NULL buffer
        wlSurface.attach(wayland::buffer_t{}, 0, 0);
        wlSurface.set_opaque_region(wayland::region_t{});
        wlSurface.commit();
      }
    }
  }
}

void CWindowDecorator::ReattachSubsurfaces()
{
  for (auto& surface : m_borderSurfaces)
  {
    surface.subsurface.set_position(surface.geometry.x1, surface.geometry.y1);
  }
}

CRectInt CWindowDecorator::GetWindowGeometry() const
{
  CRectInt geometry{{0, 0}, m_mainSurfaceSize};

  if (IsDecorationActive())
  {
    for (auto const& surface : m_borderSurfaces)
    {
      geometry.Union(surface.windowRect + surface.geometry.P1());
    }
  }

  return geometry;
}

void CWindowDecorator::ResetShm()
{
  if (IsDecorationActive())
  {
    m_memory = std::make_unique<CSharedMemory>(MemoryBytesForSize(m_mainSurfaceSize, m_scale));
    m_memoryAllocatedSize = 0;
    m_shmPool = m_shm.create_pool(m_memory->Fd(), m_memory->Size());
  }
  else
  {
    m_memory.reset();
    m_shmPool.proxy_release();
  }

  for (auto& borderSurface : m_borderSurfaces)
  {
    // Buffers are invalid now, reset
    borderSurface.surface.buffer = Buffer{};
  }
}

void CWindowDecorator::PositionButtons()
{
  CPointInt position{m_borderSurfaces[SURFACE_TOP].surface.size.Width() - BORDER_WIDTH, BORDER_WIDTH + BUTTONS_EDGE_DISTANCE};
  for (auto iter = m_buttons.rbegin(); iter != m_buttons.rend(); iter++)
  {
    position.x -= (BUTTONS_EDGE_DISTANCE + BUTTON_SIZE);
    // Clamp if not enough space
    position.x = std::max(0, position.x);

    iter->position = CRectInt{position, position + CPointInt{BUTTON_SIZE, BUTTON_SIZE}};
  }
}

CWindowDecorator::Buffer CWindowDecorator::GetBuffer(CSizeInt size)
{
  // We ignore tearing on the decorations for now.
  // We can always implement a clever buffer management scheme later... :-)

  auto totalSize{size.Area() * BYTES_PER_PIXEL};
  if (m_memory->Size() < m_memoryAllocatedSize + totalSize)
  {
    // We miscalculated something
    throw std::logic_error("Remaining SHM pool size is too small for requested buffer");
  }
  // argb8888 support is mandatory
  auto buffer = m_shmPool.create_buffer(m_memoryAllocatedSize, size.Width(), size.Height(), size.Width() * BYTES_PER_PIXEL, wayland::shm_format::argb8888);

  void* data{static_cast<std::uint8_t*> (m_memory->Data()) + m_memoryAllocatedSize};
  m_memoryAllocatedSize += totalSize;

  return { data, static_cast<std::size_t> (totalSize), size, std::move(buffer) };
}

void CWindowDecorator::AllocateBuffers()
{
  for (std::size_t i{0}; i < m_borderSurfaces.size(); i++)
  {
    auto& borderSurface = m_borderSurfaces[i];
    if (!borderSurface.surface.buffer.data)
    {
      borderSurface.geometry = SurfaceGeometry(static_cast<SurfaceIndex> (i), m_mainSurfaceSize);
      borderSurface.windowRect = SurfaceWindowRegion(static_cast<SurfaceIndex> (i), m_mainSurfaceSize);
      borderSurface.surface.buffer = GetBuffer(borderSurface.geometry.ToSize() * m_scale);
      borderSurface.surface.scale = m_scale;
      borderSurface.surface.size = borderSurface.geometry.ToSize();
      auto opaqueRegionGeometry = SurfaceOpaqueRegion(static_cast<SurfaceIndex> (i), m_mainSurfaceSize);
      auto region = m_compositor.create_region();
      region.add(opaqueRegionGeometry.x1, opaqueRegionGeometry.y1, opaqueRegionGeometry.Width(), opaqueRegionGeometry.Height());
      borderSurface.surface.wlSurface.set_opaque_region(region);
      if (borderSurface.surface.wlSurface.can_set_buffer_scale())
      {
        borderSurface.surface.wlSurface.set_buffer_scale(m_scale);
      }
    }
  }
}

void CWindowDecorator::Repaint()
{
  // Fill transparent (outer) and color (inner)
  for (auto& borderSurface : m_borderSurfaces)
  {
    std::fill_n(static_cast<std::uint32_t*> (borderSurface.surface.buffer.data), borderSurface.surface.buffer.size.Area(), Endian_SwapLE32(TRANSPARENT));
    FillRectangle(borderSurface.surface, BORDER_COLOR, borderSurface.windowRect - CSizeInt{1, 1});
  }
  auto& topSurface = m_borderSurfaces[SURFACE_TOP].surface;
  auto innerBorderColor = m_buttonColor;
  // Draw rectangle
  DrawHorizontalLine(topSurface, innerBorderColor, {BORDER_WIDTH - 1, BORDER_WIDTH - 1}, topSurface.size.Width() - 2 * BORDER_WIDTH + 2);
  DrawVerticalLine(topSurface, innerBorderColor, {BORDER_WIDTH - 1, BORDER_WIDTH - 1}, topSurface.size.Height() - BORDER_WIDTH + 1);
  DrawVerticalLine(topSurface, innerBorderColor, {topSurface.size.Width() - BORDER_WIDTH, BORDER_WIDTH - 1}, topSurface.size.Height() - BORDER_WIDTH + 1);
  DrawVerticalLine(m_borderSurfaces[SURFACE_LEFT].surface, innerBorderColor, {BORDER_WIDTH - 1, 0}, m_borderSurfaces[SURFACE_LEFT].surface.size.Height());
  DrawVerticalLine(m_borderSurfaces[SURFACE_RIGHT].surface, innerBorderColor, {0, 0}, m_borderSurfaces[SURFACE_RIGHT].surface.size.Height());
  DrawHorizontalLine(m_borderSurfaces[SURFACE_BOTTOM].surface, innerBorderColor, {BORDER_WIDTH - 1, 0}, m_borderSurfaces[SURFACE_BOTTOM].surface.size.Width() - 2 * BORDER_WIDTH + 2);
  // Draw white line into top bar as separator
  DrawHorizontalLine(topSurface, innerBorderColor, {BORDER_WIDTH - 1, topSurface.size.Height() - 1}, topSurface.size.Width() - 2 * BORDER_WIDTH + 2);
  // Draw buttons
  for (auto& button : m_buttons)
  {
    button.draw(topSurface, button.position, button.hovered);
  }

  // Finally make everything visible
  CommitAllBuffers();
}

void CWindowDecorator::CommitAllBuffers()
{
  std::unique_lock<CCriticalSection> lock(m_pendingBuffersMutex);

  for (auto& borderSurface : m_borderSurfaces)
  {
    auto& wlSurface = borderSurface.surface.wlSurface;
    auto& wlBuffer = borderSurface.surface.buffer.wlBuffer;
    // Keep buffers in list so they are kept alive even when the Buffer gets
    // recreated on size change
    auto emplaceResult = m_pendingBuffers.emplace(wlBuffer);
    if (emplaceResult.second)
    {
      // Buffer was not pending already
      auto wlBufferC = reinterpret_cast<wl_buffer*> (wlBuffer.c_ptr());
      // We can refer to the buffer neither by iterator (might be invalidated) nor by
      // capturing the C++ instance in the lambda (would create a reference loop and
      // never allow the object to be freed), so use the raw pointer for now
      wlBuffer.on_release() = [this, wlBufferC]()
      {
        std::unique_lock<CCriticalSection> lock(m_pendingBuffersMutex);
        // Construct a dummy object for searching the set
        wayland::buffer_t findDummy(wlBufferC, wayland::proxy_t::wrapper_type::foreign);
        auto iter = m_pendingBuffers.find(findDummy);
        if (iter == m_pendingBuffers.end())
        {
          throw std::logic_error("Cannot release buffer that is not pending");
        }

        // Do not erase again until buffer is reattached (should not happen anyway, just to be safe)
        // const_cast is OK since changing the function pointer does not affect
        // the key in the set
        const_cast<wayland::buffer_t&>(*iter).on_release() = nullptr;
        m_pendingBuffers.erase(iter);
      };
    }

    wlSurface.attach(wlBuffer, 0, 0);
    wlSurface.damage(0, 0, borderSurface.surface.size.Width(), borderSurface.surface.size.Height());
    wlSurface.commit();
  }
}

void CWindowDecorator::LoadCursorTheme()
{
  std::unique_lock<CCriticalSection> lock(m_mutex);
  if (!m_cursorTheme)
  {
    // Load default cursor theme
    // Base size of 24px is what most cursor themes seem to have
    m_cursorTheme = wayland::cursor_theme_t("", 24 * m_scale, m_shm);
  }
}
