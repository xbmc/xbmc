/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaBackendMse.h"

#include "Interface/DemuxPacket.h"
#include "Interface/TimingConstants.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iterator>
#include <thread>

#include <emscripten/threading.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

using namespace std::chrono_literals;

namespace
{
constexpr auto OPEN_TIMEOUT = 2000ms;
// How long the end of a track waits for the bridge to take the last data.
constexpr auto END_TIMEOUT = 2000ms;
// Keeps the timestamps in the MP4 positive; the SourceBuffers take it off again.
constexpr double TIMESTAMP_OFFSET = 10.0;
constexpr int MAX_CONVERTED_CHANNELS = 8;
constexpr const char* FLAC_MIME = "audio/mp4; codecs=\"flac\"";
constexpr const char* OPUS_MIME = "audio/mp4; codecs=\"opus\"";
constexpr int OPUS_SAMPLE_RATE = 48000;
constexpr int OPUS_BITRATE_PER_CHANNEL = 64000;

// What audio the browser cannot play is converted to: lossless where possible.
AVCodecID ConvertedCodec()
{
  if (mse_is_type_supported(FLAC_MIME) == 1)
    return AV_CODEC_ID_FLAC;
  if (avcodec_find_encoder_by_name("libopus") && mse_is_type_supported(OPUS_MIME) == 1)
    return AV_CODEC_ID_OPUS;
  return AV_CODEC_ID_NONE;
}

// MseState is written by the browser main thread.
int32_t Read(const int32_t& field)
{
  return std::atomic_ref<int32_t>(const_cast<int32_t&>(field)).load();
}

bool WaitForOpen(MseState& state)
{
  const auto end = std::chrono::steady_clock::now() + OPEN_TIMEOUT;
  while (!Read(state.open))
  {
    if (Read(state.error) != MSE_NO_ERROR || std::chrono::steady_clock::now() >= end)
      return false;
    const int32_t seen = Read(state.signal);
    if (Read(state.open))
      return true;
    emscripten_futex_wait(&state.signal, static_cast<uint32_t>(seen), 50);
  }
  return true;
}

std::string AudioCodecString(const CDVDStreamInfo& hint)
{
  switch (hint.codec)
  {
    case AV_CODEC_ID_AAC:
      if (hint.profile == AV_PROFILE_AAC_HE_V2)
        return "mp4a.40.29";
      if (hint.profile == AV_PROFILE_AAC_HE)
        return "mp4a.40.5";
      return "mp4a.40.2";
    case AV_CODEC_ID_AC3:
      return "ac-3";
    case AV_CODEC_ID_EAC3:
      return "ec-3";
    case AV_CODEC_ID_OPUS:
      return "opus";
    case AV_CODEC_ID_FLAC:
      return "flac";
    default:
      return {};
  }
}

std::string Mime(bool video, const std::string& codec)
{
  return fmt::format("{}/mp4; codecs=\"{}\"", video ? "video" : "audio", codec);
}

// The size of the NAL unit lengths of avcC and hvcC extradata; 0 for Annex B.
int NalLengthSize(const CDVDStreamInfo& hint)
{
  const uint8_t* data = hint.extradata.GetData();
  const size_t size = hint.extradata.GetSize();
  if (size == 0 || data[0] != 1)
    return 0;
  if (hint.codec == AV_CODEC_ID_H264 && size >= 5)
    return (data[4] & 3) + 1;
  if (hint.codec == AV_CODEC_ID_HEVC && size >= 22)
    return (data[21] & 3) + 1;
  return 0;
}

void SetExtradata(const CDVDStreamInfo& hint, AVCodecParameters& parameters)
{
  const size_t size = hint.extradata.GetSize();
  if (size == 0)
    return;
  parameters.extradata = static_cast<uint8_t*>(av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE));
  if (!parameters.extradata)
    return;
  std::memcpy(parameters.extradata, hint.extradata.GetData(), size);
  parameters.extradata_size = static_cast<int>(size);
}

