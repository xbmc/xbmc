/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioBypassWebOS.h"

#include "DVDCodecs/Audio/DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "Interface/DemuxPacket.h"
#include "Interface/TimingConstants.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"

#include "platform/linux/WebOSTVPlatformConfig.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <utility>

#include <dlfcn.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <gst/gst.h>
#include <sound/asound.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <webos-helpers/libhelpers.h>

using namespace std::chrono_literals;

namespace
{
constexpr const char* CONTROL_DEVICE = "/dev/snd/controlC0";
constexpr const char* IPC_DEVICE = "/dev/lg/audio_uimc";
constexpr unsigned int MIXER_PORT = 4;
const std::string UMI_MIXER =
    fmt::format(R"("source":"AMIXER","sourcePort":{},"sink":"MAIN")", MIXER_PORT);
constexpr const char* UMI_ATTRIBUTE =
    R"("pipelineAttribute":{"sourceType":"MEDIA","sourceAttribute":"MEDIA","playFormatType":"media"})";
constexpr const char* EARC_OUTPUT_TYPE = "Sndout EARC Outputtype";
constexpr long EARC_CODEC_PCM = 1;
constexpr long EARC_MIX_ALL = 1;
constexpr std::uint32_t PCMCVT_SET_ALLFMT = 0x50020020;
constexpr std::uint32_t IPC_COMMAND = 1;
constexpr const char* WIRE_ORDER = "ch-mapping-lfe-c";
constexpr int ACMOD_7_1 = 3;
constexpr AEChannel PCM_LAYOUTS[][9] = {
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_BL, AE_CH_BR, AE_CH_NULL}};

struct IpcHeader
{
  std::uint32_t command{0};
  std::uint32_t identifier{0};
  std::uint32_t sender{0};
  std::uint32_t action{0};
  std::uint32_t option{0};
  std::uint32_t target{0};
  std::uint32_t event{0};
  std::uint32_t length{0};
};

struct ConverterMessage
{
  IpcHeader header;
  std::array<std::uint32_t, 8> format;
};

struct IpcRequest
{
  std::int32_t timeout{0};
  std::uint32_t length{0};
  std::uint64_t message{0};
};

constexpr unsigned long UIMC_IPC_WRITE = _IOW('M', 2, IpcRequest);

struct EventApi
{
  explicit EventApi(void* caller)
  {
    Dl_info info{};
    void* library =
        dladdr(caller, &info) ? dlopen(info.dli_fname, RTLD_LAZY | RTLD_NOLOAD) : nullptr;
    const auto resolve = [library](const char* name)
    {
      void* symbol = library ? dlsym(library, name) : nullptr;
      return symbol ? symbol : dlsym(RTLD_NEXT, name);
    };

    push = reinterpret_cast<decltype(push)>(resolve("module_appsrc_push_event"));
    getBool = reinterpret_cast<decltype(getBool)>(resolve("aud_event_value_get_bool"));
    setBool = reinterpret_cast<decltype(setBool)>(resolve("aud_event_value_set_bool"));
    getInt = reinterpret_cast<decltype(getInt)>(resolve("aud_event_value_get_int"));
    setInt = reinterpret_cast<decltype(setInt)>(resolve("aud_event_value_set_int"));
  }

  int (*push)(void*, void*){nullptr};
  bool (*getBool)(void*, const char*, bool){nullptr};
  void (*setBool)(void*, const char*, bool){nullptr};
  int (*getInt)(void*, const char*, int){nullptr};
  void (*setInt)(void*, const char*, int){nullptr};
};

std::optional<std::array<long, 5>> OutputType(int fd,
                                              unsigned long request,
                                              std::array<long, 5> values = {})
{
  snd_ctl_elem_value value{};
  value.id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
  std::strncpy(reinterpret_cast<char*>(value.id.name), EARC_OUTPUT_TYPE, sizeof(value.id.name) - 1);
  std::ranges::copy(values, value.value.integer.value);
  if (ioctl(fd, request, &value) < 0)
    return {};
  std::copy_n(value.value.integer.value, values.size(), values.begin());
  return values;
}

