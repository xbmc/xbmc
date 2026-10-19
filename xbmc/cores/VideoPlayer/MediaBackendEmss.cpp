/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaBackendEmss.h"

#include "Interface/DemuxPacket.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/BitstreamConverter.h"
#include "utils/log.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <utility>

#include <emscripten/threading.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

using namespace std::chrono_literals;

namespace
{
constexpr int CONVERTED_STREAM_ID = -2000;

constexpr auto ATTACH_TIMEOUT = 2000ms;
constexpr auto OPEN_TIMEOUT = 2000ms;
constexpr auto SEEK_TIMEOUT = 2000ms;

// EMSS accepts only S16 for PCM input.
constexpr const char* PCM_MIME = "audio/webm; codecs=\"pcm\"";
constexpr const char* PCM_SAMPLE_FORMAT = "SampleFormatS16";
// Decoded frames are joined up to this length; TrueHD decodes under a millisecond at a time.
constexpr double MIN_PCM_DURATION = 0.02;
// Samsung limits each audio track to 768 KiB. Reserve space for packet submission.
constexpr size_t AUDIO_BUFFER_BYTES = 768 * 1024;
constexpr size_t MAX_PCM_PACKET_BYTES = AUDIO_BUFFER_BYTES / 4;

// TizenEmssState is written by the browser main thread.
int32_t Read(const int32_t& field)
{
  return std::atomic_ref<int32_t>(const_cast<int32_t&>(field)).load();
}

std::string VideoMime(const CDVDStreamInfo& hint)
{
  const std::string codec = WASM_MEDIA::VideoCodecString(hint);
  if (codec.empty())
    return {};
  return fmt::format("video/{}; codecs=\"{}\"", hint.codec == AV_CODEC_ID_VP9 ? "webm" : "mp4",
                     codec);
}

std::string AudioMime(const CDVDStreamInfo& hint)
{
  switch (hint.codec)
  {
    case AV_CODEC_ID_AAC:
      if (hint.profile == AV_PROFILE_AAC_HE_V2)
        return "audio/mp4; codecs=\"mp4a.40.29\"";
      if (hint.profile == AV_PROFILE_AAC_HE)
        return "audio/mp4; codecs=\"mp4a.40.5\"";
      return "audio/mp4; codecs=\"mp4a.40.2\"";
    case AV_CODEC_ID_AC3:
      return "audio/mp4; codecs=\"ac-3\"";
    case AV_CODEC_ID_EAC3:
      return "audio/mp4; codecs=\"ec-3\"";
    case AV_CODEC_ID_FLAC:
      return "audio/mp4; codecs=\"flac\"";
    case AV_CODEC_ID_OPUS:
      return "audio/webm; codecs=\"opus\"";
    default:
      return {};
  }
}

const char* SampleFormat(AVCodecID codec)
{
  switch (codec)
  {
    case AV_CODEC_ID_AC3:
      return "SampleFormatAc3";
    case AV_CODEC_ID_EAC3:
      return "SampleFormatEac3";
    default:
      return "SampleFormatPlanarF32";
  }
}

uint64_t StreamLayout(const CDVDStreamInfo& hint)
{
  if (hint.channellayout)
    return hint.channellayout;
  AVChannelLayout layout{};
  av_channel_layout_default(&layout, hint.channels);
  const uint64_t mask = layout.order == AV_CHANNEL_ORDER_NATIVE ? layout.u.mask : 0;
  av_channel_layout_uninit(&layout);
  return mask;
}

const char* ChannelLayout(uint64_t mask)
{
  static constexpr std::pair<uint64_t, const char*> layouts[] = {
      {AV_CH_LAYOUT_MONO, "ChannelLayoutMono"},
      {AV_CH_LAYOUT_STEREO, "ChannelLayoutStereo"},
      {AV_CH_LAYOUT_2POINT1, "ChannelLayout2Point1"},
      {AV_CH_LAYOUT_SURROUND, "ChannelLayoutSurround"},
      {AV_CH_LAYOUT_2_1, "ChannelLayout2_1"},
      {AV_CH_LAYOUT_4POINT0, "ChannelLayout4_0"},
      {AV_CH_LAYOUT_2_2, "ChannelLayout2_2"},
      {AV_CH_LAYOUT_QUAD, "ChannelLayoutQuad"},
      {AV_CH_LAYOUT_3POINT1, "ChannelLayout3_1"},
      {AV_CH_LAYOUT_5POINT0, "ChannelLayout5_0"},
      {AV_CH_LAYOUT_5POINT0_BACK, "ChannelLayout5_0Back"},
      {AV_CH_LAYOUT_4POINT1, "ChannelLayout4_1"},
      {AV_CH_LAYOUT_5POINT1, "ChannelLayout5_1"},
      {AV_CH_LAYOUT_5POINT1_BACK, "ChannelLayout5_1Back"},
      {AV_CH_LAYOUT_6POINT0, "ChannelLayout6_0"},
      {AV_CH_LAYOUT_6POINT0_FRONT, "ChannelLayout6_0Front"},
      {AV_CH_LAYOUT_HEXAGONAL, "ChannelLayoutHexagonal"},
      {AV_CH_LAYOUT_6POINT1, "ChannelLayout6_1"},
      {AV_CH_LAYOUT_6POINT1_BACK, "ChannelLayout6_1Back"},
      {AV_CH_LAYOUT_6POINT1_FRONT, "ChannelLayout6_1Front"},
      {AV_CH_LAYOUT_7POINT0, "ChannelLayout7_0"},
      {AV_CH_LAYOUT_7POINT0_FRONT, "ChannelLayout7_0Front"},
      {AV_CH_LAYOUT_7POINT1, "ChannelLayout7_1"},
      {AV_CH_LAYOUT_7POINT1_WIDE, "ChannelLayout7_1Wide"},
      {AV_CH_LAYOUT_7POINT1_WIDE_BACK, "ChannelLayout7_1WideBack"},
      {AV_CH_LAYOUT_OCTAGONAL, "ChannelLayoutOctagonal"},
  };
  for (const auto& [layout, name] : layouts)
  {
    if (mask == layout)
      return name;
  }
  return nullptr;
}

const char* DecoderName(int32_t mode)
{
  switch (mode)
  {
    case TIZEN_EMSS_DECODE_HARDWARE:
      return "hardware";
    case TIZEN_EMSS_DECODE_SOFTWARE:
      return "software";
    default:
      return "unknown";
  }
}

bool WaitForState(TizenEmssState& state,
                  const std::function<bool()>& ready,
                  std::chrono::milliseconds timeout)
{
  const auto end = std::chrono::steady_clock::now() + timeout;
  while (!ready())
  {
    if (Read(state.error) != TIZEN_EMSS_NO_ERROR || std::chrono::steady_clock::now() >= end)
      return false;
    const int32_t seen = Read(state.signal);
    if (ready())
      return true;
    emscripten_futex_wait(&state.signal, static_cast<uint32_t>(seen), 50);
  }
  return true;
}

// For firmware that cannot validate a configuration: whether the TV takes the track.
bool TryTrack(const std::function<int(int handle)>& addTrack)
{
  TizenEmssState state{};
  const int handle = tizen_emss_create(&state);
  if (handle < 0)
    return false;
  const bool added = WaitForState(
                         state, [&state] { return Read(state.readyState) == TIZEN_EMSS_CLOSED; },
                         ATTACH_TIMEOUT) &&
                     addTrack(handle) >= 0;
  tizen_emss_destroy(handle);
  return added;
}

bool FindPcmConfig(const CDVDStreamInfo& hint, uint64_t& outputLayout, int& outputRate)
{
  const uint64_t inputLayout = StreamLayout(hint);
  const int inputRate = hint.samplerate > 0 ? hint.samplerate : 48000;
  for (const uint64_t mask : {inputLayout, uint64_t{AV_CH_LAYOUT_STEREO}})
  {
    const char* layout = ChannelLayout(mask);
    if (!layout)
      continue;
    for (const int rate : {inputRate, 48000})
    {
      int supported = tizen_emss_validate_audio(PCM_MIME, PCM_SAMPLE_FORMAT, layout, rate);
      if (supported < 0)
        supported = TryTrack(
            [&](int handle)
            {
              return tizen_emss_add_audio_track(handle, PCM_MIME, nullptr, 0, PCM_SAMPLE_FORMAT,
                                                layout, rate);
            });
      if (supported > 0)
      {
        outputLayout = mask;
        outputRate = rate;
        return true;
      }
    }
  }
  return false;
}

void FrameRate(const CDVDStreamInfo& hint, int& rate, int& scale)
{
  const bool known = hint.fpsrate > 0 && hint.fpsscale > 0;
  rate = known ? hint.fpsrate : 30;
  scale = known ? hint.fpsscale : 1;
}
} // namespace

