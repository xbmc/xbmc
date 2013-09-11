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

#include "GameInfoTagLoader.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#define PLATFORM_SEPARATOR  "|"

using namespace GAME_INFO;
using namespace std;

/*
 * Lookups are made using comparisons between case-insensitive alphanumeric
 * strings. "CD-i" will match "CDi", "CD_I" and "CD I". For performance reasons,
 * extensions are parsed and cached in PlatformInfo::parsedExtensions.
 */
namespace GAME_INFO
{
  static PlatformInfo platformInfo[] =
  {// ID                             Name                    Ports  Extensions
    { PLATFORM_UNKNOWN,              "",                     -1,    "" },
    { PLATFORM_3D0,                  "3DO",                  -1,    "" },
    { PLATFORM_AMIGA,                "Amiga",                -1,    "" },
    { PLATFORM_AMIGA_CD32,           "Amiga CD32",           -1,    "" },
    { PLATFORM_AMSTRAD_CPC,          "Amstrad CPC",          -1,    "" },
    { PLATFORM_APPLE_II,             "Apple II",             -1,    "" },
    { PLATFORM_ATARI_2600,           "Atari 2600",           -1,    "" },
    { PLATFORM_ATARI_5200,           "Atari 5200",           -1,    "" },
    { PLATFORM_ATARI_7800,           "Atari 7800",           -1,    "" },
    { PLATFORM_ATARI_8_BIT,          "Atari 8-bit",          -1,    "" },
    { PLATFORM_ATARI_ST,             "Atari ST",             -1,    "" },
    { PLATFORM_BBC_MICRO,            "BBC Micro",            -1,    "" },
    { PLATFORM_BREW,                 "BREW",                 -1,    "" },
    { PLATFORM_CD_I,                 "CD-i",                 -1,    "" },
    { PLATFORM_CHANNEL_F,            "Channel F",            -1,    "" },
    { PLATFORM_COLECO_VISION,        "ColecoVision",         -1,    "" },
    { PLATFORM_COMMODORE_128,        "Commodore 128",        -1,    "" },
    { PLATFORM_COMMODORE_64,         "Commodore 64",         -1,    "" },
    { PLATFORM_COMMODORE_PET_CBM,    "Commodore PET/CBM",    -1,    "" },
    { PLATFORM_DOJA,                 "DoJa",                 -1,    "" },
    { PLATFORM_DOS,                  "DOS",                  -1,    "" },
    { PLATFORM_DRAGON_32_64,         "Dragon 32/64",         -1,    "" },
    { PLATFORM_DREAMCAST,            "Dreamcast",            -1,    "" },
    { PLATFORM_ELECTRON,             "Electron",             -1,    "" },
    { PLATFORM_EXEN,                 "ExEn",                 -1,    "" },
    { PLATFORM_GAMEBOY,              "Game Boy",             -1,    ".gb" },
    { PLATFORM_GAMEBOY_ADVANCE,      "Game Boy Advance",     -1,    ".gba|.agb|.elf|.mb|.bin" },
    { PLATFORM_GAMEBOY_COLOR,        "Game Boy Color",       -1,    ".gbc|.cgb|.sgb" },
    { PLATFORM_GAMECUBE,             "GameCube",             -1,    "" },
    { PLATFORM_GAME_GEAR,            "Game Gear",            -1,    "" },
    { PLATFORM_GENESIS,              "Genesis",              -1,    "" },
    { PLATFORM_GIZMONDO,             "Gizmondo",             -1,    "" },
    { PLATFORM_INTELLIVISION,        "Intellivision",        -1,    "" },
    { PLATFORM_JAGUAR,               "Jaguar",               -1,    "" },
    { PLATFORM_LINUX,                "Linux",                -1,    "" },
    { PLATFORM_LYNX,                 "Lynx",                 -1,    "" },
    { PLATFORM_MACINTOSH,            "Macintosh",            -1,    "" },
    { PLATFORM_MAME,                 "MAME",                 -1,    "" },
    { PLATFORM_MOPHUN,               "Mophun",               -1,    "" },
    { PLATFORM_MSX,                  "MSX",                  -1,    "" },
    { PLATFORM_NEO_GEO,              "Neo Geo",              -1,    "" },
    { PLATFORM_NEO_GEO_CD,           "Neo Geo CD",           -1,    "" },
    { PLATFORM_NEO_GEO_POCKET,       "Neo Geo Pocket",       -1,    "" },
    { PLATFORM_NEO_GEO_POCKET_COLOR, "Neo Geo Pocket Color", -1,    "" },
    { PLATFORM_NES,                  "NES",                  -1,    "" },
    { PLATFORM_N_GAGE,               "N-Gage",               -1,    "" },
    { PLATFORM_NINTENDO_64,          "Nintendo 64",          -1,    "" },
    { PLATFORM_NINTENDO_DS,          "Nintendo DS",          -1,    "" },
    { PLATFORM_NINTENDO_DSI,         "Nintendo DSi",         -1,    "" },
    { PLATFORM_ODYSSEY,              "Odyssey",              -1,    "" },
    { PLATFORM_ODYSSEY_2,            "Odyssey 2",            -1,    "" },
    { PLATFORM_PC_88,                "PC-88",                -1,    "" },
    { PLATFORM_PC_98,                "PC-98",                -1,    "" },
    { PLATFORM_PC_BOOTER,            "PC Booter",            -1,    "" },
    { PLATFORM_PC_FX,                "PC-FX",                -1,    "" },
    { PLATFORM_PLAYSTATION,          "PlayStation",          -1,    "" },
    { PLATFORM_PLAYSTATION_2,        "PlayStation 2",        -1,    "" },
    { PLATFORM_PLAYSTATION_3,        "PlayStation 3",        -1,    "" },
    { PLATFORM_PSP,                  "PSP",                  -1,    "" },
    { PLATFORM_SEGA_32X,             "SEGA 32X ",            -1,    "" },
    { PLATFORM_SEGA_CD,              "SEGA CD",              -1,    "" },
    { PLATFORM_SEGA_MASTER_SYSTEM,   "SEGA Master System",   -1,    "" },
    { PLATFORM_SEGA_SATURN,          "SEGA Saturn",          -1,    "" },
    { PLATFORM_SNES,                 "SNES",                 -1,    ".smc|.sfc|.fig|.gd3|.gd7|.dx2|.bsx|.swc" },
    { PLATFORM_SPECTRAVIDEO,         "Spectravideo",         -1,    "" },
    { PLATFORM_TI_99_4A,             "TI-99/4A",             -1,    "" },
    { PLATFORM_TRS_80,               "TRS-80",               -1,    "" },
    { PLATFORM_TRS_80_COCO,          "TRS-80 CoCo",          -1,    "" },
    { PLATFORM_TURBOGRAFX_16,        "TurboGrafx-16",        -1,    "" },
    { PLATFORM_TURBOGRAFX_CD,        "TurboGrafx CD",        -1,    "" },
    { PLATFORM_VECTREX,              "Vectrex",              -1,    "" },
    { PLATFORM_VIC_20,               "VIC-20",               -1,    "" },
    { PLATFORM_VIRTUAL_BOY,          "Virtual Boy",          -1,    "" },
    { PLATFORM_V_SMILE,              "V.Smile",              -1,    "" },
    { PLATFORM_WII,                  "Wii",                  -1,    "" },
    { PLATFORM_WINDOWS,              "Windows",              -1,    "" },
    { PLATFORM_WINDOWS_3X,           "Windows 3.x",          -1,    "" },
    { PLATFORM_WONDERSWAN,           "WonderSwan",           -1,    "" },
    { PLATFORM_WONDERSWAN_COLOR,     "WonderSwan Color",     -1,    "" },
    { PLATFORM_XBOX,                 "Xbox",                 -1,    "" },
    { PLATFORM_XBOX_360,             "Xbox 360",             -1,    "" },
    { PLATFORM_ZEEBO,                "Zeebo",                -1,    "" },
    { PLATFORM_ZODIAC,               "Zodiac",               -1,    "" },
    { PLATFORM_ZX_SPECTR,            "ZX Spectr",            -1,    "" },
  };
}