void Luna(const char* method, const std::string& payload)
{
  HContext context{};
  context.pub = true;
  if (HLunaServiceCall(fmt::format("luna://com.webos.service.audio/{}", method).c_str(),
                       payload.c_str(), &context))
    CLog::LogF(LOGWARNING, "{} failed", method);
}

std::set<unsigned int> ListConverters()
{
  std::set<unsigned int> uids;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator("/proc/aaudio/instances", ec))
  {
    const std::filesystem::path target = std::filesystem::read_symlink(entry.path(), ec);
    if (ec || std::none_of(target.begin(), target.end(),
                           [](const std::filesystem::path& part) { return part == "srcbin"; }))
      continue;

    const std::string name = entry.path().filename().string();
    unsigned int uid = 0;
    if (std::from_chars(name.data(), name.data() + name.size(), uid, 16).ec == std::errc())
      uids.insert(uid);
  }
  return uids;
}

void ConfigureConverter(
    int fd, unsigned int uid, unsigned int rate, unsigned int channels, unsigned int bits)
{
  const ConverterMessage message{{.command = PCMCVT_SET_ALLFMT,
                                  .action = IPC_COMMAND,
                                  .target = uid,
                                  .length = sizeof(ConverterMessage::format)},
                                 {rate, channels, bits, 0, 0, rate, channels, bits}};
  IpcRequest request{.length = sizeof(ConverterMessage::format),
                     .message = reinterpret_cast<std::uintptr_t>(&message)};
  if (ioctl(fd, UIMC_IPC_WRITE, &request) < 0)
    CLog::LogF(LOGWARNING, "Failed to configure converter {:08x}", uid);
}

bool Available(const CDVDStreamInfo& hint)
{
  static const bool supported = []
  {
    const KODI::UTILS::POSIX::CFileHandle fd{open(CONTROL_DEVICE, O_RDWR | O_CLOEXEC)};
    GstElement* sink = gst_init_check(nullptr, nullptr, nullptr)
                           ? gst_element_factory_make("audiosink", nullptr)
                           : nullptr;
    const bool bypass = sink &&
                        g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "pcm-bypass") &&
                        g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "pcm-multi-ch");
    if (sink)
      gst_object_unref(sink);
    return bypass && OutputType(fd, SNDRV_CTL_IOCTL_ELEM_READ) &&
           access(IPC_DEVICE, R_OK | W_OK) == 0;
  }();
  return supported && !hint.cryptoSession && WebOSTVPlatformConfig::SupportsEARCPassthrough();
}
} // namespace

extern "C" int module_appsrc_push_event(void* source, void* event)
{
  static const EventApi api(__builtin_return_address(0));
  if (!api.push)
    return -1;

  if (api.getBool && api.setBool && api.getInt && api.setInt &&
      api.getBool(event, WIRE_ORDER, false))
  {
    api.setBool(event, WIRE_ORDER, false);
    api.setInt(event, "data-rate", api.getInt(event, "rate", 0));
    if (api.getInt(event, "channels", 0) == 8)
      api.setInt(event, "acmod", ACMOD_7_1);
  }
  return api.push(source, event);
}

CAudioBypassWebOS::CAudioBypassWebOS(CProcessInfo& processInfo,
                                     CAEStreamInfo::DataType type,
                                     int profile)
  : CAudioBypassWebOS(AEAudioFormat{})
{
  m_format.m_streamInfo.m_type = type;
  m_atmos = profile == AV_PROFILE_EAC3_DDP_ATMOS || profile == AV_PROFILE_TRUEHD_ATMOS;
  m_parser = std::make_unique<CDVDAudioCodecPassthrough>(processInfo, type);
  CDVDStreamInfo hints;
  CDVDCodecOptions options;
  m_parser->Open(hints, options);
}

