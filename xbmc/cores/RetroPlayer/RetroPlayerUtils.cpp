/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayerUtils.h"

using namespace KODI;
using namespace RETRO;

std::string CRetroPlayerUtils::ViewModeToDescription(VIEWMODE viewMode)
{
  switch (viewMode)
  {
    case VIEWMODE::Normal:
      return "normal";
    case VIEWMODE::Stretch4x3:
      return "4:3";
    case VIEWMODE::Stretch16x9:
      return "16:9";
    case VIEWMODE::Original:
      return "original";
    default:
      break;
  }

  return "";
}
