/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include <wayland-client.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * Connection to Wayland compositor
 */
class CConnection
{
public:
  CConnection();

  bool HasDisplay() const;
  wayland::display_t& GetDisplay();

private:
  std::unique_ptr<wayland::display_t> m_display;
};

}
}
}
