/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodec.h"
#include "addons/AddonProvider.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/VideoCodec.h"

class BufferPool;

class CAddonVideoCodec
  : public CDVDVideoCodec
  , public ADDON::IAddonInstanceHandler
{
public:
  CAddonVideoCodec(CProcessInfo& processInfo,
                   ADDON::AddonInfoPtr& addonInfo,
                   KODI_HANDLE parentInstance);
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
   * GetFrameBuffer has to set decodedData to a valid memory address and return true.
   * In case buffer allocation fails, return false.
   */
  bool GetFrameBuffer(VIDEOCODEC_PICTURE &picture);
  void ReleaseFrameBuffer(KODI_HANDLE videoBufferHandle);

  static bool get_frame_buffer(void* kodiInstance, VIDEOCODEC_PICTURE *picture);
  static void release_frame_buffer(void* kodiInstance, KODI_HANDLE videoBufferHandle);

  int m_codecFlags = 0;
  VIDEOCODEC_FORMAT m_formats[VIDEOCODEC_FORMAT_MAXFORMATS + 1];
  float m_displayAspect = 0.0f;
  unsigned int m_width, m_height;
};
