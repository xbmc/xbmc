/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/VideoPlayer/DVDMessage.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoPlayerAudio.h"

#include <memory>

#include <gtest/gtest.h>

namespace
{
constexpr int SAMPLE_RATE = 48000;
constexpr unsigned int FRAMES_PER_PACKET = SAMPLE_RATE / 10;

class CFakeAEStream : public IAEStream
{
public:
  ~CFakeAEStream() override = default;

  unsigned int GetSpace() override { return 0; }
  unsigned int AddData(const uint8_t* const* data,
                       unsigned int offset,
                       unsigned int frames,
                       ExtData* extData) override
  {
    return frames;
  }
  double GetDelay() override { return 0.0; }
  CAESyncInfo GetSyncInfo() override { return {}; }
  bool IsBuffering() override { return false; }
  double GetCacheTime() override { return 0.0; }
  double GetCacheTotal() override { return 1.0; }
  double GetMaxDelay() override { return 1.0; }
  void Pause() override {}
  void Hold() override {}
  void Resume() override {}
  void Drain(bool wait) override {}
  bool IsDraining() override { return false; }
  bool IsDrained() override { return true; }
  void Flush() override {}
  float GetVolume() override { return 1.0f; }
  void SetVolume(float volume) override {}
  float GetReplayGain() override { return 1.0f; }
  void SetReplayGain(float factor) override {}
  float GetAmplification() override { return 1.0f; }
  void SetAmplification(float amplify) override {}
  void SetFFmpegInfo(int profile,
                     enum AVMatrixEncoding matrix_encoding,
                     enum AVAudioServiceType audio_service_type) override
  {
  }
  unsigned int GetFrameSize() const override { return 8; }
  unsigned int GetChannelCount() const override { return 2; }
  unsigned int GetSampleRate() const override { return SAMPLE_RATE; }
  enum AEDataFormat GetDataFormat() const override { return AE_FMT_FLOAT; }
  bool HasSinkFormatChanged() const override { return true; }
  double GetResampleRatio() override { return 1.0; }
  void SetResampleRatio(double ratio) override {}
  void SetResampleMode(int mode) override {}
  void RegisterAudioCallback(IAudioCallback* pCallback) override {}
  void UnRegisterAudioCallback() override {}
  void RegisterSlave(IAEStream* stream) override {}
};

//! Every stream it makes reports that the sink was reopened for it.
class CFakeAE : public IAE
{
public:
  bool Suspend() override { return true; }
  bool Resume() override { return true; }
  float GetVolume() override { return 1.0f; }
  void SetVolume(const float volume) override {}
  void SetMute(const bool enabled) override {}
  bool IsMuted() override { return false; }
  StreamPtr MakeStream(AEAudioFormat& audioFormat,
                       unsigned int options,
                       IAEClockCallback* clock) override
  {
    return StreamPtr(new CFakeAEStream, IAEStreamDeleter(*this));
  }
  SoundPtr MakeSound(const std::string& file) override { return {}; }
  void EnumerateOutputDevices(AEDeviceList& devices, bool passthrough) override {}

protected:
  void Start() override {}

private:
  bool FreeStream(IAEStream* stream, bool finish) override
  {
    delete static_cast<CFakeAEStream*>(stream);
    return true;
  }
  void FreeSound(IAESound* sound) override {}
};

//! Decodes 100 ms of stereo float per call.
class CFakeAudioCodec : public CDVDAudioCodec
{
public:
  explicit CFakeAudioCodec(CProcessInfo& processInfo) : CDVDAudioCodec(processInfo) {}

  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override { return true; }
  void Dispose() override {}
  bool AddData(const DemuxPacket& packet) override { return true; }
  void GetData(DVDAudioFrame& frame) override
  {
    frame = DVDAudioFrame{};
    frame.nb_frames = FRAMES_PER_PACKET;
    frame.duration = DVD_MSEC_TO_TIME(100);
    frame.pts = DVD_NOPTS_VALUE;
    frame.format.m_dataFormat = AE_FMT_FLOAT;
    frame.format.m_sampleRate = SAMPLE_RATE;
    frame.format.m_channelLayout = AE_CH_LAYOUT_2_0;
    frame.bits_per_sample = 32;
  }
  void Reset() override {}
  AEAudioFormat GetFormat() override { return {}; }
  std::string GetName() override { return "fake"; }
};

class CTestVideoPlayerAudio : public CVideoPlayerAudio
{
public:
  CTestVideoPlayerAudio(CDVDClock& clock, CDVDMessageQueue& parent, CProcessInfo& processInfo)
    : CVideoPlayerAudio(&clock, parent, processInfo, 8.0)
  {
    m_pAudioCodec = std::make_unique<CFakeAudioCodec>(processInfo);
    m_streaminfo.samplerate = SAMPLE_RATE;
    m_syncState = IDVDStreamPlayer::SYNC_INSYNC;
  }

  void Play(int packets)
  {
    for (int i = 0; i < packets; ++i)
      ProcessDecoderOutput(m_frame);
  }

  //! What closing the stream for a track switch does to the sink
  void DestroySink() { m_audioSink.Destroy(false); }

private:
  DVDAudioFrame m_frame{};
};
} // namespace

class TestVideoPlayerAudio : public testing::Test
{
protected:
  TestVideoPlayerAudio()
  {
    CServiceBroker::RegisterAE(&m_ae);
    m_parent.Init();
  }
  ~TestVideoPlayerAudio() override { CServiceBroker::UnregisterAE(); }

  unsigned int FormatChanges()
  {
    return m_parent.GetPacketCount(CDVDMsg::PLAYER_AUDIO_FORMAT_CHANGE);
  }

  CFakeAE m_ae;
  CDVDClock m_clock;
  CDVDMessageQueue m_parent{"player"};
  std::unique_ptr<CProcessInfo> m_processInfo{CProcessInfo::CreateInstance()};
};

TEST_F(TestVideoPlayerAudio, StartOfPlaybackHolds)
{
  CTestVideoPlayerAudio audio(m_clock, m_parent, *m_processInfo);

  audio.OnPlaybackStart();
  audio.Play(3);

  EXPECT_EQ(1u, FormatChanges());
}

TEST_F(TestVideoPlayerAudio, NewSinkLaterInPlaybackDoesNotHold)
{
  CTestVideoPlayerAudio audio(m_clock, m_parent, *m_processInfo);

  audio.OnPlaybackStart();
  audio.Play(3);
  audio.DestroySink();
  audio.Play(3);

  EXPECT_EQ(1u, FormatChanges());
}

TEST_F(TestVideoPlayerAudio, StartKeepingItsSinkEndsTheStart)
{
  CTestVideoPlayerAudio audio(m_clock, m_parent, *m_processInfo);

  audio.OnPlaybackStart();
  audio.Play(3);
  audio.OnPlaybackStart();
  audio.Play(3);
  audio.DestroySink();
  audio.Play(3);

  EXPECT_EQ(1u, FormatChanges());
}

TEST_F(TestVideoPlayerAudio, EachPlaybackHoldsAtItsStart)
{
  CTestVideoPlayerAudio audio(m_clock, m_parent, *m_processInfo);

  audio.OnPlaybackStart();
  audio.Play(3);
  audio.DestroySink();
  audio.OnPlaybackStart();
  audio.Play(3);

  EXPECT_EQ(2u, FormatChanges());
}