/* static */
CGameInfoTagLoader &CGameInfoTagLoader::Get()
{
  static CGameInfoTagLoader gameInfoTagLoaderInstance;
  return gameInfoTagLoaderInstance;
}

bool CGameInfoTagLoader::Load(const string &strPath, CGameInfoTag &tag)
{
  if (strPath.empty())
    return false;

  PlatformInfo platform = GetPlatformInfoByExtension(URIUtils::GetExtension(strPath));
  if (platform.id == PLATFORM_UNKNOWN)
    return false;
  
  tag.SetPlatform(platform.name);
  tag.SetLoaded(true);

  // TODO: implement game info loaders per platform
  switch (platform.id)
  {
  case PLATFORM_GAMEBOY:
  case PLATFORM_GAMEBOY_COLOR:
  case PLATFORM_GAMEBOY_ADVANCE:
  default:
    break;
  }
  return true;
}

/* static */
const PlatformInfo &CGameInfoTagLoader::GetPlatformInfoByName(const string &strPlatform)
{
  if (strPlatform.empty())
    return platformInfo[0]; // Unknown

  for (size_t i = 0; i < ARRAY_LENGTH(platformInfo); i++)
    if (SanitizedEquals(strPlatform.c_str(), platformInfo[i].name))
      return platformInfo[i];

  return platformInfo[0]; // Unknown
}

