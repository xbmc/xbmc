/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DynamicDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/AddonBase.h"

class DllAddonInterface
{
public:
  virtual ~DllAddonInterface() = default;
  virtual void GetAddon(void* pAddon) =0;
  virtual ADDON_STATUS Create(void *cb, void *info) =0;
  virtual void Destroy() =0;
  virtual ADDON_STATUS GetStatus() =0;
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue) =0;
  virtual const char* GetAddonTypeVersion(int type)=0;
};

class DllAddon : public DllDynamic, public DllAddonInterface
{
public:
  DECLARE_DLL_WRAPPER_TEMPLATE(DllAddon)
  DEFINE_METHOD2(ADDON_STATUS, Create, (void* p1, void* p2))
  DEFINE_METHOD0(void, Destroy)
  DEFINE_METHOD0(ADDON_STATUS, GetStatus)
  DEFINE_METHOD2(ADDON_STATUS, SetSetting, (const char *p1, const void *p2))
  DEFINE_METHOD1(void, GetAddon, (void* p1))
  DEFINE_METHOD1(const char*, GetAddonTypeVersion, (int p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_addon,GetAddon)
    RESOLVE_METHOD_RENAME(ADDON_Create, Create)
    RESOLVE_METHOD_RENAME(ADDON_Destroy, Destroy)
    RESOLVE_METHOD_RENAME(ADDON_GetStatus, GetStatus)
    RESOLVE_METHOD_RENAME(ADDON_SetSetting, SetSetting)
    RESOLVE_METHOD_RENAME(ADDON_GetTypeVersion, GetAddonTypeVersion)
  END_METHOD_RESOLVE()
};