CMediaBackendEmss::CMediaBackendEmss()
{
  const int size = tizen_emss_get_capabilities(nullptr, 0);
  if (size > 0)
  {
    std::vector<char> capabilities(size + 1);
    tizen_emss_get_capabilities(capabilities.data(), static_cast<int>(capabilities.size()));
    CLog::LogF(LOGINFO, "EMSS capabilities: {}", capabilities.data());
  }
}

std::string CMediaBackendEmss::VideoCodecName() const
{
  return fmt::format("tizen-{}-{}", DecoderName(Read(m_state.videoDecodingMode)), m_videoMime);
}

std::string CMediaBackendEmss::AudioCodecName() const
{
  return fmt::format("tizen-{}-{}", DecoderName(Read(m_state.audioDecodingMode)), m_audioMime);
}

bool CMediaBackendEmss::IsHardwareVideoDecoder() const
{
  return Read(m_state.videoDecodingMode) == TIZEN_EMSS_DECODE_HARDWARE;
}

CMediaBackendEmss::~CMediaBackendEmss()
{
  Close();
}

bool CMediaBackendEmss::SupportsVideo(const CDVDStreamInfo& hint)
{
  switch (hint.codec)
  {
    case AV_CODEC_ID_H264:
    case AV_CODEC_ID_HEVC:
    case AV_CODEC_ID_VP9:
    case AV_CODEC_ID_AV1:
      break;
    default:
      return false;
  }
  if (hint.width <= 0 || hint.height <= 0)
    return false;
  // The TV has no Dolby Vision decoder, and profile 5 has no base layer it can show.
  if (hint.dovi.dv_profile != 0 && hint.dovi.dv_bl_signal_compatibility_id == 0)
    return false;

  int rate = 0;
  int scale = 0;
  FrameRate(hint, rate, scale);
  const std::string mime = VideoMime(hint);
  int supported = tizen_emss_validate_video(mime.c_str(), hint.width, hint.height, rate, scale);
  if (supported < 0)
    supported = TryTrack(
        [&](int handle)
        {
          return tizen_emss_add_video_track(handle, mime.c_str(), hint.extradata.GetData(),
                                            static_cast<int>(hint.extradata.GetSize()), hint.width,
                                            hint.height, rate, scale);
        });
  CLog::LogF(LOGDEBUG, "{} {}x{} {}/{}: {}", mime, hint.width, hint.height, rate, scale, supported);
  return supported != 0;
}

