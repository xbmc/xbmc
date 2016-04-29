#pragma once
/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://xbmc.org
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

#include "DynamicDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_cpp_dll.h"

template <typename TheStruct, typename Props>
class DllAddonInterface
{
public:
  virtual ~DllAddonInterface() {}
  virtual void GetAddon(TheStruct* pAddon) =0;
  virtual ADDON_STATUS Create(void *cb, Props *info) =0;
  virtual void Stop() =0;
  virtual void Destroy() =0;
  virtual ADDON_STATUS GetStatus() =0;
  virtual bool HasSettings() =0;
  virtual unsigned int GetSettings(ADDON_StructSetting*** sSet)=0;
  virtual void FreeSettings()=0;
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue) =0;
};

template <typename TheStruct, typename Props>
class DllAddon : public DllDynamic, public DllAddonInterface<TheStruct, Props>
{
public:
  DECLARE_DLL_WRAPPER_TEMPLATE(DllAddon)
  DEFINE_METHOD2(ADDON_STATUS, Create, (void* p1, Props* p2))
  DEFINE_METHOD0(void, Stop)
  DEFINE_METHOD0(void, Destroy)
  DEFINE_METHOD0(ADDON_STATUS, GetStatus)
  DEFINE_METHOD0(bool, HasSettings)
  DEFINE_METHOD1(unsigned int, GetSettings, (ADDON_StructSetting ***p1))
  DEFINE_METHOD0(void, FreeSettings)
  DEFINE_METHOD2(ADDON_STATUS, SetSetting, (const char *p1, const void *p2))
  DEFINE_METHOD1(void, GetAddon, (TheStruct* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_addon,GetAddon)
    RESOLVE_METHOD_RENAME(ADDON_Create, Create)
    RESOLVE_METHOD_RENAME(ADDON_Stop, Stop)
    RESOLVE_METHOD_RENAME(ADDON_Destroy, Destroy)
    RESOLVE_METHOD_RENAME(ADDON_GetStatus, GetStatus)
    RESOLVE_METHOD_RENAME(ADDON_HasSettings, HasSettings)
    RESOLVE_METHOD_RENAME(ADDON_SetSetting, SetSetting)
    RESOLVE_METHOD_RENAME(ADDON_GetSettings, GetSettings)
    RESOLVE_METHOD_RENAME(ADDON_FreeSettings, FreeSettings)
  END_METHOD_RESOLVE()
};

