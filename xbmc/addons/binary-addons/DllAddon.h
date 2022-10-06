/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DynamicDll.h"

class DllAddonInterface
{
public:
  virtual ~DllAddonInterface() = default;
  virtual ADDON_STATUS Create(void* cb) = 0;
  virtual const char* GetAddonTypeVersion(int type)=0;
  virtual const char* GetAddonTypeMinVersion(int type) = 0;
};

class DllAddon : public DllDynamic, public DllAddonInterface
{
public:
  DECLARE_DLL_WRAPPER_TEMPLATE(DllAddon)
  DEFINE_METHOD1(ADDON_STATUS, Create, (void* p1))
  DEFINE_METHOD1(const char*, GetAddonTypeVersion, (int p1))
  DEFINE_METHOD1(const char*, GetAddonTypeMinVersion, (int p1))
  bool GetAddonTypeMinVersion_available() { return m_GetAddonTypeMinVersion != nullptr; }
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(ADDON_Create, Create)
    RESOLVE_METHOD_RENAME(ADDON_GetTypeVersion, GetAddonTypeVersion)
    RESOLVE_METHOD_RENAME_OPTIONAL(ADDON_GetTypeMinVersion, GetAddonTypeMinVersion)
  END_METHOD_RESOLVE()
};