void FillVideoParameters(const CDVDStreamInfo& hint, AVCodecParameters& parameters)
{
  parameters.codec_type = AVMEDIA_TYPE_VIDEO;
  parameters.codec_id = hint.codec;
  parameters.width = hint.width;
  parameters.height = hint.height;
  parameters.profile = hint.profile;
  parameters.level = hint.level;
  // The vpcC box of VP9 takes the bit depth from the pixel format.
  parameters.format = hint.bitdepth > 8 ? AV_PIX_FMT_YUV420P10 : AV_PIX_FMT_YUV420P;
  parameters.color_range = hint.colorRange;
  parameters.color_primaries = hint.colorPrimaries;
  parameters.color_trc = hint.colorTransferCharacteristic;
  parameters.color_space = hint.colorSpace;
  SetExtradata(hint, parameters);
}

// The bits per sample of FLAC extradata: STREAMINFO, possibly after the stream marker and a
// block header.
int FlacBitsPerSample(const uint8_t* data, size_t size)
{
  if (size >= 8 && std::memcmp(data, "fLaC", 4) == 0)
  {
    data += 8;
    size -= 8;
  }
  if (size < 14)
    return 0;
  return (((data[12] & 1) << 4) | (data[13] >> 4)) + 1;
}

// An AudioSpecificConfig for ADTS streams, which carry theirs in every frame header.
void SetAacConfig(const CDVDStreamInfo& hint, AVCodecParameters& parameters)
{
  static constexpr int RATES[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                  22050, 16000, 12000, 11025, 8000,  7350};
  int rateIndex = 3;
  for (int i = 0; i < static_cast<int>(std::size(RATES)); ++i)
  {
    if (RATES[i] == parameters.sample_rate)
      rateIndex = i;
  }
  // ADTS signals HE-AAC implicitly, as AAC LC.
  const int objectType = hint.profile >= AV_PROFILE_AAC_MAIN && hint.profile <= AV_PROFILE_AAC_LTP
                             ? hint.profile + 1
                             : 2;
  const int channels = std::clamp(hint.channels, 1, 7);
  parameters.extradata = static_cast<uint8_t*>(av_mallocz(2 + AV_INPUT_BUFFER_PADDING_SIZE));
  if (!parameters.extradata)
    return;
  parameters.extradata[0] = static_cast<uint8_t>((objectType << 3) | (rateIndex >> 1));
  parameters.extradata[1] = static_cast<uint8_t>(((rateIndex & 1) << 7) | (channels << 3));
  parameters.extradata_size = 2;
}

// The length of the ADTS header of an AAC frame, 0 when it has none.
size_t AdtsHeaderSize(const uint8_t* data, size_t size)
{
  if (size < 7 || data[0] != 0xff || (data[1] & 0xf6) != 0xf0)
    return 0;
  return (data[1] & 1) ? 7 : 9;
}

void FillAudioParameters(const CDVDStreamInfo& hint, AVCodecParameters& parameters)
{
  parameters.codec_type = AVMEDIA_TYPE_AUDIO;
  parameters.codec_id = hint.codec;
  parameters.profile = hint.profile;
  parameters.sample_rate = hint.samplerate > 0 ? hint.samplerate : 48000;
  if (hint.channellayout)
    av_channel_layout_from_mask(&parameters.ch_layout, hint.channellayout);
  if (parameters.ch_layout.nb_channels != hint.channels)
  {
    av_channel_layout_uninit(&parameters.ch_layout);
    av_channel_layout_default(&parameters.ch_layout, hint.channels);
  }
  parameters.bit_rate = hint.bitrate;
  parameters.block_align = hint.blockalign;
  parameters.bits_per_coded_sample = hint.bitspersample;
  SetExtradata(hint, parameters);
  if (hint.codec == AV_CODEC_ID_AAC && !parameters.extradata)
    SetAacConfig(hint, parameters);
  // The MP4 sample entry of FLAC has to give the bits per sample of its STREAMINFO.
  if (hint.codec == AV_CODEC_ID_FLAC)
    parameters.bits_per_raw_sample =
        FlacBitsPerSample(hint.extradata.GetData(), hint.extradata.GetSize());
}
} // namespace

