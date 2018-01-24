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