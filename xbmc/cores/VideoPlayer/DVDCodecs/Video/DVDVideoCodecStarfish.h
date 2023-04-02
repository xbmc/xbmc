/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "threads/SingleLock.h"
#include "threads/Thread.h"
#include "utils/Geometry.h"

#include <starfish-media-pipeline/StarfishMediaAPIs.h>

class CBitstreamConverter;

class CStarfishVideoBuffer : public CVideoBuffer
{
public:
  explicit CStarfishVideoBuffer(int id) : CVideoBuffer(id) {}
  AVPixelFormat GetFormat() override { return AV_PIX_FMT_NONE; }
};

class CDVDVideoCodecStarfish : public CDVDVideoCodec
{
public:
  explicit CDVDVideoCodecStarfish(CProcessInfo& processInfo);
  ~CDVDVideoCodecStarfish() override;

  // registration
  static std::unique_ptr<CDVDVideoCodec> Create(CProcessInfo& processInfo);
  static bool Register();

  // required overrides
  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  bool AddData(const DemuxPacket& packet) override;
  void Reset() override;
  bool Reconfigure(CDVDStreamInfo& hints) override;
  VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  const char* GetName() override { return m_formatname.c_str(); }
  void SetCodecControl(int flags) override;
  unsigned GetAllowedReferences() override;
  void SetSpeed(int iSpeed) override;

private:
  void Dispose();
  void SetHDR();
  void UpdateFpsDuration();

  void PlayerCallback(const int32_t type, const int64_t numValue, const char* strValue);
  static void PlayerCallback(const int32_t type,
                             const int64_t numValue,
                             const char* strValue,
                             void* data);
  std::unique_ptr<StarfishMediaAPIs> m_starfishMediaAPI;

  CDVDStreamInfo m_hints;
  std::string m_codecname;
  std::string m_formatname{"starfish"};
  bool m_opened{false};
  int m_codecControlFlags;
  int64_t m_currentPlaytime{0};

  enum class StarfishState
  {
    RESET,
    FLUSHED,
    RUNNING,
  };
  StarfishState m_state{StarfishState::FLUSHED};

  VideoPicture m_videobuffer;
  std::unique_ptr<CBitstreamConverter> m_bitstream;

  static std::atomic<bool> ms_instanceGuard;
};