CMediaBackendMse::CMediaBackendMse()
{
  m_video.video = true;
}

CMediaBackendMse::~CMediaBackendMse()
{
  Close();
}

bool CMediaBackendMse::SupportsVideo(const CDVDStreamInfo& hint)
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
  // Browsers have no Dolby Vision decoder, and profile 5 has no base layer they can show.
  if (hint.dovi.dv_profile != 0 && hint.dovi.dv_bl_signal_compatibility_id == 0)
    return false;

  const std::string mime = Mime(true, WASM_MEDIA::VideoCodecString(hint));
  const bool supported = mse_is_type_supported(mime.c_str()) == 1;
  CLog::LogF(LOGDEBUG, "{}: {}", mime, supported);
  return supported;
}

IMediaBackendWasm::AudioPath CMediaBackendMse::GetAudioPath(const CDVDStreamInfo& hint)
{
  const std::string codec = AudioCodecString(hint);
  if (!codec.empty() && mse_is_type_supported(Mime(false, codec).c_str()) == 1)
    return AudioPath::NATIVE;

  const AVCodecID converted = ConvertedCodec();
  if (hint.channels < 1 || hint.channels > MAX_CONVERTED_CHANNELS ||
      !avcodec_find_decoder(hint.codec) || converted == AV_CODEC_ID_NONE)
    return AudioPath::NONE;
  CLog::LogF(LOGDEBUG, "codec {} as {}, {} channels", hint.codec, avcodec_get_name(converted),
             hint.channels);
  return AudioPath::CONVERTED;
}

std::string CMediaBackendMse::Open(const CDVDStreamInfo& video,
                                   const CDVDStreamInfo* audio,
                                   AudioPath audioPath,
                                   double duration)
{
  m_state = MseState{};
  m_handle = mse_create(&m_state);
  if (m_handle < 0)
  {
    CLog::LogF(LOGERROR, "Could not create the media source");
    return {};
  }
  if (!WaitForOpen(m_state))
  {
    CLog::LogF(LOGERROR, "The media source did not open (error {})", Read(m_state.error));
    Close();
    return {};
  }

  m_video.codec = WASM_MEDIA::VideoCodecString(video);
  AVCodecParameters* parameters = avcodec_parameters_alloc();
  if (!parameters)
  {
    Close();
    return {};
  }
  FillVideoParameters(video, *parameters);
  bool added = m_video.writer.Open(*parameters) && AddTrack(m_video, Mime(true, m_video.codec));
  avcodec_parameters_free(&parameters);

  m_withAudio = audio != nullptr;
  if (added && audio)
  {
    parameters = avcodec_parameters_alloc();
    m_audioCodec = AV_CODEC_ID_NONE;
    if (audioPath == AudioPath::CONVERTED)
    {
      const AVCodecID converted = ConvertedCodec();
      m_encoder = std::make_unique<CAudioEncoder>();
      added = parameters &&
              m_encoder->Open(converted, audio->channels,
                              audio->samplerate > 0 ? audio->samplerate : 48000,
                              audio->bitspersample > 16) &&
              avcodec_parameters_from_context(parameters, m_encoder->Context()) >= 0;
      m_audio.codec = converted == AV_CODEC_ID_FLAC ? "flac" : "opus";
    }
    else
    {
      if (parameters)
        FillAudioParameters(*audio, *parameters);
      m_audio.codec = AudioCodecString(*audio);
      m_audioCodec = audio->codec;
    }
    added = added && parameters && m_audio.writer.Open(*parameters) &&
            AddTrack(m_audio, Mime(false, m_audio.codec));
    avcodec_parameters_free(&parameters);
  }

  if (!added)
  {
    CLog::LogF(LOGERROR, "The browser refused the tracks: {}, {}", m_video.codec, m_audio.codec);
    Close();
    return {};
  }

  mse_set_duration(m_handle, duration);
  m_videoCodec = video.codec;
  m_nalLengthSize = NalLengthSize(video);
  m_demuxerMarksKeyFrames = false;
  const std::string videoMime = Mime(true, m_video.codec);
  return audio ? fmt::format("{} with {}", videoMime, Mime(false, m_audio.codec)) : videoMime;
}

