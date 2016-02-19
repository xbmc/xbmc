#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addon/definitions_addon.hpp"
#include "audioengine/definitions_audioengine.hpp"
#include "gui/definitions_gui.hpp"
#include "player/definitions_player.hpp"
#include "pvr/definitions_pvr.hpp"

#if defined(BUILD_KODI_ADDON)
  using namespace V2::KodiAPI;
  using namespace V2;
#endif