CAudioBypassWebOS::CAudioBypassWebOS(const AEAudioFormat& format)
  : m_format(format),
    m_controlFd(open(CONTROL_DEVICE, O_RDWR | O_CLOEXEC)),
    m_ipcFd(open(IPC_DEVICE, O_RDWR | O_CLOEXEC))
{
}

CAudioBypassWebOS::~CAudioBypassWebOS()
{
  std::unique_lock lock(m_mutex);
  DestroyPipeline();
  if (m_videoPad)
    gst_object_unref(m_videoPad);
  if (m_starfish)
    gst_object_unref(m_starfish);
}

std::optional<CAEStreamInfo::DataType> CAudioBypassWebOS::GetPassthroughType(
    const CDVDStreamInfo& hint)
{
  if (!Available(hint))
    return {};

  if (hint.codec == AV_CODEC_ID_TRUEHD && WebOSTVPlatformConfig::SupportsEARCFormat("TrueHD"))
    return CAEStreamInfo::STREAM_TYPE_TRUEHD;

  if (hint.codec == AV_CODEC_ID_EAC3 && WebOSTVPlatformConfig::SupportsEARCFormat("DD+"))
    return CAEStreamInfo::STREAM_TYPE_EAC3;

  if (hint.codec == AV_CODEC_ID_AC3 && WebOSTVPlatformConfig::SupportsEARCFormat("DD"))
    return CAEStreamInfo::STREAM_TYPE_AC3;

  if (hint.codec == AV_CODEC_ID_DTS)
  {
    if (WebOSTVPlatformConfig::SupportsEARCFormat("DTSHD_MA"))
      return CAEStreamInfo::STREAM_TYPE_DTSHD;
    if (WebOSTVPlatformConfig::SupportsEARCFormat("DTS"))
      return CAEStreamInfo::STREAM_TYPE_DTSHD_CORE;
  }

  return {};
}

std::optional<AEAudioFormat> CAudioBypassWebOS::GetPCMFormat(const CDVDStreamInfo& hint)
{
  if (hint.codec == AV_CODEC_ID_NONE || hint.samplerate <= 0 || !Available(hint))
    return {};

  const CAEChannelInfo source = hint.channellayout ? CAEUtil::GetAEChannelLayout(hint.channellayout)
                                                   : CAEUtil::GuessChLayout(hint.channels);
  std::optional<AEAudioFormat> downmix;
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_S32NE;
  for (const auto& layout : PCM_LAYOUTS)
  {
    format.m_channelLayout = layout;
    format.m_sampleRate =
        WebOSTVPlatformConfig::GetEARCPCMRate(format.m_channelLayout.Count(), hint.samplerate);
    if (format.m_sampleRate == 0)
      break;
    if (format.m_channelLayout.ContainsChannels(source))
      return format;
    if (format.m_channelLayout.Count() == 6)
      downmix = format;
  }
  return downmix;
}

std::string CAudioBypassWebOS::GetName() const
{
  return m_parser ? "earc-" + m_parser->GetName() : "earc-pcm";
}

unsigned int CAudioBypassWebOS::GetQueuedBytes() const
{
  std::unique_lock lock(m_mutex);
  guint64 level = 0;
  if (m_source)
    g_object_get(m_source, "current-level-bytes", &level, nullptr);
  return static_cast<unsigned int>(level);
}

void CAudioBypassWebOS::Attach(GstElement* pipeline, GstElement* videoSink, const char* mediaId)
{
  std::unique_lock lock(m_mutex);
  if (m_videoPad)
    gst_object_unref(m_videoPad);
  if (m_starfish)
    gst_object_unref(m_starfish);
  m_umiPipeline = fmt::format(R"("pipelineId":"{}","streamType":"umimedia")", mediaId);
  m_starfish = GST_ELEMENT(gst_object_ref(pipeline));
  m_videoPad = gst_element_get_static_pad(videoSink, "sink");
}