bool CMediaBackendMse::AddTrack(Track& track, const std::string& mime)
{
  track.pending.clear();
  track.lastPts = -1.0;
  track.lastDuration = 0.0;
  return mse_add_track(m_handle, track.video, mime.c_str(), -TIMESTAMP_OFFSET) == 0;
}

void CMediaBackendMse::Close()
{
  if (m_handle >= 0)
    mse_destroy(m_handle);
  m_handle = -1;
  for (Track* track : {&m_video, &m_audio})
  {
    track->writer.Close();
    track->pending.clear();
  }
  m_encoder.reset();
  m_withAudio = false;
}

bool CMediaBackendMse::PrepareVideo(DemuxPacket& packet, Packet& out)
{
  if (packet.m_keyFrame)
    m_demuxerMarksKeyFrames = true;
  out.data = packet.pData;
  out.size = static_cast<size_t>(packet.iSize);
  out.keyFrame = m_demuxerMarksKeyFrames
                     ? packet.m_keyFrame
                     : WASM_MEDIA::IsKeyFrame(m_videoCodec, out.data, out.size, m_nalLengthSize);
  return true;
}

int32_t CMediaBackendMse::Session(const Track& track) const
{
  return Read(track.video ? m_state.videoSession : m_state.audioSession);
}

IMediaBackendWasm::AppendResult CMediaBackendMse::SendPending(Track& track)
{
  const int result = mse_append(m_handle, track.video, track.pending.data(),
                                static_cast<int>(track.pending.size()), Session(track));
  if (result == MSE_APPEND_FULL)
    return AppendResult::FULL;
  track.pending.clear();
  if (result == MSE_APPEND_OK)
    return AppendResult::OK;
  return result == MSE_APPEND_STALE ? AppendResult::STALE : AppendResult::FAILED;
}

IMediaBackendWasm::AppendResult CMediaBackendMse::Write(Track& track, const Packet& packet)
{
  if (!track.pending.empty() && SendPending(track) == AppendResult::FULL)
    return AppendResult::FULL;

  // A fragment of one packet needs its duration; demuxers do not always know it.
  Packet sample = packet;
  if (sample.duration <= 0.0)
    sample.duration = track.lastPts >= 0.0 && sample.pts > track.lastPts
                          ? sample.pts - track.lastPts
                          : track.lastDuration;
  track.lastPts = sample.pts;
  track.lastDuration = sample.duration;

  if (!track.writer.Write(sample, track.pending))
    return AppendResult::FAILED;
  // The packet is muxed; its bytes go once the bridge has room.
  const AppendResult result = SendPending(track);
  return result == AppendResult::FULL ? AppendResult::OK : result;
}

IMediaBackendWasm::AppendResult CMediaBackendMse::AppendVideo(const Packet& packet)
{
  return Write(m_video, packet);
}

IMediaBackendWasm::AppendResult CMediaBackendMse::AppendAudio(const Packet& packet)
{
  // MP4 takes raw AAC frames.
  const size_t header =
      m_audioCodec == AV_CODEC_ID_AAC ? AdtsHeaderSize(packet.data, packet.size) : 0;
  if (header == 0)
    return Write(m_audio, packet);
  Packet frame = packet;
  frame.data += header;
  frame.size -= header;
  return Write(m_audio, frame);
}

