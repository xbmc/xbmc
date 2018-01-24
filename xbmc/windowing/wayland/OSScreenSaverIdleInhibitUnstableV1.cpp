/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#include "OSScreenSaverIdleInhibitUnstableV1.h"

#include <cassert>

#include "Registry.h"

using namespace KODI::WINDOWING::WAYLAND;

COSScreenSaverIdleInhibitUnstableV1* COSScreenSaverIdleInhibitUnstableV1::TryCreate(CConnection& connection, wayland::surface_t const& inhibitSurface)
{
  wayland::zwp_idle_inhibit_manager_v1_t manager;
  CRegistry registry{connection};
  registry.RequestSingleton(manager, 1, 1, false);
  registry.Bind();

  if (manager)
  {
    return new COSScreenSaverIdleInhibitUnstableV1(manager, inhibitSurface);
  }
  else
  {
    return nullptr;
  }
}

COSScreenSaverIdleInhibitUnstableV1::COSScreenSaverIdleInhibitUnstableV1(const wayland::zwp_idle_inhibit_manager_v1_t& manager, const wayland::surface_t& inhibitSurface)
: m_manager{manager}, m_surface{inhibitSurface}
{
  assert(m_manager);
  assert(m_surface);
}

void COSScreenSaverIdleInhibitUnstableV1::Inhibit()
{
  if (!m_inhibitor)
  {
    m_inhibitor = m_manager.create_inhibitor(m_surface);
  }
}

void COSScreenSaverIdleInhibitUnstableV1::Uninhibit()
{
  m_inhibitor.proxy_release();
}
