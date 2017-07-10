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
#include "addons/AddonProvider.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/VideoCodec.h"

class BufferPool;

class CAddonVideoCodec
  : public CDVDVideoCodec
  , public ADDON::IAddonInstanceHandler
{
public:
  CAddonVideoCodec(CProcessInfo &processInfo, ADDON::BinaryAddonBasePtr& addonInfo, kodi::addon::IAddonInstance* parentInstance);
  ~CAddonVideoCodec() override;

  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  bool Reconfigure(CDVDStreamInfo &hints) override;
  bool AddData(const DemuxPacket &packet) override;
  void Reset() override;
  VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  const char* GetName() override;
  void SetCodecControl(int flags) override { m_codecFlags = flags; }

private:
  bool CopyToInitData(VIDEOCODEC_INITDATA &initData, CDVDStreamInfo &hints);

  /*!
   * @brief All picture members can be expected to be set correctly except decodedData and pts.
   * GetFrameBuffer has to set decodedData to a valid memory adress and return true.
   * In case buffer allocation fails, return false.
   */
  bool GetFrameBuffer(VIDEOCODEC_PICTURE &picture);

  static bool get_frame_buffer(void* kodiInstance, VIDEOCODEC_PICTURE *picture);

  AddonInstance_VideoCodec m_struct;
  int m_codecFlags;
  VIDEOCODEC_FORMAT m_formats[VIDEOCODEC_FORMAT::MaxVideoFormats + 1];
  float m_displayAspect;
  unsigned int m_width, m_height;

  void * m_lastPictureBuffer;

  std::map<void *,CVideoBuffer *> m_map;
};