void CMediaBackendMse::EndVideo()
{
  m_video.writer.Finish(m_video.pending);
  const auto end = std::chrono::steady_clock::now() + END_TIMEOUT;
  while (!m_video.pending.empty() && SendPending(m_video) == AppendResult::FULL &&
         std::chrono::steady_clock::now() < end)
    std::this_thread::sleep_for(10ms);
  mse_end_track(m_handle, 1, Session(m_video));
}

void CMediaBackendMse::EndAudio()
{
  m_audio.writer.Finish(m_audio.pending);
  const auto end = std::chrono::steady_clock::now() + END_TIMEOUT;
  while (!m_audio.pending.empty() && SendPending(m_audio) == AppendResult::FULL &&
         std::chrono::steady_clock::now() < end)
    std::this_thread::sleep_for(10ms);
  mse_end_track(m_handle, 0, Session(m_audio));
}

void CMediaBackendMse::ConvertAudio(const DVDAudioFrame& frame)
{
  if (m_encoder)
    m_encoder->Add(frame);
}

void CMediaBackendMse::FinishConvertedAudio()
{
  if (m_encoder)
    m_encoder->Finish();
}

bool CMediaBackendMse::PeekConvertedAudio(Packet& out)
{
  return m_encoder && m_encoder->Peek(out);
}

void CMediaBackendMse::PopConvertedAudio()
{
  if (m_encoder)
    m_encoder->Pop();
}

void CMediaBackendMse::Flush()
{
  if (m_encoder)
    m_encoder->Reset();
  for (Track* track : {&m_video, &m_audio})
  {
    track->pending.clear();
    track->lastPts = -1.0;
  }
}

void CMediaBackendMse::Play()
{
  mse_play(m_handle);
}

void CMediaBackendMse::Pause()
{
  mse_pause(m_handle);
}

void CMediaBackendMse::SetRate(double rate)
{
  mse_set_playback_rate(m_handle, rate);
}

void CMediaBackendMse::SetVolume(float volume)
{
  mse_set_volume(m_handle, static_cast<double>(volume));
}

void CMediaBackendMse::Seek(double seconds)
{
  // The SourceBuffers start parsing over, from an initialisation segment.
  m_video.writer.Restart();
  if (m_withAudio)
    m_audio.writer.Restart();
  Flush();
  std::atomic_ref<double>(m_state.position).store(seconds);
  mse_seek(m_handle, seconds);
}

void CMediaBackendMse::SetStartPosition(double seconds)
{
  std::atomic_ref<double>(m_state.position).store(seconds);
}

int CMediaBackendMse::Error() const
{
  return Read(m_state.error);
}

bool CMediaBackendMse::CanPlay() const
{
  return Read(m_state.canPlay) != 0;
}

bool CMediaBackendMse::Ended() const
{
  return Read(m_state.ended) != 0;
}

double CMediaBackendMse::Position() const
{
  return std::atomic_ref<double>(const_cast<double&>(m_state.position)).load();
}

CMediaBackendMse::CFragmentWriter::~CFragmentWriter()
{
  Close();
}

bool CMediaBackendMse::CFragmentWriter::Open(const AVCodecParameters& parameters)
{
  Close();
  m_parameters = avcodec_parameters_alloc();
  if (!m_parameters || avcodec_parameters_copy(m_parameters, &parameters) < 0)
  {
    Close();
    return false;
  }
  return Start();
}

void CMediaBackendMse::CFragmentWriter::Close()
{
  Stop();
  avcodec_parameters_free(&m_parameters);
}

bool CMediaBackendMse::CFragmentWriter::Restart()
{
  Stop();
  return m_parameters && Start();
}

