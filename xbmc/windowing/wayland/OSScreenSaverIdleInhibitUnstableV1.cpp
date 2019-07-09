/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaverIdleInhibitUnstableV1.h"

#include "Registry.h"

#include <cassert>

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
