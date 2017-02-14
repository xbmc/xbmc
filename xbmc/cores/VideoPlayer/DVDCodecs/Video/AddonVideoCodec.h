#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDVideoCodec.h"
#include "addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/VideoCodec.h"

class CAddonVideoCodec
  : public CDVDVideoCodec
  , public ADDON::IAddonInstanceHandler
{
public:
  CAddonVideoCodec(CProcessInfo &processInfo, ADDON::AddonInfoPtr& addonInfo, kodi::addon::IAddonInstance* parentInstance);
  virtual ~CAddonVideoCodec();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool Reconfigure(CDVDStreamInfo &hints) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset() override;
  virtual VCReturn GetPicture(DVDVideoPicture* pDvdVideoPicture) override;
  virtual const char* GetName() override;
  virtual void SetCodecControl(int flags) override { m_codecFlags = flags; }
private:
  bool CopyToInitData(VIDEOCODEC_INITDATA &initData, CDVDStreamInfo &hints);

  kodi::addon::CInstanceVideoCodec* m_addonInstance;
  AddonInstance_VideoCodec m_struct;
  
  int m_codecFlags;
  VIDEOCODEC_FORMAT m_formats[VIDEOCODEC_FORMAT::MaxVideoFormats + 1];
};