bool CMediaBackendMse::CFragmentWriter::Start()
{
  if (avformat_alloc_output_context2(&m_context, nullptr, "mp4", nullptr) < 0)
    return false;
  AVStream* stream = avformat_new_stream(m_context, nullptr);
  if (!stream || avcodec_parameters_copy(stream->codecpar, m_parameters) < 0)
  {
    Stop();
    return false;
  }
  stream->codecpar->codec_tag = 0;
  stream->time_base = AVRational{1, DVD_TIME_BASE};

  constexpr int BUFFER_SIZE = 64 * 1024;
  auto* buffer = static_cast<unsigned char*>(av_malloc(BUFFER_SIZE));
  m_context->pb =
      buffer ? avio_alloc_context(buffer, BUFFER_SIZE, 1, this, nullptr, WriteBytes, nullptr)
             : nullptr;
  if (!m_context->pb)
  {
    av_free(buffer);
    Stop();
    return false;
  }
  m_context->flags |= AVFMT_FLAG_CUSTOM_IO;
  m_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  // Timestamps stay as they are, so that the browser places the data where Kodi does.
  m_context->avoid_negative_ts = AVFMT_AVOID_NEG_TS_DISABLED;

  AVDictionary* options = nullptr;
  av_dict_set(&options, "movflags",
              "frag_custom+empty_moov+default_base_moof+delay_moov+frag_discont", 0);
  av_dict_set(&options, "use_editlist", "0", 0);
  const int result = avformat_write_header(m_context, &options);
  av_dict_free(&options);
  if (result < 0)
  {
    CLog::LogF(LOGERROR, "Cannot write MP4 for codec {}: {}", m_parameters->codec_id, result);
    Stop();
    return false;
  }
  return true;
}

void CMediaBackendMse::CFragmentWriter::Stop()
{
  av_packet_free(&m_held);
  m_started = false;
  if (m_context)
  {
    if (m_context->pb)
    {
      av_freep(&m_context->pb->buffer);
      avio_context_free(&m_context->pb);
    }
    avformat_free_context(m_context);
    m_context = nullptr;
  }
  m_bytes.clear();
}

int CMediaBackendMse::CFragmentWriter::WriteBytes(void* opaque, const uint8_t* data, int size)
{
  auto& bytes = static_cast<CFragmentWriter*>(opaque)->m_bytes;
  bytes.insert(bytes.end(), data, data + size);
  return size;
}

bool CMediaBackendMse::CFragmentWriter::Write(const Packet& packet, std::vector<uint8_t>& out)
{
  if (!m_context)
    return false;

  AVPacket* sample = av_packet_alloc();
  if (!sample || av_new_packet(sample, static_cast<int>(packet.size)) < 0)
  {
    av_packet_free(&sample);
    return false;
  }
  std::memcpy(sample->data, packet.data, packet.size);
  sample->pts = std::llround((packet.pts + TIMESTAMP_OFFSET) * DVD_TIME_BASE);
  sample->dts = std::llround((packet.dts + TIMESTAMP_OFFSET) * DVD_TIME_BASE);
  sample->duration = std::llround(packet.duration * DVD_TIME_BASE);
  sample->flags = packet.keyFrame ? AV_PKT_FLAG_KEY : 0;
  sample->stream_index = 0;
  av_packet_rescale_ts(sample, AVRational{1, DVD_TIME_BASE}, m_context->streams[0]->time_base);

  // MP4 needs decode times that rise in the track's time scale.
  if (m_started && sample->dts <= m_lastDts)
    sample->dts = m_lastDts + 1;
  sample->pts = std::max(sample->pts, sample->dts);
  m_lastDts = sample->dts;
  m_started = true;

  bool written = true;
  if (m_held)
  {
    m_held->duration = sample->dts - m_held->dts;
    written = WriteSample(m_held, out);
    av_packet_free(&m_held);
  }
  m_held = sample;
  return written;
}

bool CMediaBackendMse::CFragmentWriter::Finish(std::vector<uint8_t>& out)
{
  if (!m_held)
    return true;
  const bool written = WriteSample(m_held, out);
  av_packet_free(&m_held);
  return written;
}