void CAudioBypassWebOS::SetPlaying(bool playing)
{
  std::unique_lock lock(m_mutex);
  m_playing = playing;
  if (!playing && m_pipeline)
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
}

void CAudioBypassWebOS::Update(bool started)
{
  std::unique_lock lock(m_mutex);
  Sync();
  if (m_pipeline && started && !m_started)
    Report("", "");
  m_started = started;
}

void CAudioBypassWebOS::Decode()
{
  std::unique_lock lock(m_mutex);
  DestroyPipeline();
  m_idleParser = std::move(m_parser);
  m_idleFormat = std::exchange(m_format, {});
  m_format.m_dataFormat = AE_FMT_S32NE;
  m_format.m_channelLayout = AE_CH_LAYOUT_2_0;
  m_format.m_sampleRate = 48000;
}

void CAudioBypassWebOS::Bitstream()
{
  std::unique_lock lock(m_mutex);
  DestroyPipeline();
  m_parser = std::move(m_idleParser);
  m_parser->Reset();
  m_packer.Reset();
  m_format = m_idleFormat;
}

void CAudioBypassWebOS::Flush()
{
  std::unique_lock lock(m_mutex);
  if (m_parser)
    m_parser->Reset();
  m_packer.Reset();
  m_nextPts = 0.0;

  if (m_pipeline)
  {
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    gst_element_send_event(m_source, gst_event_new_flush_start());
    gst_element_send_event(m_source, gst_event_new_flush_stop(FALSE));
  }

  if (m_starfish)
    m_staleBaseTime = gst_element_get_base_time(m_starfish);

  if (GstEvent* event =
          m_videoPad ? gst_pad_get_sticky_event(m_videoPad, GST_EVENT_SEGMENT, 0) : nullptr)
  {
    m_staleSegment = gst_event_get_seqnum(event);
    gst_event_unref(event);
  }
}

bool CAudioBypassWebOS::AddPacket(const DemuxPacket& packet)
{
  std::unique_lock lock(m_mutex);
  GstEvent* event =
      m_videoPad ? gst_pad_get_sticky_event(m_videoPad, GST_EVENT_SEGMENT, 0) : nullptr;
  if (!event)
    return false;

  const guint32 seqnum = gst_event_get_seqnum(event);
  GstSegment segment;
  gst_event_copy_segment(event, &segment);
  gst_event_unref(event);

  guint64 level = 0;
  if (m_source)
    g_object_get(m_source, "current-level-bytes", &level, nullptr);

  if (seqnum == m_staleSegment || (m_source && level >= m_rate * m_channels) ||
      (m_parser ? !m_parser->AddData(packet)
                : !m_pipeline &&
                      !CreatePipeline(m_format.m_sampleRate, m_format.m_channelLayout.Count())))
    return false;

  if (!m_parser)
  {
    Push(segment, packet.pts, packet.pData, packet.iSize);
    return true;
  }

  DVDAudioFrame frame;
  for (m_parser->GetData(frame); frame.nb_frames > 0; m_parser->GetData(frame))
  {
    const double pts = frame.pts == DVD_NOPTS_VALUE ? m_nextPts : frame.pts;
    m_nextPts = pts + frame.duration;

    CAEStreamInfo& info = frame.format.m_streamInfo;
    m_packer.Pack(info, frame.data[0], static_cast<int>(frame.nb_frames));
    const unsigned int size = m_packer.GetSize();
    const unsigned int rate = CAEBitstreamPacker::GetOutputRate(info);
    const unsigned int channels = CAEBitstreamPacker::GetOutputChannelMap(info).Count();
    if (size > 0 && ((rate == m_rate && channels == m_channels) || CreatePipeline(rate, channels)))
      Push(segment, pts, m_packer.GetBuffer(), size);
  }

  return true;
}

