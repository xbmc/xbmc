/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonDll.h"
#include "addons/AddonVersion.h"
#include "addons/kodi-dev-kit/include/kodi/AddonBase.h"

#include <memory>

namespace ADDON
{

  class IAddonInstanceHandler
  {
  public:
    IAddonInstanceHandler(ADDON_TYPE type,
                          const AddonInfoPtr& addonInfo,
                          KODI_HANDLE parentInstance = nullptr,
                          const std::string& instanceID = "");
    virtual ~IAddonInstanceHandler();

    ADDON_TYPE UsedType() const { return m_type; }
    const std::string& InstanceID() { return m_instanceId; }

    std::string ID() const;
    std::string Name() const;
    std::string Author() const;
    std::string Icon() const;
    std::string Path() const;
    std::string Profile() const;
    AddonVersion Version() const;

    ADDON_STATUS CreateInstance(KODI_HANDLE instance);
    void DestroyInstance();
    const AddonDllPtr& Addon() const { return m_addon; }
    AddonInfoPtr GetAddonInfo() const { return m_addonInfo; }

    virtual void OnPreInstall() {}
    virtual void OnPostInstall(bool update, bool modal) {}
    virtual void OnPreUnInstall() {}
    virtual void OnPostUnInstall() {}

  private:
    ADDON_TYPE m_type;
    std::string m_instanceId;
    KODI_HANDLE m_parentInstance;
    AddonInfoPtr m_addonInfo;
    BinaryAddonBasePtr m_addonBase;
    AddonDllPtr m_addon;
    static CCriticalSection m_cdSec;
  };

} /* namespace ADDON */