IMediaBackendWasm::AudioPath CMediaBackendEmss::GetAudioPath(const CDVDStreamInfo& hint)
{
  const int samplerate = hint.samplerate > 0 ? hint.samplerate : 48000;
  const char* layout = ChannelLayout(StreamLayout(hint));
  const std::string mime = AudioMime(hint);
  if (!mime.empty() && layout)
  {
    int supported =
        tizen_emss_validate_audio(mime.c_str(), SampleFormat(hint.codec), layout, samplerate);
    if (supported < 0)
      supported = TryTrack(
          [&](int handle)
          {
            return tizen_emss_add_audio_track(handle, mime.c_str(), hint.extradata.GetData(),
                                              static_cast<int>(hint.extradata.GetSize()),
                                              SampleFormat(hint.codec), layout, samplerate);
          });
    CLog::LogF(LOGDEBUG, "{} {} channels {} Hz: {}", mime, hint.channels, samplerate, supported);
    if (supported)
      return AudioPath::NATIVE;
  }

  if (hint.channels < 1 || !avcodec_find_decoder(hint.codec))
    return AudioPath::NONE;
  uint64_t outputLayout = 0;
  int outputRate = 0;
  return FindPcmConfig(hint, outputLayout, outputRate) ? AudioPath::CONVERTED : AudioPath::NONE;
}

