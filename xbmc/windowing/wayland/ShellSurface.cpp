/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShellSurface.h"

#include "utils/StringUtils.h"

using namespace KODI::WINDOWING::WAYLAND;

std::string IShellSurface::StateToString(StateBitset state)
{
  std::vector<std::string> parts;
  if (state.test(STATE_ACTIVATED))
  {
    parts.emplace_back("activated");
  }
  if (state.test(STATE_FULLSCREEN))
  {
    parts.emplace_back("fullscreen");
  }
  if (state.test(STATE_MAXIMIZED))
  {
    parts.emplace_back("maximized");
  }
  if (state.test(STATE_RESIZING))
  {
    parts.emplace_back("resizing");
  }
  return parts.empty() ? "none" : StringUtils::Join(parts, ",");
}