void CAudioBypassWebOS::Push(const GstSegment& segment,
                             double pts,
                             const std::uint8_t* data,
                             unsigned int size)
{
  const auto position = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(pts));
  const guint64 time =
      position < 0ns ? GST_CLOCK_TIME_NONE
                     : gst_segment_to_running_time(&segment, GST_FORMAT_TIME, position.count());
  if (time == GST_CLOCK_TIME_NONE)
    return;

  GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  if (m_parser)
  {
    const auto* words = reinterpret_cast<const std::uint16_t*>(data);
    std::transform(words, words + size / 2, reinterpret_cast<std::uint16_t*>(map.data),
                   Endian_Swap16);
  }
  else
    std::copy_n(data, size, map.data);
  gst_buffer_unmap(buffer, &map);
  GST_BUFFER_PTS(buffer) = time;

  GstFlowReturn ret;
  g_signal_emit_by_name(m_source, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);
}

bool CAudioBypassWebOS::CreatePipeline(unsigned int rate, unsigned int channels)
{
  DestroyPipeline();

  const CAEStreamInfo::DataType type = m_format.m_streamInfo.m_type;
  const std::string format =
      m_parser ? fmt::format("S16LE,acmod=(uint)0,es=(boolean)true,codec-str=(string){},"
                             "clock-x4=(boolean){},data-rate=(int){},data-channels=(int){},"
                             "data-bps=(int)16",
                             channels == 8                             ? "mat"
                             : type == CAEStreamInfo::STREAM_TYPE_EAC3 ? "eac3"
                             : type == CAEStreamInfo::STREAM_TYPE_AC3  ? "ac3"
                                                                       : "dts",
                             rate == 192000, rate, channels)
               : "S32LE";
  m_pipeline = gst_parse_launch(
      fmt::format("appsrc name=source format=time max-bytes={0} caps=\"audio/x-raw,format={3},"
                  "layout=interleaved,rate={1},channels={2},channel-mask=(bitmask){4:#x}\" ! "
                  "audiosink name=sink mixer=true index={5} lipsync=true {6}",
                  rate * channels, rate, channels, format,
                  channels == 8   ? 0xc3f
                  : channels == 6 ? 0x3f
                                  : 0x3,
                  MIXER_PORT, m_parser ? "pcm-multi-ch=true" : "pcm-bypass=true")
          .c_str(),
      nullptr);
  if (!m_pipeline)
  {
    CLog::LogF(LOGERROR, "Failed to create the eARC pipeline");
    return false;
  }

  m_source = gst_bin_get_by_name(GST_BIN(m_pipeline), "source");
  m_sink = gst_bin_get_by_name(GST_BIN(m_pipeline), "sink");
  gst_object_unref(m_source);
  gst_object_unref(m_sink);

  MixOutput();
  m_preexisting = ListConverters();

  GstPad* pad = gst_element_get_static_pad(m_sink, "sink");
  gst_pad_add_probe(
      pad, GST_PAD_PROBE_TYPE_BUFFER,
      [](GstPad*, GstPadProbeInfo* info, gpointer self)
      {
        static_cast<CAudioBypassWebOS*>(self)->OnBuffer(info);
        return GST_PAD_PROBE_OK;
      },
      this, nullptr);
  gst_object_unref(pad);

  m_rate = rate;
  m_channels = channels;
  m_buffers = 0;
  gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
  return true;
}

void CAudioBypassWebOS::DestroyPipeline()
{
  if (!m_pipeline)
    return;

  gst_element_set_state(m_pipeline, GST_STATE_NULL);
  gst_object_unref(m_pipeline);
  m_pipeline = m_source = m_sink = nullptr;
  m_rate = m_channels = 0;

  if (m_buffers > 0)
  {
    Luna("UMI/disconnect", fmt::format("{{{},{}}}", m_umiPipeline, UMI_MIXER));
    Luna("UMI/unregister", fmt::format("{{{},{}}}", m_umiPipeline, UMI_ATTRIBUTE));
  }
  MixOutput();
}

