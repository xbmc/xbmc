/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Connection.h"

#include <cassert>

using namespace KODI::WINDOWING::WAYLAND;

CConnection::CConnection()
{
  m_display.reset(new wayland::display_t);
}

wayland::display_t& CConnection::GetDisplay()
{
  assert(m_display);
  return *m_display;
}