std::string CMediaBackendEmss::Open(const CDVDStreamInfo& video,
                                    const CDVDStreamInfo* audio,
                                    AudioPath audioPath,
                                    double duration)
{
  m_state = TizenEmssState{};
  m_handle = tizen_emss_create(&m_state);
  if (m_handle < 0)
  {
    CLog::LogF(LOGERROR, "Could not create the media source");
    return {};
  }
  // Tracks are added once the source is attached to its media element.
  if (!WaitForState(
          m_state, [this] { return Read(m_state.readyState) == TIZEN_EMSS_CLOSED; },
          ATTACH_TIMEOUT))
  {
    CLog::LogF(LOGERROR, "The media source was not attached (state {}, error {})",
               Read(m_state.readyState), Read(m_state.error));
    Close();
    return {};
  }

  int rate = 0;
  int scale = 0;
  FrameRate(video, rate, scale);
  m_videoMime = VideoMime(video);
  m_videoTrack = tizen_emss_add_video_track(
      m_handle, m_videoMime.c_str(), video.extradata.GetData(),
      static_cast<int>(video.extradata.GetSize()), video.width, video.height, rate, scale);

  m_audioMime.clear();
  m_maxAudioAhead = WASM_MEDIA::MAX_FEED_AHEAD;
  if (audio)
  {
    const int samplerate = audio->samplerate > 0 ? audio->samplerate : 48000;
    if (audioPath == AudioPath::CONVERTED)
    {
      if (!FindPcmConfig(*audio, m_pcmLayout, m_pcmRate))
      {
        Close();
        return {};
      }
      AVChannelLayout layout{};
      av_channel_layout_from_mask(&layout, m_pcmLayout);
      m_pcmFrameSize = layout.nb_channels * sizeof(int16_t);
      av_channel_layout_uninit(&layout);
      m_pcmPacketSize = std::min(static_cast<size_t>(m_pcmRate * MIN_PCM_DURATION),
                                 MAX_PCM_PACKET_BYTES / m_pcmFrameSize) *
                        m_pcmFrameSize;
      m_audioMime = "pcm";
      m_audioTrack = tizen_emss_add_audio_track(m_handle, PCM_MIME, nullptr, 0, PCM_SAMPLE_FORMAT,
                                                ChannelLayout(m_pcmLayout), m_pcmRate);
      m_maxAudioAhead =
          std::min(m_maxAudioAhead,
                   AUDIO_BUFFER_BYTES * 0.75 / (m_pcmFrameSize * static_cast<double>(m_pcmRate)));
      CLog::LogF(LOGINFO, "PCM output: S16, {}, {} Hz, {:.3f}s ahead, {} byte packets",
                 ChannelLayout(m_pcmLayout), m_pcmRate, m_maxAudioAhead, m_pcmPacketSize);
    }
    else
    {
      m_audioMime = AudioMime(*audio);
      m_audioTrack = tizen_emss_add_audio_track(
          m_handle, m_audioMime.c_str(), audio->extradata.GetData(),
          static_cast<int>(audio->extradata.GetSize()), SampleFormat(audio->codec),
          ChannelLayout(StreamLayout(*audio)), samplerate);
    }
  }

  if (m_videoTrack < 0 || (audio && m_audioTrack < 0))
  {
    CLog::LogF(LOGERROR, "The TV refused the tracks: {}, {}", m_videoMime, m_audioMime);
    Close();
    return {};
  }

  tizen_emss_set_duration(m_handle, duration);
  if (tizen_emss_open(m_handle) < 0 || !WaitForState(
                                           m_state,
                                           [this, audio]
                                           {
                                             return Read(m_state.readyState) == TIZEN_EMSS_OPEN &&
                                                    Read(m_state.videoOpen) &&
                                                    (!audio || Read(m_state.audioOpen));
                                           },
                                           OPEN_TIMEOUT))
  {
    CLog::LogF(LOGERROR, "The media source did not open (state {}, error {})",
               Read(m_state.readyState), Read(m_state.error));
    Close();
    return {};
  }

  CLog::LogF(LOGINFO, "Video decoder: {}; audio decoder: {}", VideoCodecName(), AudioCodecName());

  m_videoCodec = video.codec;
  SetupBitstreamConverter(video);
  m_demuxerMarksKeyFrames = false;
  m_pcm.clear();
  m_pcmOffset = 0;
  m_pcmClock = 0.0;
  m_pcmFinished = false;
  return audio ? fmt::format("{} with {}", m_videoMime, m_audioMime) : m_videoMime;
}

