/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaBackendWasm.h"

#include "platform/wasm/MseBridge.h"

#include <deque>
#include <memory>
#include <string>
#include <vector>

struct AVCodecContext;
struct AVCodecParameters;
struct AVFormatContext;
struct AVAudioFifo;
struct AVFrame;
struct AVPacket;
struct SwrContext;

/*!
 * \brief The browser's Media Source Extensions: each track is muxed into fragmented MP4
 * and appended to a SourceBuffer of a <video> element. Audio the browser cannot play is
 * converted to FLAC, or to Opus where FLAC does not play.
 */
class CMediaBackendMse final : public IMediaBackendWasm
{
public:
  CMediaBackendMse();
  ~CMediaBackendMse() override;

  std::string Name() const override { return "mse"; }
  bool SupportsVideo(const CDVDStreamInfo& hint) override;
  AudioPath GetAudioPath(const CDVDStreamInfo& hint) override;

  std::string Open(const CDVDStreamInfo& video,
                   const CDVDStreamInfo* audio,
                   AudioPath audioPath,
                   double duration) override;
  void Close() override;
  std::string VideoCodecName() const override { return "mse-" + m_video.codec; }
  std::string AudioCodecName() const override { return "mse-" + m_audio.codec; }
  double MaxAudioAhead() const override { return WASM_MEDIA::MAX_FEED_AHEAD; }

  bool PrepareVideo(DemuxPacket& packet, Packet& out) override;
  AppendResult AppendVideo(const Packet& packet) override;
  AppendResult AppendAudio(const Packet& packet) override;
  void EndVideo() override;
  void EndAudio() override;

  void ConvertAudio(const DVDAudioFrame& frame) override;
  void FinishConvertedAudio() override;
  bool PeekConvertedAudio(Packet& out) override;
  void PopConvertedAudio() override;
  void Flush() override;

  void Play() override;
  void Pause() override;
  void SetRate(double rate) override;
  bool HasVolume() const override { return true; }
  void SetVolume(float volume) override;
  void Seek(double seconds) override;
  void SetStartPosition(double seconds) override;

  int Error() const override;
  bool CanPlay() const override;
  bool Ended() const override;
  double Position() const override;

  //! Writes one track as fragmented MP4: an initialisation segment, then a fragment per
  //! packet. A packet is held back until the next one gives its duration, so that the
  //! samples follow on without gaps.
  class CFragmentWriter
  {
  public:
    ~CFragmentWriter();
    bool Open(const AVCodecParameters& parameters);
    void Close();
    //! Starts a new initialisation segment, for data that does not follow on.
    bool Restart();
    //! Adds the bytes of the packet before \p packet, after the initialisation segment if
    //! it is the first.
    bool Write(const Packet& packet, std::vector<uint8_t>& out);
    //! Adds the bytes of the packet held back.
    bool Finish(std::vector<uint8_t>& out);

  private:
    static int WriteBytes(void* opaque, const uint8_t* data, int size);
    bool Start();
    void Stop();
    bool WriteSample(AVPacket* sample, std::vector<uint8_t>& out);

    AVFormatContext* m_context{nullptr};
    AVCodecParameters* m_parameters{nullptr};
    std::vector<uint8_t> m_bytes;
    AVPacket* m_held{nullptr};
    int64_t m_lastDts{0};
    bool m_started{false};
  };

  //! Encodes decoded audio as FLAC or Opus.
  class CAudioEncoder
  {
  public:
    ~CAudioEncoder();
    //! \param highResolution FLAC only: 24 instead of 16 bits per sample.
    bool Open(AVCodecID codec, int channels, int sampleRate, bool highResolution);
    void Close();
    //! Drops what is held and starts over, for a new position.
    bool Reset();
    const AVCodecContext* Context() const { return m_context; }
    void Add(const DVDAudioFrame& frame);
    //! Encodes what is held back for a full frame and drains the encoder.
    void Finish();
    bool Peek(Packet& out) const;
    void Pop();

  private:
    void Encode(AVFrame* frame);

    struct EncodedFrame
    {
      std::vector<uint8_t> data;
      double pts;
      double duration;
    };

    AVCodecID m_codec{AV_CODEC_ID_NONE};
    int m_channels{0};
    int m_sampleRate{0};
    bool m_highResolution{false};
    AVCodecContext* m_context{nullptr};
    SwrContext* m_resample{nullptr};
    int m_resampleFormat{-1};
    int m_resampleChannels{0};
    int m_resampleRate{0};
    AVAudioFifo* m_fifo{nullptr};
    AVFrame* m_frame{nullptr};
    AVPacket* m_packet{nullptr};
    double m_startPts{-1.0};
    int64_t m_samples{0};
    std::deque<EncodedFrame> m_encoded;
  };

private:
  struct Track
  {
    bool video{false};
    std::string codec;
    CFragmentWriter writer;
    //! Muxed but not taken by the bridge yet.
    std::vector<uint8_t> pending;
    double lastPts{-1.0};
    double lastDuration{0.0};
  };

  bool AddTrack(Track& track, const std::string& mime);
  AppendResult Write(Track& track, const Packet& packet);
  AppendResult SendPending(Track& track);
  int32_t Session(const Track& track) const;

  MseState m_state{};
  int m_handle{-1};
  Track m_video;
  Track m_audio;
  bool m_withAudio{false};
  AVCodecID m_videoCodec{AV_CODEC_ID_NONE};
  AVCodecID m_audioCodec{AV_CODEC_ID_NONE};
  int m_nalLengthSize{0};
  bool m_demuxerMarksKeyFrames{false};
  std::unique_ptr<CAudioEncoder> m_encoder;
};