bool CMediaBackendMse::CFragmentWriter::WriteSample(AVPacket* sample, std::vector<uint8_t>& out)
{
  const int64_t dts = sample->dts;
  int result = av_write_frame(m_context, sample);
  // Cuts the fragment.
  if (result >= 0)
    result = av_write_frame(m_context, nullptr);
  avio_flush(m_context->pb);
  if (result < 0)
  {
    CLog::LogF(LOGERROR, "Cannot mux a packet at {}: {}", dts, result);
    m_bytes.clear();
    return false;
  }
  out.insert(out.end(), m_bytes.begin(), m_bytes.end());
  m_bytes.clear();
  return true;
}

CMediaBackendMse::CAudioEncoder::~CAudioEncoder()
{
  Close();
}

bool CMediaBackendMse::CAudioEncoder::Open(AVCodecID codecId,
                                           int channels,
                                           int sampleRate,
                                           bool highResolution)
{
  Close();
  const bool opus = codecId == AV_CODEC_ID_OPUS;
  m_codec = codecId;
  m_channels = channels;
  // Opus takes 48 kHz only.
  m_sampleRate = opus ? OPUS_SAMPLE_RATE : sampleRate;
  m_highResolution = highResolution;

  const AVCodec* codec =
      opus ? avcodec_find_encoder_by_name("libopus") : avcodec_find_encoder(codecId);
  m_context = codec ? avcodec_alloc_context3(codec) : nullptr;
  if (!m_context)
    return false;
  m_context->sample_rate = m_sampleRate;
  av_channel_layout_default(&m_context->ch_layout, channels);
  m_context->time_base = AVRational{1, m_sampleRate};
  if (opus)
  {
    m_context->sample_fmt = AV_SAMPLE_FMT_FLT;
    m_context->bit_rate = static_cast<int64_t>(channels) * OPUS_BITRATE_PER_CHANNEL;
    // Complexity 5 instead of the default 10 costs about a third less on TVs.
    m_context->compression_level = 5;
  }
  else
  {
    m_context->sample_fmt = highResolution ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;
    if (highResolution)
      m_context->bits_per_raw_sample = 24;
    m_context->compression_level = 0;
  }
  if (avcodec_open2(m_context, codec, nullptr) < 0 || m_context->frame_size <= 0)
  {
    CLog::LogF(LOGERROR, "Cannot encode {} channels at {} Hz as {}", channels, m_sampleRate,
               avcodec_get_name(codecId));
    Close();
    return false;
  }

  m_fifo = av_audio_fifo_alloc(m_context->sample_fmt, channels, m_context->frame_size);
  m_frame = av_frame_alloc();
  m_packet = av_packet_alloc();
  if (!m_fifo || !m_frame || !m_packet)
  {
    Close();
    return false;
  }
  m_startPts = -1.0;
  m_samples = 0;
  m_encoded.clear();
  return true;
}

void CMediaBackendMse::CAudioEncoder::Close()
{
  avcodec_free_context(&m_context);
  swr_free(&m_resample);
  m_resampleFormat = -1;
  if (m_fifo)
    av_audio_fifo_free(m_fifo);
  m_fifo = nullptr;
  av_frame_free(&m_frame);
  av_packet_free(&m_packet);
  m_encoded.clear();
}

bool CMediaBackendMse::CAudioEncoder::Reset()
{
  return m_context && Open(m_codec, m_channels, m_sampleRate, m_highResolution);
}

