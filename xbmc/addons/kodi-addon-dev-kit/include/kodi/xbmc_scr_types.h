#pragma once
/*
 *      Copyright (C) 2005-2016 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonBase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AddonProps_Screensaver
{
  void *device;
  int x;
  int y;
  int width;
  int height;
  float pixelRatio;
  const char *name;
  const char *presets;
  const char *profile;
} AddonProps_Screensaver;

typedef struct AddonToKodiFuncTable_Screensaver
{
  KODI_HANDLE kodiInstance;
} AddonToKodiFuncTable_Screensaver;

typedef struct KodiToAddonFuncTable_Screensaver
{
  void (__cdecl* Start) (KODI_HANDLE addonHandle);
  void (__cdecl* Stop) (KODI_HANDLE addonHandle);
  void (__cdecl* Render) (KODI_HANDLE addonHandle);
} KodiToAddonFuncTable_Screensaver;

typedef struct AddonInstance_ScreenSaver
{
  AddonProps_Screensaver props;
  AddonToKodiFuncTable_Screensaver toKodi;
  KodiToAddonFuncTable_Screensaver toAddon;
} AddonInstance_ScreenSaver;

#ifdef __cplusplus
}
#endif