void CAudioBypassWebOS::Report(const std::string& pipelineInfo, const std::string& categoryInfo)
{
  Luna("tv/setQualityAttribute",
       fmt::format(R"({{"categoryType":"audioInputInfo","requestType":"setAudioInputInfo",)"
                   R"("pipelineInfo":{{{}{}}},"categoryInfo":{{{}}}}})",
                   m_umiPipeline, pipelineInfo, categoryInfo));
}

bool CAudioBypassWebOS::IsMixed() const
{
  const auto earc = OutputType(m_controlFd, SNDRV_CTL_IOCTL_ELEM_READ);
  return !earc || (*earc)[4] == EARC_MIX_ALL;
}

void CAudioBypassWebOS::MixOutput()
{
  if (const auto earc = OutputType(m_controlFd, SNDRV_CTL_IOCTL_ELEM_READ))
    OutputType(m_controlFd, SNDRV_CTL_IOCTL_ELEM_WRITE,
               {(*earc)[0], EARC_CODEC_PCM, 0, 0, EARC_MIX_ALL});
}

void CAudioBypassWebOS::Sync()
{
  if (!m_pipeline || !m_starfish || !m_playing || GST_STATE(m_starfish) != GST_STATE_PLAYING)
    return;

  const GstClockTime baseTime = gst_element_get_base_time(m_starfish);
  if (baseTime == m_staleBaseTime || (GST_STATE_TARGET(m_pipeline) == GST_STATE_PLAYING &&
                                      baseTime == gst_element_get_base_time(m_pipeline)))
    return;

  if (m_parser && IsMixed())
    return;

  GstClock* clock = gst_element_get_clock(m_starfish);
  if (!clock)
    return;

  gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
  gst_pipeline_use_clock(GST_PIPELINE(m_pipeline), clock);
  gst_object_unref(clock);
  gst_element_set_start_time(m_pipeline, GST_CLOCK_TIME_NONE);
  gst_element_set_base_time(m_pipeline, baseTime);
  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void CAudioBypassWebOS::OnBuffer(GstPadProbeInfo* info)
{
  const unsigned int index = m_buffers++;
  if (index == 0)
  {
    if (m_parser)
      g_object_set(m_sink, "pcm-multi-ch", FALSE, nullptr);
    unsigned int converter = 0;
    for (const unsigned int uid : ListConverters())
      if (!m_preexisting.contains(uid))
        converter = uid;
    if (converter)
      ConfigureConverter(m_ipcFd, converter, m_rate, m_channels, m_parser ? 16 : 32);
    const CAEStreamInfo::DataType type = m_format.m_streamInfo.m_type;
    Luna("UMI/register", fmt::format("{{{}}}", m_umiPipeline));
    Report(fmt::format(R"(,"resource":"AMIXER{}","sourceType":"MEDIA","sourceAttribute":"MEDIA",)"
                       R"("contentType":"movie","pipelinetype":"audiovideo")",
                       MIXER_PORT),
           fmt::format(R"("codec":"{}","channel":"{}","immersive":"{}")",
                       type == CAEStreamInfo::STREAM_TYPE_TRUEHD ? "truehd"
                       : type == CAEStreamInfo::STREAM_TYPE_EAC3 ? "eac3"
                       : type == CAEStreamInfo::STREAM_TYPE_AC3  ? "ac3"
                                                                 : "pcm",
                       m_channels, m_parser && m_atmos ? "atmos" : "none"));
    Luna("UMI/connect",
         fmt::format(R"({{{},{},{},"activate":true}})", m_umiPipeline, UMI_MIXER, UMI_ATTRIBUTE));
  }
  else if (index == 1 && m_parser)
    g_object_set(m_sink, "pcm-multi-ch", TRUE, nullptr);

  if (!m_parser || index == 0 || !IsMixed())
    return;

  GstBuffer* buffer = gst_buffer_make_writable(GST_PAD_PROBE_INFO_BUFFER(info));
  GST_PAD_PROBE_INFO_DATA(info) = buffer;
  gst_buffer_memset(buffer, 0, 0, gst_buffer_get_size(buffer));
}
