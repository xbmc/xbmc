/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <wayland-extra-protocols.hpp>

#include "Connection.h"
#include "../OSScreenSaver.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class COSScreenSaverIdleInhibitUnstableV1 : public IOSScreenSaver
{
public:
  COSScreenSaverIdleInhibitUnstableV1(wayland::zwp_idle_inhibit_manager_v1_t const& manager, wayland::surface_t const& inhibitSurface);
  static COSScreenSaverIdleInhibitUnstableV1* TryCreate(CConnection& connection, wayland::surface_t const& inhibitSurface);
  void Inhibit() override;
  void Uninhibit() override;

private:
  wayland::zwp_idle_inhibit_manager_v1_t m_manager;
  wayland::zwp_idle_inhibitor_v1_t m_inhibitor;
  wayland::surface_t m_surface;
};

}
}
}