void CMediaBackendMse::CAudioEncoder::Add(const DVDAudioFrame& frame)
{
  if (!m_context || frame.nb_frames == 0)
    return;

  const int channels = static_cast<int>(frame.format.m_channelLayout.Count());
  const int rate = static_cast<int>(frame.format.m_sampleRate);
  const AVSampleFormat format = CAEUtil::GetAVSampleFormat(frame.format.m_dataFormat);
  if (!m_resample || format != m_resampleFormat || channels != m_resampleChannels ||
      rate != m_resampleRate)
  {
    swr_free(&m_resample);
    AVChannelLayout layout;
    av_channel_layout_default(&layout, channels);
    if (swr_alloc_set_opts2(&m_resample, &m_context->ch_layout, m_context->sample_fmt, m_sampleRate,
                            &layout, format, rate, 0, nullptr) < 0 ||
        swr_init(m_resample) < 0)
    {
      CLog::LogF(LOGERROR, "Cannot convert {} channels of format {} for {}", channels,
                 static_cast<int>(format), avcodec_get_name(m_codec));
      swr_free(&m_resample);
      return;
    }
    m_resampleFormat = format;
    m_resampleChannels = channels;
    m_resampleRate = rate;
  }

  if (m_startPts < 0.0)
    m_startPts = frame.pts != DVD_NOPTS_VALUE ? frame.pts / DVD_TIME_BASE : 0.0;

  const int capacity = swr_get_out_samples(m_resample, static_cast<int>(frame.nb_frames));
  std::vector<uint8_t> converted(static_cast<size_t>(std::max(capacity, 0)) * m_channels *
                                 av_get_bytes_per_sample(m_context->sample_fmt));
  uint8_t* out = converted.data();
  const int samples =
      swr_convert(m_resample, &out, capacity, reinterpret_cast<const uint8_t* const*>(frame.data),
                  static_cast<int>(frame.nb_frames));
  if (samples <= 0)
    return;
  void* data = converted.data();
  av_audio_fifo_write(m_fifo, &data, samples);

  while (av_audio_fifo_size(m_fifo) >= m_context->frame_size)
  {
    m_frame->nb_samples = m_context->frame_size;
    m_frame->format = m_context->sample_fmt;
    m_frame->sample_rate = m_sampleRate;
    av_channel_layout_copy(&m_frame->ch_layout, &m_context->ch_layout);
    if (av_frame_get_buffer(m_frame, 0) < 0)
      return;
    av_audio_fifo_read(m_fifo, reinterpret_cast<void**>(m_frame->data), m_frame->nb_samples);
    m_frame->pts = m_samples;
    m_samples += m_frame->nb_samples;
    Encode(m_frame);
    av_frame_unref(m_frame);
  }
}

void CMediaBackendMse::CAudioEncoder::Finish()
{
  if (!m_context)
    return;
  const int left = av_audio_fifo_size(m_fifo);
  if (left > 0)
  {
    m_frame->nb_samples = left;
    m_frame->format = m_context->sample_fmt;
    m_frame->sample_rate = m_sampleRate;
    av_channel_layout_copy(&m_frame->ch_layout, &m_context->ch_layout);
    if (av_frame_get_buffer(m_frame, 0) >= 0)
    {
      av_audio_fifo_read(m_fifo, reinterpret_cast<void**>(m_frame->data), left);
      m_frame->pts = m_samples;
      m_samples += left;
      Encode(m_frame);
    }
    av_frame_unref(m_frame);
  }
  Encode(nullptr);
}

void CMediaBackendMse::CAudioEncoder::Encode(AVFrame* frame)
{
  if (avcodec_send_frame(m_context, frame) < 0)
    return;
  while (avcodec_receive_packet(m_context, m_packet) == 0)
  {
    m_encoded.push_back({std::vector<uint8_t>(m_packet->data, m_packet->data + m_packet->size),
                         m_startPts + static_cast<double>(m_packet->pts) / m_sampleRate,
                         static_cast<double>(m_packet->duration) / m_sampleRate});
    av_packet_unref(m_packet);
  }
}

bool CMediaBackendMse::CAudioEncoder::Peek(Packet& out) const
{
  if (m_encoded.empty())
    return false;
  const EncodedFrame& frame = m_encoded.front();
  out.data = frame.data.data();
  out.size = frame.data.size();
  out.pts = frame.pts;
  out.dts = frame.pts;
  out.duration = frame.duration;
  out.keyFrame = true;
  return true;
}

void CMediaBackendMse::CAudioEncoder::Pop()
{
  if (!m_encoded.empty())
    m_encoded.pop_front();
}
