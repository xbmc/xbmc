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

// Audio the TV cannot decode is decoded by FFmpeg and handed to it in this form.
constexpr const char* PCM_MIME = "audio/webm; codecs=\"pcm\"";
constexpr const char* PCM_SAMPLE_FORMAT = "SampleFormatS16";
constexpr int PCM_MAX_CHANNELS = 8;
// Decoded frames are joined up to this length; TrueHD decodes under a millisecond at a time.
constexpr double MIN_PCM_DURATION = 0.02;
// Samsung's limit for the data buffered on an audio track, which PCM fills in about a second.
constexpr double AUDIO_BUFFER_BYTES = 768 * 1024;

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

const char* ChannelLayout(int channels)
{
  switch (channels)
  {
    case 1:
      return "ChannelLayoutMono";
    case 3:
      return "ChannelLayoutSurround";
    case 4:
      return "ChannelLayoutQuad";
    case 5:
      return "ChannelLayout5_0";
    case 6:
      return "ChannelLayout5_1";
    case 7:
      return "ChannelLayout6_1";
    case 8:
      return "ChannelLayout7_1";
    default:
      return "ChannelLayoutStereo";
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

void FrameRate(const CDVDStreamInfo& hint, int& rate, int& scale)
{
  const bool known = hint.fpsrate > 0 && hint.fpsscale > 0;
  rate = known ? hint.fpsrate : 30;
  scale = known ? hint.fpsscale : 1;
}
} // namespace

CMediaBackendEmss::CMediaBackendEmss() = default;

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
  const char* layout = ChannelLayout(hint.channels);
  const std::string mime = AudioMime(hint);
  if (!mime.empty())
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

  if (hint.channels < 1 || hint.channels > PCM_MAX_CHANNELS || !avcodec_find_decoder(hint.codec))
    return AudioPath::NONE;
  int pcm = tizen_emss_validate_audio(PCM_MIME, PCM_SAMPLE_FORMAT, layout, samplerate);
  if (pcm < 0)
    pcm = TryTrack(
        [&](int handle)
        {
          return tizen_emss_add_audio_track(handle, PCM_MIME, nullptr, 0, PCM_SAMPLE_FORMAT, layout,
                                            samplerate);
        });
  CLog::LogF(LOGDEBUG, "codec {} as PCM, {} channels {} Hz: {}", hint.codec, hint.channels,
             samplerate, pcm);
  return pcm ? AudioPath::CONVERTED : AudioPath::NONE;
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
      m_audioMime = "pcm";
      m_audioTrack = tizen_emss_add_audio_track(m_handle, PCM_MIME, nullptr, 0, PCM_SAMPLE_FORMAT,
                                                ChannelLayout(audio->channels), samplerate);
      m_maxAudioAhead = std::min(m_maxAudioAhead,
                                 AUDIO_BUFFER_BYTES * 0.75 / (audio->channels * 2.0 * samplerate));
    }
    else
    {
      m_audioMime = AudioMime(*audio);
      m_audioTrack = tizen_emss_add_audio_track(
          m_handle, m_audioMime.c_str(), audio->extradata.GetData(),
          static_cast<int>(audio->extradata.GetSize()), SampleFormat(audio->codec),
          ChannelLayout(audio->channels), samplerate);
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

  const int32_t decoding = Read(m_state.videoDecodingMode);
  CLog::LogF(LOGINFO, "{} is decoded {}", m_videoMime,
             decoding == TIZEN_EMSS_DECODE_HARDWARE   ? "in hardware"
             : decoding == TIZEN_EMSS_DECODE_SOFTWARE ? "in software"
                                                      : "by an unreported decoder");

  m_videoCodec = video.codec;
  SetupBitstreamConverter(video);
  m_demuxerMarksKeyFrames = false;
  m_pcm.clear();
  m_pcmDuration = 0.0;
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
  m_pcmDuration = 0.0;
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
  const int channels = static_cast<int>(frame.format.m_channelLayout.Count());
  const int rate = static_cast<int>(frame.format.m_sampleRate);
  const AVSampleFormat format = CAEUtil::GetAVSampleFormat(frame.format.m_dataFormat);
  if (!m_resample || format != m_resampleFormat || channels != m_resampleChannels ||
      rate != m_resampleRate)
  {
    swr_free(&m_resample);
    AVChannelLayout layout;
    av_channel_layout_default(&layout, channels);
    if (swr_alloc_set_opts2(&m_resample, &layout, AV_SAMPLE_FMT_S16, rate, &layout, format, rate, 0,
                            nullptr) < 0 ||
        swr_init(m_resample) < 0)
    {
      CLog::LogF(LOGERROR, "Cannot convert {} channels of format {} to PCM", channels,
                 static_cast<int>(format));
      swr_free(&m_resample);
      return;
    }
    m_resampleFormat = format;
    m_resampleChannels = channels;
    m_resampleRate = rate;
  }

  const size_t offset = m_pcm.size();
  m_pcm.resize(offset + static_cast<size_t>(frame.nb_frames) * channels * 2);
  uint8_t* out = m_pcm.data() + offset;
  const int samples = swr_convert(m_resample, &out, static_cast<int>(frame.nb_frames),
                                  reinterpret_cast<const uint8_t* const*>(frame.data),
                                  static_cast<int>(frame.nb_frames));
  if (samples <= 0 || rate <= 0)
  {
    m_pcm.resize(offset);
    return;
  }
  m_pcm.resize(offset + static_cast<size_t>(samples) * channels * 2);
  if (offset == 0)
    m_pcmPts = frame.pts != DVD_NOPTS_VALUE ? frame.pts / DVD_TIME_BASE : m_pcmClock;
  m_pcmDuration += static_cast<double>(samples) / rate;
  m_pcmClock = m_pcmPts + m_pcmDuration;
}

bool CMediaBackendEmss::PeekConvertedAudio(Packet& out)
{
  if (m_pcm.empty() || (m_pcmDuration < MIN_PCM_DURATION && !m_pcmFinished))
    return false;
  out.data = m_pcm.data();
  out.size = m_pcm.size();
  out.pts = m_pcmPts;
  out.dts = m_pcmPts;
  out.duration = m_pcmDuration;
  out.keyFrame = true;
  return true;
}

void CMediaBackendEmss::PopConvertedAudio()
{
  m_pcm.clear();
  m_pcmDuration = 0.0;
  m_pcmFinished = false;
}

void CMediaBackendEmss::Flush()
{
  if (m_bitstream)
    m_bitstream->ResetStartDecode();
  PopConvertedAudio();
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
