#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IAddonInterface.h"

#include <stdint.h>

namespace KODI
{
namespace MESSAGING
{
  class ThreadMessage;
}
}

typedef struct AddonCB
{
  const char* libBasePath;  ///< Never, never change this!!!
  void*       addonData;
  void*       interface;
} AddonCB;

namespace ADDON
{

  class CAddon;

  class CAddonInterfaces
  {
  public:
    CAddonInterfaces(CAddon* addon);
    ~CAddonInterfaces();

    AddonCB* GetCallbacks()        { return m_callbacks; }
    CAddon *GetAddon()             { return m_addon; }
    const CAddon *GetAddon() const { return m_addon; }
    void* GetInterface() { return m_addonInterface; }

    /*
     * API level independent functions for Kodi
     */
    static void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

  private:
    AddonCB*  m_callbacks;
    CAddon*   m_addon;
    void* m_addonInterface;
  };

} /* namespace ADDON */
