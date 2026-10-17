/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaBackendWasm.h"

#include "platform/wasm/TizenEmss.h"

#include <memory>
#include <string>
#include <vector>

class CBitstreamConverter;
struct SwrContext;

/*!
 * \brief Samsung's Tizen WASM Player: the TV decodes elementary streams and shows the
 * video on its own plane. Audio it refuses is converted to PCM.
 */
class CMediaBackendEmss final : public IMediaBackendWasm
{
public:
  CMediaBackendEmss();
  ~CMediaBackendEmss() override;

  std::string Name() const override { return "tizen"; }
  bool SupportsVideo(const CDVDStreamInfo& hint) override;
  AudioPath GetAudioPath(const CDVDStreamInfo& hint) override;

  std::string Open(const CDVDStreamInfo& video,
                   const CDVDStreamInfo* audio,
                   AudioPath audioPath,
                   double duration) override;
  void Close() override;
  std::string VideoCodecName() const override { return "tizen-" + m_videoMime; }
  std::string AudioCodecName() const override { return "tizen-" + m_audioMime; }
  double MaxAudioAhead() const override { return m_maxAudioAhead; }

  bool PrepareVideo(DemuxPacket& packet, Packet& out) override;
  AppendResult AppendVideo(const Packet& packet) override;
  AppendResult AppendAudio(const Packet& packet) override;
  void EndVideo() override;
  void EndAudio() override;

  void ConvertAudio(const DVDAudioFrame& frame) override;
  void FinishConvertedAudio() override { m_pcmFinished = true; }
  bool PeekConvertedAudio(Packet& out) override;
  void PopConvertedAudio() override;
  void Flush() override;

  void Play() override;
  void Pause() override;
  void SetRate(double rate) override;
  bool HasVolume() const override { return false; }
  void SetVolume(float volume) override {}
  void Seek(double seconds) override;
  void SetStartPosition(double seconds) override;

  int Error() const override;
  bool CanPlay() const override;
  bool Ended() const override;
  double Position() const override;

private:
  void SetupBitstreamConverter(const CDVDStreamInfo& hint);
  AppendResult Append(int track, int32_t sessionId, const Packet& packet);

  TizenEmssState m_state{};
  int m_handle{-1};
  int m_videoTrack{-1};
  int m_audioTrack{-1};
  std::string m_videoMime;
  std::string m_audioMime;
  double m_maxAudioAhead{0.0};

  AVCodecID m_videoCodec{AV_CODEC_ID_NONE};
  std::unique_ptr<CBitstreamConverter> m_bitstream;
  bool m_demuxerMarksKeyFrames{false};

  SwrContext* m_resample{nullptr};
  int m_resampleFormat{-1};
  int m_resampleChannels{0};
  int m_resampleRate{0};
  std::vector<uint8_t> m_pcm;
  double m_pcmPts{0.0};
  double m_pcmDuration{0.0};
  double m_pcmClock{0.0};
  bool m_pcmFinished{false};
};
