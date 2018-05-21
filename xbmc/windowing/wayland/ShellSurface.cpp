/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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