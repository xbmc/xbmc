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

#include <fmt/format.h>
#include <starfish-media-pipeline/StarfishMediaAPIs.h>

class CBitstreamConverter;

class CStarfishVideoBuffer : public CVideoBuffer
{
public:
  explicit CStarfishVideoBuffer(int id) : CVideoBuffer(id) {}
  AVPixelFormat GetFormat() override { return AV_PIX_FMT_NONE; }
  long m_acbId{0};
};

enum class StarfishState
{
  RESET,
  FLUSHED,
  RUNNING,
  EOS,
  ERROR,
  MAX,
};

template<>
struct fmt::formatter<StarfishState> : fmt::formatter<std::string_view>
{
public:
  template<typename FormatContext>
  constexpr auto format(const StarfishState& state, FormatContext& ctx)
  {
    const auto it = ms_stateMap.find(state);
    if (it == ms_stateMap.cend())
      throw std::range_error("no starfish state string found");

    return fmt::formatter<string_view>::format(it->second, ctx);
  }

private:
  static constexpr auto ms_stateMap = make_map<StarfishState, std::string_view>({
      {StarfishState::RESET, "Reset"},
      {StarfishState::FLUSHED, "Flushed"},
      {StarfishState::RUNNING, "Running"},
      {StarfishState::EOS, "EOS"},
      {StarfishState::ERROR, "Error"},
  });
  static_assert(static_cast<size_t>(StarfishState::MAX) == ms_stateMap.size(),
                "ms_stateMap doesn't match the size of StarfishState, did you forget to "
                "add/remove a mapping?");
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
  void SetSpeed(int iSpeed) override;

private:
  void Dispose();
  void SetHDR();
  void UpdateFpsDuration();
  bool OpenInternal(CDVDStreamInfo& hints, CDVDCodecOptions& options);

  void PlayerCallback(const int32_t type, const int64_t numValue, const char* strValue);
  static void PlayerCallback(const int32_t type,
                             const int64_t numValue,
                             const char* strValue,
                             void* data);
  static void AcbCallback(
      long acbId, long taskId, long eventType, long appState, long playState, const char* reply);
  std::unique_ptr<StarfishMediaAPIs> m_starfishMediaAPI;
  long m_acbId{0};

  CDVDStreamInfo m_hints;
  std::string m_codecname;
  std::string m_formatname{"starfish"};
  bool m_opened{false};
  int m_codecControlFlags;
  std::chrono::nanoseconds m_currentPlaytime{0};
  bool m_newFrame{false};

  StarfishState m_state{StarfishState::FLUSHED};

  VideoPicture m_videobuffer;
  std::unique_ptr<CBitstreamConverter> m_bitstream;

  static constexpr auto ms_codecMap = make_map<AVCodecID, std::string_view>({
      {AV_CODEC_ID_VP8, "VP8"},
      {AV_CODEC_ID_VP9, "VP9"},
      {AV_CODEC_ID_AVS, "H264"},
      {AV_CODEC_ID_CAVS, "H264"},
      {AV_CODEC_ID_H264, "H264"},
      {AV_CODEC_ID_HEVC, "H265"},
      {AV_CODEC_ID_AV1, "AV1"},
  });

  static constexpr auto ms_formatInfoMap = make_map<AVCodecID, std::string_view>({
      {AV_CODEC_ID_VP8, "starfish-vp8"},
      {AV_CODEC_ID_VP9, "starfish-vp9"},
      {AV_CODEC_ID_AVS, "starfish-h264"},
      {AV_CODEC_ID_CAVS, "starfish-h264"},
      {AV_CODEC_ID_H264, "starfish-h264"},
      {AV_CODEC_ID_HEVC, "starfish-h265"},
      {AV_CODEC_ID_AV1, "starfish-av1"},
  });

  static constexpr auto ms_hdrInfoMap = make_map<AVColorTransferCharacteristic, std::string_view>({
      {AVCOL_TRC_SMPTE2084, "HDR10"},
      {AVCOL_TRC_ARIB_STD_B67, "HLG"},
  });

  static std::atomic<bool> ms_instanceGuard;
};
