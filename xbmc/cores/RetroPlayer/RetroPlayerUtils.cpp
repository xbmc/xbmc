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

#include "RetroPlayerUtils.h"

using namespace KODI;
using namespace RETRO;

std::string CRetroPlayerUtils::ViewModeToDescription(ViewMode viewMode)
{
  switch (viewMode)
  {
    case ViewModeNormal:
      return "normal";
    case ViewModeStretch4x3:
      return "4:3";
    case ViewModeStretch16x9:
      return "16:9";
    case ViewModeStretch16x9Nonlin:
      return "nonlinear";
    case ViewModeOriginal:
      return "original";
    default:
      break;
  }

  return "";
}
