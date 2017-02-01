#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
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

  ADDON_STATUS __declspec(dllexport) ADDON_Create(void *callbacks, void* props);
  void         __declspec(dllexport) ADDON_Destroy();
  ADDON_STATUS __declspec(dllexport) ADDON_GetStatus();
  bool         __declspec(dllexport) ADDON_HasSettings();
  unsigned int __declspec(dllexport) ADDON_GetSettings(ADDON_StructSetting ***sSet);
  ADDON_STATUS __declspec(dllexport) ADDON_SetSetting(const char *settingName, const void *settingValue);
  void         __declspec(dllexport) ADDON_FreeSettings();
  __declspec(dllexport) const char* ADDON_GetTypeVersion(int type)
  {
    return kodi::addon::GetTypeVersion(type);
  }
  __declspec(dllexport) ADDON_STATUS ADDON_CreateInstance(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance);
  __declspec(dllexport) void ADDON_DestroyInstance(int instanceType, KODI_HANDLE instance);

#ifdef __cplusplus
};
#endif
