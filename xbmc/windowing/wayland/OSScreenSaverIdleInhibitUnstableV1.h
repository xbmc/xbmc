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