/* static */
const PlatformInfo &CGameInfoTagLoader::GetPlatformInfoByExtension(const string &strExtension)
{
  if (strExtension.empty())
    return platformInfo[0]; // Unknown

  // Canonicalize as lower case, starts with "."
  string strExt(strExtension);
  StringUtils::ToLower(strExt);
  if (strExt[0] != '.')
    strExt.insert(0, ".");

  for (size_t i = 0; i < ARRAY_LENGTH(platformInfo); i++)
  {
    if (!*platformInfo[i].extensions)
      continue; // No extensions

    vector<string> vecExts = StringUtils::Split(platformInfo[i].extensions, PLATFORM_SEPARATOR);

    for (unsigned int i = 0; i < vecExts.size(); i++)
      if (vecExts[i] == strExt)
        return platformInfo[i];
  }
  return platformInfo[0]; // Unknown
}

/* static */
const PlatformInfo &CGameInfoTagLoader::GetPlatformInfoByID(GamePlatform platform)
{
  for (size_t i = 0; i < ARRAY_LENGTH(platformInfo); i++)
    if (platformInfo[i].id == platform)
      return platformInfo[i];
  return platformInfo[0]; // Unknown
}

#define IS_ALPHANUMERIC(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z') || ('0' <= (c) && (c) <= '9'))
#define LOWER(c) (('A' <= (c) && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

/* static */
bool CGameInfoTagLoader::SanitizedEquals(const char *str1, const char *str2)
{
  // Sanity check
  if (!str1 || !str2)
    return false;
  if (str1 == str2)
    return true;

  // Break at the first null character
  for (; *str1 && *str2; )
  {
    // Advance to the next alphanumeric character
    while (*str1 && !IS_ALPHANUMERIC(*str1))
      str1++;
    while (*str2 && !IS_ALPHANUMERIC(*str2))
      str2++;

    // If they differ, we're done here, otherwise increment and continue
    if (LOWER(*str1) != LOWER(*str2))
      return false;

    str1++;
    str2++;
  }

  // Final test, return true if these are both null
  return *str1 == *str2;
}
