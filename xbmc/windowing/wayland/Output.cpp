/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Output.h"

#include <cassert>
#include <cmath>
#include <mutex>
#include <set>
#include <stdexcept>
#include <utility>

using namespace KODI::WINDOWING::WAYLAND;

COutput::COutput(std::uint32_t globalName,
                 wayland::output_t const& output,
                 std::function<void()> doneHandler)
  : m_globalName{globalName}, m_output{output}, m_doneHandler{std::move(doneHandler)}
{
  assert(m_output);

  m_output.on_geometry() = [this](std::int32_t x, std::int32_t y, std::int32_t physWidth,
                                  std::int32_t physHeight, wayland::output_subpixel,
                                  std::string const& make, std::string const& model,
                                  const wayland::output_transform&)
  {
    std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
    m_position = {x, y};
    // Some monitors report invalid (negative) values that would cause an exception
    // with CSizeInt
    if (physWidth < 0 || physHeight < 0)
      m_physicalSize = {};
    else
      m_physicalSize = {physWidth, physHeight};
    m_make = make;
    m_model = model;
  };
  m_output.on_mode() = [this](const wayland::output_mode& flags, std::int32_t width,
                              std::int32_t height, std::int32_t refresh) {
    // std::set.emplace returns pair of iterator to the (possibly) inserted
    // element and boolean information whether the element was actually added
    // which we do not need
    auto modeIterator = m_modes.emplace(CSizeInt{width, height}, refresh).first;
    std::unique_lock<CCriticalSection> lock(m_iteratorCriticalSection);
    // Remember current and preferred mode
    // Current mode is the last one that was sent with current flag set
    if (flags & wayland::output_mode::current)
    {
      m_currentMode = modeIterator;
    }
    if (flags & wayland::output_mode::preferred)
    {
      m_preferredMode = modeIterator;
    }
#ifdef TARGET_WEBOS
    // on_done() is never executed on webOS 3.x
    m_doneHandler();
#endif
  };
  m_output.on_scale() = [this](std::int32_t scale)
  {
    m_scale = scale;
  };

  m_output.on_done() = [this]()
  {
#ifndef TARGET_WEBOS
    m_doneHandler();
#endif
  };
}

COutput::~COutput() noexcept
{
  // Reset event handlers - someone might still hold a reference to the output_t,
  // causing events to be dispatched. They should not go to a deleted class.
  m_output.on_geometry() = nullptr;
  m_output.on_mode() = nullptr;
  m_output.on_done() = nullptr;
  m_output.on_scale() = nullptr;
}

const COutput::Mode& COutput::GetCurrentMode() const
{
  std::unique_lock<CCriticalSection> lock(m_iteratorCriticalSection);
  if (m_currentMode == m_modes.end())
  {
    throw std::runtime_error("Current mode not set");
  }
  return *m_currentMode;
}

const COutput::Mode& COutput::GetPreferredMode() const
{
  std::unique_lock<CCriticalSection> lock(m_iteratorCriticalSection);
  if (m_preferredMode == m_modes.end())
  {
    throw std::runtime_error("Preferred mode not set");
  }
  return *m_preferredMode;
}

float COutput::GetPixelRatioForMode(const Mode& mode) const
{
  std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
  if (m_physicalSize.IsZero() || mode.size.IsZero())
  {
    return 1.0f;
  }
  else
  {
    return (
            (static_cast<float> (m_physicalSize.Width()) / static_cast<float> (mode.size.Width()))
            /
            (static_cast<float> (m_physicalSize.Height()) / static_cast<float> (mode.size.Height()))
            );
  }
}

float COutput::GetDpiForMode(const Mode& mode) const
{
  std::unique_lock<CCriticalSection> lock(m_geometryCriticalSection);
  if (m_physicalSize.IsZero())
  {
    // We really have no idea, so use a "sane" default
    return 96.0;
  }
  else
  {
    constexpr float INCH_MM_RATIO{25.4f};

    float diagonalPixels = std::sqrt(mode.size.Width() * mode.size.Width() + mode.size.Height() * mode.size.Height());
    // physicalWidth/physicalHeight is in millimeters
    float diagonalInches =
        std::sqrt(static_cast<float>(m_physicalSize.Width() * m_physicalSize.Width() +
                                     m_physicalSize.Height() * m_physicalSize.Height())) /
        INCH_MM_RATIO;

    return diagonalPixels / diagonalInches;
  }
}

float COutput::GetCurrentDpi() const
{
  return GetDpiForMode(GetCurrentMode());
}
