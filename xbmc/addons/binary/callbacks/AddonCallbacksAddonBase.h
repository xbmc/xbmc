#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/binary/callbacks/api1/Addon/AddonCallbacksAddon.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"

namespace KODI
{
  namespace MESSAGING
  {
    class ThreadMessage;
  };
};

namespace ADDON
{

  class CAddonCallbacksAddonBase
  {
  public:
    static int APILevel();
    static int MinAPILevel();
    static std::string Version();
    static std::string MinVersion();
    static void* CreateHelper(CAddonCallbacks* addon, int level);
    static void DestroyHelper(CAddonCallbacks* addon);

    /*
     * API level independent functions for Kodi
     */
    static void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);
  };

}; /* namespace ADDON */
