/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

struct AddonCB;

namespace KODI
{
namespace MESSAGING
{
  class ThreadMessage;
}
}

namespace ADDON
{

  class CAddon;

  class CAddonInterfaces
  {
  public:
    explicit CAddonInterfaces(CAddon* addon);
    ~CAddonInterfaces();

    AddonCB* GetCallbacks()        { return m_callbacks; }
    CAddon *GetAddon()             { return m_addon; }
    const CAddon *GetAddon() const { return m_addon; }
    /*\_________________________________________________________________________
    \*/
    static void*        AddOnLib_RegisterMe            (void* addonData);
    static void         AddOnLib_UnRegisterMe          (void* addonData, void* cbTable);
    void*               AddOnLib_GetHelper()          { return m_helperAddOn; }
    /*\_________________________________________________________________________
    \*/
    static void*        GUILib_RegisterMe              (void* addonData);
    static void         GUILib_UnRegisterMe            (void* addonData, void* cbTable);
    void*               GUILib_GetHelper()            { return m_helperGUI; }
    /*\_________________________________________________________________________
    \*/
    static void*        PVRLib_RegisterMe              (void* addonData);
    static void         PVRLib_UnRegisterMe            (void* addonData, void* cbTable);
    /*
     * API level independent functions for Kodi
     */
    static void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

  private:
    CAddonInterfaces(const CAddonInterfaces&) = delete;
    CAddonInterfaces& operator=(const CAddonInterfaces&) = delete;
    AddonCB*  m_callbacks;
    CAddon*   m_addon;

    void*     m_helperAddOn;
    void*     m_helperGUI;
  };

} /* namespace ADDON */
