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

#include <memory>
#include "platform/Platform.h"

namespace ADDON {
class CAddonMgr;
class CBinaryAddonCache;
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

class XBPython;
class CDataCacheCore;

class CServiceManager
{
public:
  bool Init1();
  bool Init2();
  bool Init3();
  void Deinit();
  ADDON::CAddonMgr& GetAddonMgr();
  ADDON::CBinaryAddonCache& GetBinaryAddonCache();
  ANNOUNCEMENT::CAnnouncementManager& GetAnnouncementManager();
  XBPython& GetXBPython();
  PVR::CPVRManager& GetPVRManager();
  ActiveAE::CActiveAEDSP& GetADSPManager();
  CDataCacheCore& GetDataCacheCore();
  /**\brief Get the platform object. This is save to be called after Init1() was called
   */
  CPlatform& GetPlatform();

protected:
  struct delete_dataCacheCore
  {
    void operator()(CDataCacheCore *p) const;
  };

  std::unique_ptr<ADDON::CAddonMgr> m_addonMgr;
  std::unique_ptr<ADDON::CBinaryAddonCache> m_binaryAddonCache;
  std::unique_ptr<ANNOUNCEMENT::CAnnouncementManager> m_announcementManager;
  std::unique_ptr<XBPython> m_XBPython;
  std::unique_ptr<PVR::CPVRManager> m_PVRManager;
  std::unique_ptr<ActiveAE::CActiveAEDSP> m_ADSPManager;
  std::unique_ptr<CDataCacheCore, delete_dataCacheCore> m_dataCacheCore;
  std::unique_ptr<CPlatform> m_Platform;
};
