#include "AENullSink.h"

#include "AESinkFactory.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace
{

constexpr std::chrono::duration<double, std::ratio<1>> DEFAULT_BUFFER_DURATION = 0.200s;
constexpr int DEFAULT_PERIODS = 4;
constexpr std::chrono::duration<double, std::ratio<1>> DEFAULT_PERIOD_DURATION =
    DEFAULT_BUFFER_DURATION / DEFAULT_PERIODS;

} // namespace

bool CAENullSink::Register()
{
  AE::AESinkRegEntry entry;
  entry.sinkName = "NULL";
  entry.createFunc = CAENullSink::Create;
  entry.enumerateFunc = CAENullSink::EnumerateDevicesEx;
  entry.cleanupFunc = CAENullSink::Destroy;
  AE::CAESinkFactory::RegisterSink(entry);

  return true;
}

std::unique_ptr<IAESink> CAENullSink::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAENullSink>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

void CAENullSink::EnumerateDevicesEx(AEDeviceInfoList& list, bool force)
{
  CAEDeviceInfo info;
  info.m_deviceType = AE_DEVTYPE_PCM;
  info.m_deviceName = "Default";
  info.m_displayName = "Default";
  info.m_displayNameExtra = "Default Output Device (NULL)";
  info.m_wantsIECPassthrough = true;
  info.m_dataFormats.emplace_back(AEDataFormat::AE_FMT_FLOAT);

  constexpr std::array<unsigned int, 13> sampleRates = {
      8000, 11025, 16000, 22050, 32000, 44100, 48000, 64000, 88200, 96000, 176400, 192000, 384000,
  };

  std::for_each(sampleRates.cbegin(), sampleRates.cend(),
                [&info](const auto& rate) { info.m_sampleRates.emplace_back(rate); });

  info.m_channels = CAEChannelInfo(AE_CH_LAYOUT_7_1);

  constexpr std::array<CAEStreamInfo::DataType, 10> streamTypes = {
      CAEStreamInfo::STREAM_TYPE_AC3,      CAEStreamInfo::STREAM_TYPE_DTS_512,
      CAEStreamInfo::STREAM_TYPE_DTS_1024, CAEStreamInfo::STREAM_TYPE_DTS_2048,
      CAEStreamInfo::STREAM_TYPE_DTSHD,    CAEStreamInfo::STREAM_TYPE_DTSHD_CORE,
      CAEStreamInfo::STREAM_TYPE_EAC3,     CAEStreamInfo::STREAM_TYPE_MLP,
      CAEStreamInfo::STREAM_TYPE_TRUEHD,   CAEStreamInfo::STREAM_TYPE_DTSHD_MA,
  };

  std::for_each(streamTypes.cbegin(), streamTypes.cend(),
                [&info](const auto& streamType) { info.m_streamTypes.emplace_back(streamType); });

  info.m_dataFormats.emplace_back(AEDataFormat::AE_FMT_RAW);
  info.m_deviceType = AE_DEVTYPE_HDMI;

  list.emplace_back(info);
}

void CAENullSink::Destroy()
{
  // nothing to do here
}

bool CAENullSink::Initialize(AEAudioFormat& format, std::string& device)
{
  format.m_frameSize =
      format.m_channelLayout.Count() * CAEUtil::DataFormatToBits(format.m_dataFormat) / 8;
  format.m_frames = std::nearbyint(DEFAULT_PERIOD_DURATION.count() * format.m_sampleRate);

  m_format = format;

  return true;
}

unsigned int CAENullSink::AddPackets(uint8_t** data, unsigned int frames, unsigned int offset)
{
  const auto period = std::chrono::duration<double, std::ratio<1>>(static_cast<double>(frames) /
                                                                   m_format.m_sampleRate);

  std::this_thread::sleep_for(period);

  return frames;
}
