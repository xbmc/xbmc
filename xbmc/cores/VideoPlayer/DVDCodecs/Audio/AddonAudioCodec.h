/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDAudioCodec.h"
#include "addons/AddonProvider.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/AudioCodec.h"

class BufferPool;

class CAddonAudioCodec : public CDVDAudioCodec, public ADDON::IAddonInstanceHandler
{
public:
  CAddonAudioCodec(CProcessInfo& processInfo,
                   ADDON::AddonInfoPtr& addonInfo,
                   KODI_HANDLE parentInstance);
  ~CAddonAudioCodec() override;

  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  bool AddData(const DemuxPacket& packet) override;
  void GetData(DVDAudioFrame& frame) override;
  void Reset() override;

  AEAudioFormat GetFormat() override;
  std::string GetName() override;

  void Dispose() override {}

private:
  bool CopyToInitData(AUDIOCODEC_INITDATA& initData, CDVDStreamInfo& hints);

  /*!
   * @brief All frame members can be expected to be set correctly except decodedData and pts.
   * GetFrameBuffer has to set decodedData to a valid memory address and return true.
   * In case buffer allocation fails, return false.
   */
  bool GetFrameBuffer(AUDIOCODEC_FRAME& frame);
  void ReleaseFrameBuffer(KODI_HANDLE audioBufferHandle);

  static bool get_frame_buffer(void* kodiInstance, AUDIOCODEC_FRAME* frame);
  static void release_frame_buffer(void* kodiInstance, KODI_HANDLE audioBufferHandle);

  AEAudioFormat m_format{};
  int32_t m_audioChannels = 0;
};
