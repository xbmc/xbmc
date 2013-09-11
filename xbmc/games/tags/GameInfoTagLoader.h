/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "GameInfoTag.h"

#include <set>
#include <string>
#include <vector>

namespace GAME_INFO
{
  // Platforms that XBMC is currently aware of
  // Game ID used is MobyGames ID (choice seemed arbitrary, might as well use an existing system for now)
  enum GamePlatform
  {
    PLATFORM_UNKNOWN              = -1,
    PLATFORM_3D0                  = 35,
    PLATFORM_AMIGA                = 19,
    PLATFORM_AMIGA_CD32           = 56,
    PLATFORM_AMSTRAD_CPC          = 60,
    PLATFORM_APPLE_II             = 31,
    PLATFORM_ATARI_2600           = 28,
    PLATFORM_ATARI_5200           = 33,
    PLATFORM_ATARI_7800           = 34,
    PLATFORM_ATARI_8_BIT          = 39,
    PLATFORM_ATARI_ST             = 24,
    PLATFORM_BBC_MICRO            = 92,
    PLATFORM_BREW                 = 63,
    PLATFORM_CD_I                 = 73,
    PLATFORM_CHANNEL_F            = 76,
    PLATFORM_COLECO_VISION        = 29,
    PLATFORM_COMMODORE_128        = 61,
    PLATFORM_COMMODORE_64         = 27,
    PLATFORM_COMMODORE_PET_CBM    = 77,
    PLATFORM_DOJA                 = 72,
    PLATFORM_DOS                  =  2,
    PLATFORM_DRAGON_32_64         = 79,
    PLATFORM_DREAMCAST            =  8,
    PLATFORM_ELECTRON             = 93,
    PLATFORM_EXEN                 = 70,
    PLATFORM_GAMEBOY              = 10,
    PLATFORM_GAMEBOY_ADVANCE      = 12,
    PLATFORM_GAMEBOY_COLOR        = 11,
    PLATFORM_GAMECUBE             = 14,
    PLATFORM_GAME_GEAR            = 25,
    PLATFORM_GENESIS              = 16,
    PLATFORM_GIZMONDO             = 55,
    PLATFORM_INTELLIVISION        = 30,
    PLATFORM_JAGUAR               = 17,
    PLATFORM_LINUX                = 1,
    PLATFORM_LYNX                 = 18,
    PLATFORM_MACINTOSH            = 74,
    PLATFORM_MAME                 =  0, // No platform, per se
    PLATFORM_MOPHUN               = 71,
    PLATFORM_MSX                  = 57,
    PLATFORM_NEO_GEO              = 36,
    PLATFORM_NEO_GEO_CD           = 54,
    PLATFORM_NEO_GEO_POCKET       = 52,
    PLATFORM_NEO_GEO_POCKET_COLOR = 53,
    PLATFORM_NES                  = 22,
    PLATFORM_N_GAGE               = 32,
    PLATFORM_NINTENDO_64          =  9,
    PLATFORM_NINTENDO_DS          = 44,
    PLATFORM_NINTENDO_DSI         = 87,
    PLATFORM_ODYSSEY              = 75,
    PLATFORM_ODYSSEY_2            = 78,
    PLATFORM_PC_88                = 94,
    PLATFORM_PC_98                = 95,
    PLATFORM_PC_BOOTER            =  4,
    PLATFORM_PC_FX                = 59,
    PLATFORM_PLAYSTATION          =  6,
    PLATFORM_PLAYSTATION_2        =  7,
    PLATFORM_PLAYSTATION_3        = 81,
    PLATFORM_PSP                  = 46,
    PLATFORM_SEGA_32X             = 21,
    PLATFORM_SEGA_CD              = 20,
    PLATFORM_SEGA_MASTER_SYSTEM   = 26,
    PLATFORM_SEGA_SATURN          = 23,
    PLATFORM_SNES                 = 15,
    PLATFORM_SPECTRAVIDEO         = 85,
    PLATFORM_TI_99_4A             = 47,
    PLATFORM_TRS_80               = 58,
    PLATFORM_TRS_80_COCO          = 62,
    PLATFORM_TURBOGRAFX_16        = 40,
    PLATFORM_TURBOGRAFX_CD        = 45,
    PLATFORM_VECTREX              = 37,
    PLATFORM_VIC_20               = 43,
    PLATFORM_VIRTUAL_BOY          = 38,
    PLATFORM_V_SMILE              = 42,
    PLATFORM_WII                  = 82,
    PLATFORM_WINDOWS              =  3,
    PLATFORM_WINDOWS_3X           =  5,
    PLATFORM_WONDERSWAN           = 48,
    PLATFORM_WONDERSWAN_COLOR     = 49,
    PLATFORM_XBOX                 = 13,
    PLATFORM_XBOX_360             = 69,
    PLATFORM_ZEEBO                = 88,
    PLATFORM_ZODIAC               = 68,
    PLATFORM_ZX_SPECTR            = 41,
  };

  typedef std::vector<GamePlatform> GamePlatformArray;

  struct PlatformInfo
  {
    GamePlatform          id;
    const char            *name;
    int                   ports; // -1 for unknown
    const char            *extensions; // Must be unique to the platform (e.g. no zip)
  };

  class CGameInfoTagLoader
  {
  public:
    static CGameInfoTagLoader &Get();

    bool Load(const std::string &strPath, CGameInfoTag &tag);

    /**
     * Get platform info by the platform's name. See struct platformInfo in
     * GameInfoTagLoader.cpp. The comparison is performed case-, space- and
     * punctuation-insensitive. If the name isn't recognized as a valid game
     * platform, the PLATFORM_UNKNOWN struct is returned.
     */
    static const PlatformInfo &GetPlatformInfoByName(const std::string &strPlatform);

    /**
     * Resolve a known extension name into a PlatformInfo struct. If the
     * extension isn't listed in struct platformInfo, PLATFORM_UNKOWN is
     * returned. If the extension is valid for more than 1 platform (zip, bin),
     * PLATFORM_UNKOWN is returned.
     */
    static const PlatformInfo &GetPlatformInfoByExtension(const std::string &strExtension);

    /**
     * Look up platform information by ID.
     */
    static const PlatformInfo &GetPlatformInfoByID(GamePlatform id);

  private:
    CGameInfoTagLoader() { }

    /**
     * Strip all non-alphanumeric characters and compare strings case-insensitive.
     */
    static bool SanitizedEquals(const char *str1, const char *str2);
  };
}
