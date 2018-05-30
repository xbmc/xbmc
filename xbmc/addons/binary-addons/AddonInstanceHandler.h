/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#pragma once

#include "AddonDll.h"
#include "addons/AddonVersion.h"
#include "addons/kodi-addon-dev-kit/include/kodi/AddonBase.h"

#include <memory>

namespace ADDON
{

  class IAddonInstanceHandler
  {
  public:
    IAddonInstanceHandler(ADDON_TYPE type, const BinaryAddonBasePtr& addonBase, KODI_HANDLE parentInstance = nullptr, const std::string& instanceID = "");
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
    const AddonDllPtr& Addon() { return m_addon; }
    BinaryAddonBasePtr GetAddonBase() { return m_addonBase; };

  private:
    ADDON_TYPE m_type;
    std::string m_instanceId;
    KODI_HANDLE m_parentInstance;
    BinaryAddonBasePtr m_addonBase;
    AddonDllPtr m_addon;
  };

} /* namespace ADDON */

