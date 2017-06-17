/*
 *      Copyright (C) 2005-2016 Team XBMC
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

#pragma once

namespace ADDON {
class CAddonMgr;
class CBinaryAddonManager;
class CBinaryAddonCache;
class CVFSAddonCache;
class CServiceAddonManager;
}

namespace ActiveAE {
class CActiveAEDSP;
}

namespace ANNOUNCEMENT
{
  class CAnnouncementManager;
}

namespace PVR
{
  class CPVRManager;
}

namespace PLAYLIST
{
  class CPlayListPlayer;
}

class CContextMenuManager;
class XBPython;
class CDataCacheCore;
class CSettings;
class IAE;
class CFavouritesService;

namespace KODI
{
namespace GAME
{
  class CGameServices;
}
}

namespace PERIPHERALS
{
  class CPeripherals;
}

class CServiceBroker
{
public:
  static ADDON::CAddonMgr &GetAddonMgr();
  static ADDON::CBinaryAddonManager &GetBinaryAddonManager();
  static ADDON::CBinaryAddonCache &GetBinaryAddonCache();
  static ADDON::CVFSAddonCache &GetVFSAddonCache();
  static ANNOUNCEMENT::CAnnouncementManager &GetAnnouncementManager();
  static XBPython &GetXBPython();
  static PVR::CPVRManager &GetPVRManager();
  static IAE& GetActiveAE();
  static CContextMenuManager& GetContextMenuManager();
  static CDataCacheCore& GetDataCacheCore();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  static CSettings& GetSettings();
  static KODI::GAME::CGameServices& GetGameServices();
  static PERIPHERALS::CPeripherals& GetPeripherals();
  static CFavouritesService& GetFavouritesService();
  static ADDON::CServiceAddonManager& GetServiceAddons();
  static bool IsBinaryAddonCacheUp();
};
