#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
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

#include "AddonBase.h"

#ifdef __cplusplus
extern "C" {
#endif

  ADDON_STATUS __declspec(dllexport) ADDON_Create(void *callbacks, void* props);
  void         __declspec(dllexport) ADDON_Destroy();
  ADDON_STATUS __declspec(dllexport) ADDON_GetStatus();
  ADDON_STATUS __declspec(dllexport) ADDON_SetSetting(const char *settingName, const void *settingValue);
  __declspec(dllexport) const char* ADDON_GetTypeVersion(int type)
  {
    return kodi::addon::GetTypeVersion(type);
  }

#ifdef __cplusplus
};
#endif
