/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Connection.h"

#include "utils/log.h"

#include <cassert>
#include <stdexcept>

using namespace KODI::WINDOWING::WAYLAND;

CConnection::CConnection()
{
  try
  {
    m_display = std::make_unique<wayland::display_t>();
  }
  catch (const std::exception& err)
  {
    CLog::Log(LOGERROR, "Wayland connection error: {}", err.what());
  }
}

bool CConnection::HasDisplay() const
{
  return static_cast<bool>(m_display);
}

wayland::display_t& CConnection::GetDisplay()
{
  assert(m_display);
  return *m_display;
}
