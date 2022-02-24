/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/Geometry.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <set>
#include <tuple>

#include <wayland-client-protocol.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * wl_output handler that collects information from the compositor and then
 * passes it on when everything is available
 */
class COutput
{
public:
  COutput(std::uint32_t globalName, wayland::output_t const & output, std::function<void()> doneHandler);
  ~COutput() noexcept;

  wayland::output_t const& GetWaylandOutput() const
  {
    return m_output;
  }
  std::uint32_t GetGlobalName() const
  {
    return m_globalName;
  }
  /**
   * Get output position in compositor coordinate space
   * \return (x, y) tuple of output position
   */
  CPointInt GetPosition() const
  {
    std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
    return m_position;
  }
  /**
   * Get output physical size in millimeters
   * \return (width, height) tuple of output physical size in millimeters
   */
  CSizeInt GetPhysicalSize() const
  {
    std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
    return m_physicalSize;
  }
  std::string const& GetMake() const
  {
    std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
    return m_make;
  }
  std::string const& GetModel() const
  {
    std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
    return m_model;
  }
  std::int32_t GetScale() const
  {
    return m_scale;
  }

  struct Mode
  {
    CSizeInt size;
    std::int32_t refreshMilliHz;
    Mode(CSizeInt size, std::int32_t refreshMilliHz)
    : size{size}, refreshMilliHz(refreshMilliHz)
    {}

    float GetRefreshInHz() const
    {
      return refreshMilliHz / 1000.0f;
    }

    std::tuple<std::int32_t, std::int32_t, std::int32_t> AsTuple() const
    {
      return std::make_tuple(size.Width(), size.Height(), refreshMilliHz);
    }

    // Comparison operator needed for std::set
    bool operator<(const Mode& right) const
    {
      return AsTuple() < right.AsTuple();
    }

    bool operator==(const Mode& right) const
    {
      return AsTuple() == right.AsTuple();
    }

    bool operator!=(const Mode& right) const
    {
      return !(*this == right);
    }
  };

  std::set<Mode> const& GetModes() const
  {
    return m_modes;
  }
  Mode const& GetCurrentMode() const;
  Mode const& GetPreferredMode() const;

  float GetPixelRatioForMode(Mode const& mode) const;
  float GetDpiForMode(Mode const& mode) const;
  float GetCurrentDpi() const;

private:
  COutput(COutput const& other) = delete;
  COutput& operator=(COutput const& other) = delete;

  std::uint32_t m_globalName;
  wayland::output_t m_output;
  std::function<void()> m_doneHandler;

  mutable CCriticalSection m_geometryCriticalSection;
  mutable CCriticalSection m_iteratorCriticalSection;

  CPointInt m_position;
  CSizeInt m_physicalSize;
  std::string m_make, m_model;
  std::atomic<std::int32_t> m_scale{1}; // default scale of 1 if no wl_output::scale is sent

  std::set<Mode> m_modes;
  // For std::set, insertion never invalidates existing iterators, and modes are
  // never removed, so the usage of iterators is safe
  std::set<Mode>::iterator m_currentMode{m_modes.end()};
  std::set<Mode>::iterator m_preferredMode{m_modes.end()};
};


}
}
}