void CMediaBackendEmss::Close()
{
  if (m_handle >= 0)
    tizen_emss_destroy(m_handle);
  m_handle = -1;
  m_videoTrack = -1;
  m_audioTrack = -1;
  m_bitstream.reset();
  swr_free(&m_resample);
  m_pcm.clear();
  m_pcmOffset = 0;
}

void CMediaBackendEmss::SetupBitstreamConverter(const CDVDStreamInfo& hint)
{
  m_bitstream.reset();
  if ((hint.codec != AV_CODEC_ID_H264 && hint.codec != AV_CODEC_ID_HEVC) || !hint.extradata)
    return;

  m_bitstream = std::make_unique<CBitstreamConverter>();
  // Open() copies the extradata.
  if (!m_bitstream->Open(hint.codec, const_cast<uint8_t*>(hint.extradata.GetData()),
                         static_cast<int>(hint.extradata.GetSize()), true))
  {
    m_bitstream.reset();
    return;
  }

  // The TV plays the base layer.
  if (hint.codec == AV_CODEC_ID_HEVC && hint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
    m_bitstream->SetRemoveDovi(true);
}

bool CMediaBackendEmss::PrepareVideo(DemuxPacket& packet, Packet& out)
{
  const uint8_t* data = packet.pData;
  size_t size = static_cast<size_t>(packet.iSize);
  if (m_bitstream)
  {
    // A packet offered again was converted already.
    if (packet.iStreamId != CONVERTED_STREAM_ID)
    {
      m_bitstream->Convert(packet.pData, packet.iSize);
      packet.iStreamId = CONVERTED_STREAM_ID;
    }
    if (!m_bitstream->CanStartDecode())
      return false;
    data = m_bitstream->GetConvertBuffer();
    size = m_bitstream->GetConvertSize();
  }

  if (packet.m_keyFrame)
    m_demuxerMarksKeyFrames = true;
  out.data = data;
  out.size = size;
  out.keyFrame = m_demuxerMarksKeyFrames ? packet.m_keyFrame
                                         : WASM_MEDIA::IsKeyFrame(m_videoCodec, data, size, 0);
  return true;
}

IMediaBackendWasm::AppendResult CMediaBackendEmss::Append(int track,
                                                          int32_t sessionId,
                                                          const Packet& packet)
{
  switch (tizen_emss_append(track, packet.data, static_cast<int>(packet.size), packet.pts,
                            packet.dts, packet.duration, packet.keyFrame, sessionId, 0, 0))
  {
    case TIZEN_EMSS_APPEND_OK:
      return AppendResult::OK;
    case TIZEN_EMSS_APPEND_BUFFER_FULL:
      return AppendResult::FULL;
    case TIZEN_EMSS_APPEND_IGNORED:
      return AppendResult::STALE;
    case TIZEN_EMSS_APPEND_NEEDS_KEYFRAME:
      return AppendResult::NEEDS_KEY_FRAME;
    default:
      return AppendResult::FAILED;
  }
}

IMediaBackendWasm::AppendResult CMediaBackendEmss::AppendVideo(const Packet& packet)
{
  return Append(m_videoTrack, Read(m_state.videoSessionId), packet);
}

IMediaBackendWasm::AppendResult CMediaBackendEmss::AppendAudio(const Packet& packet)
{
  Packet audio = packet;
  audio.keyFrame = true;
  return Append(m_audioTrack, Read(m_state.audioSessionId), audio);
}

void CMediaBackendEmss::EndVideo()
{
  tizen_emss_append_end(m_videoTrack, Read(m_state.videoSessionId));
}

void CMediaBackendEmss::EndAudio()
{
  tizen_emss_append_end(m_audioTrack, Read(m_state.audioSessionId));
}

void CMediaBackendEmss::ConvertAudio(const DVDAudioFrame& frame)
{
  const uint64_t mask = CAEUtil::GetAVChannelLayout(frame.format.m_channelLayout);
  const int rate = static_cast<int>(frame.format.m_sampleRate);
  const AVSampleFormat format = CAEUtil::GetAVSampleFormat(frame.format.m_dataFormat);
  if (!m_resample || format != m_resampleFormat || mask != m_resampleLayout ||
      rate != m_resampleRate)
  {
    if (m_resample)
      ResampleAudio(nullptr, 0);
    swr_free(&m_resample);
    AVChannelLayout input{};
    AVChannelLayout output{};
    av_channel_layout_from_mask(&input, mask);
    av_channel_layout_from_mask(&output, m_pcmLayout);
    const int result = swr_alloc_set_opts2(&m_resample, &output, AV_SAMPLE_FMT_S16, m_pcmRate,
                                           &input, format, rate, 0, nullptr);
    av_channel_layout_uninit(&input);
    av_channel_layout_uninit(&output);
    if (result < 0 || swr_init(m_resample) < 0)
    {
      CLog::LogF(LOGERROR, "Cannot convert audio layout {} at {} Hz to PCM", mask, rate);
      swr_free(&m_resample);
      return;
    }
    m_resampleFormat = format;
    m_resampleLayout = mask;
    m_resampleRate = rate;
  }

  if (m_pcm.empty())
    m_pcmPts = frame.pts != DVD_NOPTS_VALUE
                   ? frame.pts / DVD_TIME_BASE -
                         static_cast<double>(swr_get_delay(m_resample, rate)) / rate
                   : m_pcmClock;
  ResampleAudio(frame.data, static_cast<int>(frame.nb_frames));
}

void CMediaBackendEmss::ResampleAudio(const uint8_t* const* data, int frames)
{
  if (m_pcmOffset)
  {
    m_pcm.erase(m_pcm.begin(), m_pcm.begin() + m_pcmOffset);
    m_pcmOffset = 0;
  }
  const int capacity = swr_get_out_samples(m_resample, frames);
  if (capacity <= 0)
    return;
  const size_t offset = m_pcm.size();
  m_pcm.resize(offset + static_cast<size_t>(capacity) * m_pcmFrameSize);
  uint8_t* out = m_pcm.data() + offset;
  const int samples = swr_convert(m_resample, &out, capacity, data, frames);
  m_pcm.resize(offset + static_cast<size_t>(std::max(0, samples)) * m_pcmFrameSize);
  m_pcmClock = m_pcmPts + static_cast<double>(m_pcm.size() / m_pcmFrameSize) / m_pcmRate;
}

void CMediaBackendEmss::FinishConvertedAudio()
{
  if (m_resample)
    ResampleAudio(nullptr, 0);
  m_pcmFinished = true;
}

bool CMediaBackendEmss::PeekConvertedAudio(Packet& out)
{
  const size_t remaining = m_pcm.size() - m_pcmOffset;
  if (remaining == 0 || (remaining < m_pcmPacketSize && !m_pcmFinished))
    return false;
  out.data = m_pcm.data() + m_pcmOffset;
  out.size = std::min(remaining, m_pcmPacketSize);
  out.pts = m_pcmPts;
  out.dts = m_pcmPts;
  out.duration = static_cast<double>(out.size / m_pcmFrameSize) / m_pcmRate;
  out.keyFrame = true;
  return true;
}

void CMediaBackendEmss::PopConvertedAudio()
{
  const size_t size = std::min(m_pcm.size() - m_pcmOffset, m_pcmPacketSize);
  m_pcmPts += static_cast<double>(size / m_pcmFrameSize) / m_pcmRate;
  m_pcmOffset += size;
  if (m_pcmOffset == m_pcm.size())
  {
    m_pcm.clear();
    m_pcmOffset = 0;
  }
}

void CMediaBackendEmss::Flush()
{
  if (m_bitstream)
    m_bitstream->ResetStartDecode();
  swr_free(&m_resample);
  m_pcm.clear();
  m_pcmOffset = 0;
  m_pcmPts = 0.0;
  m_pcmClock = 0.0;
  m_pcmFinished = false;
}

void CMediaBackendEmss::Play()
{
  tizen_emss_play(m_handle);
}

void CMediaBackendEmss::Pause()
{
  tizen_emss_pause(m_handle);
}

void CMediaBackendEmss::SetRate(double rate)
{
  tizen_emss_set_playback_rate(m_handle, rate);
}

void CMediaBackendEmss::Seek(double seconds)
{
  const int32_t videoSession = Read(m_state.videoSessionId);
  const int32_t audioSession = Read(m_state.audioSessionId);
  const bool withAudio = m_audioTrack >= 0;

  std::atomic_ref<double>(m_state.position).store(seconds);
  std::atomic_ref<int32_t>(m_state.ended).store(0);
  tizen_emss_seek(m_handle, seconds);

  // Packets of the old sessions are dropped, so wait for the new ones.
  if (!WaitForState(
          m_state,
          [&]
          {
            return Read(m_state.readyState) == TIZEN_EMSS_OPEN && Read(m_state.videoOpen) &&
                   Read(m_state.videoSessionId) != videoSession &&
                   (!withAudio ||
                    (Read(m_state.audioOpen) && Read(m_state.audioSessionId) != audioSession));
          },
          SEEK_TIMEOUT))
    CLog::LogF(LOGWARNING, "The TV did not finish seeking to {:.3f}s", seconds);
}

void CMediaBackendEmss::SetStartPosition(double seconds)
{
  std::atomic_ref<double>(m_state.position).store(seconds);
}

int CMediaBackendEmss::Error() const
{
  return Read(m_state.error);
}

bool CMediaBackendEmss::CanPlay() const
{
  return Read(m_state.canPlay) != 0;
}

bool CMediaBackendEmss::Ended() const
{
  return Read(m_state.ended) != 0;
}

double CMediaBackendEmss::Position() const
{
  return std::atomic_ref<double>(const_cast<double&>(m_state.position)).load();
}
