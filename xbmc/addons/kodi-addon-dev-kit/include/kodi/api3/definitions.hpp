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

#define ADDON_API_LEVEL 3

#define NAMESPACE(N) namespace N {
#define END_NAMESPACE() }

// Macros to define includes without give of level number
#define PPCAT_INCLUDE(A, B) kodi/api ## A/B
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)
#define KITINCLUDE(V, I) STRINGIZE(PPCAT_INCLUDE(V,I))

// Macros to define namespace without give of level number
#define PPCAT_NX(A, B) A ## B
#define PPCAT_API_NS(A, B) PPCAT_NX(A, B)
#define API_NAMESPACE namespace PPCAT_API_NS(V,ADDON_API_LEVEL) {
#define API_NAMESPACE_NAME PPCAT_API_NS(V,ADDON_API_LEVEL)

#include "definitions-all.hpp"
#include "addon/definitions_addon.hpp"
#include "audioengine/definitions_audioengine.hpp"
#include "gui/definitions_gui.hpp"
#include "inputstream/definitions_inputstream.hpp"
#include "player/definitions_player.hpp"
#include "pvr/definitions_pvr.hpp"

#if defined(BUILD_KODI_ADDON)
  using namespace V3::KodiAPI;
  using namespace V3;
#endif
