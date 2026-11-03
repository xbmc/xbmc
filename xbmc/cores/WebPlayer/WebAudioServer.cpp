/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebAudioServer.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEPackIEC61937.h"
#include "kodi-web-audio.h"
#include "utils/log.h"

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <ctime>
#include <iterator>

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

using namespace KODI::WEB;

namespace
{

int64_t MonotonicNs()
{
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<int64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

void Signal(int fd)
{
  const uint64_t one = 1;
  if (write(fd, &one, sizeof(one)) != sizeof(one))
    CLog::Log(LOGERROR, "CWebAudioServer: eventfd write failed: {}", strerror(errno));
}

void Drain(int fd)
{
  uint64_t count;
  if (read(fd, &count, sizeof(count)) < 0 && errno != EAGAIN)
    CLog::Log(LOGERROR, "CWebAudioServer: eventfd read failed: {}", strerror(errno));
}

bool IsNativeOrder(bool bigEndian)
{
  return bigEndian == (std::endian::native == std::endian::big);
}

// the engine takes PCM in its native byte order and 24 bits at the top of 32-bit words only,
// the other layouts are repacked sample by sample on the way in
bool GetPcmLayout(uint32_t encoding, PcmLayout& layout)
{
  switch (encoding)
  {
    case KODI_WEB_AUDIO_U8:
      layout = {AE_FMT_U8, 1, 1, false, false};
      return true;
    case KODI_WEB_AUDIO_S16LE:
    case KODI_WEB_AUDIO_S16BE:
      layout = {AE_FMT_S16NE, 2, 2, encoding == KODI_WEB_AUDIO_S16BE, false};
      return true;
    case KODI_WEB_AUDIO_S24LE:
    case KODI_WEB_AUDIO_S24BE:
      layout = {AE_FMT_S24NE4MSB, 3, 4, encoding == KODI_WEB_AUDIO_S24BE, true};
      return true;
    case KODI_WEB_AUDIO_S24_32LE:
    case KODI_WEB_AUDIO_S24_32BE:
      layout = {AE_FMT_S24NE4MSB, 4, 4, encoding == KODI_WEB_AUDIO_S24_32BE, true};
      return true;
    case KODI_WEB_AUDIO_S32LE:
    case KODI_WEB_AUDIO_S32BE:
      layout = {AE_FMT_S32NE, 4, 4, encoding == KODI_WEB_AUDIO_S32BE, false};
      return true;
    case KODI_WEB_AUDIO_F32LE:
    case KODI_WEB_AUDIO_F32BE:
      layout = {AE_FMT_FLOAT, 4, 4, encoding == KODI_WEB_AUDIO_F32BE, false};
      return true;
    case KODI_WEB_AUDIO_F64LE:
    case KODI_WEB_AUDIO_F64BE:
      layout = {AE_FMT_DOUBLE, 8, 8, encoding == KODI_WEB_AUDIO_F64BE, false};
      return true;
    default:
      return false;
  }
}

bool IsPcm(uint32_t encoding)
{
  PcmLayout layout;
  return GetPcmLayout(encoding, layout);
}

bool IsEngineLayout(const PcmLayout& layout)
{
  return !layout.repack && (layout.wireBytes == 1 || IsNativeOrder(layout.bigEndian));
}

int32_t Read24(const uint8_t* sample, const PcmLayout& layout)
{
  // the significant bytes of a 4-byte S24_32 word are its low three
  const uint8_t* bytes = layout.bigEndian ? sample + layout.wireBytes - 3 : sample;
  const uint32_t value = layout.bigEndian ? (bytes[0] << 16) | (bytes[1] << 8) | bytes[2]
                                          : bytes[0] | (bytes[1] << 8) | (bytes[2] << 16);
  return static_cast<int32_t>(value << 8);
}

void AppendPcm(const PcmLayout& layout, const uint8_t* data, size_t size, std::vector<uint8_t>& out)
{
  const size_t samples = size / layout.wireBytes;
  const size_t start = out.size();

  if (IsEngineLayout(layout))
  {
    out.insert(out.end(), data, data + samples * layout.wireBytes);
    return;
  }

  out.resize(start + samples * layout.engineBytes);
  uint8_t* dst = out.data() + start;
  for (size_t i = 0; i < samples; ++i, data += layout.wireBytes, dst += layout.engineBytes)
  {
    if (layout.repack)
    {
      const int32_t value = Read24(data, layout);
      memcpy(dst, &value, sizeof(value));
    }
    else
    {
      std::reverse_copy(data, data + layout.wireBytes, dst);
    }
  }
}

uint32_t PcmEncodings(uint32_t* encodings)
{
  constexpr uint32_t PREFERRED[] = {
      KODI_WEB_AUDIO_F32LE, KODI_WEB_AUDIO_F32BE, KODI_WEB_AUDIO_F64LE,    KODI_WEB_AUDIO_F64BE,
      KODI_WEB_AUDIO_S32LE, KODI_WEB_AUDIO_S32BE, KODI_WEB_AUDIO_S24_32LE, KODI_WEB_AUDIO_S24_32BE,
      KODI_WEB_AUDIO_S24LE, KODI_WEB_AUDIO_S24BE, KODI_WEB_AUDIO_S16LE,    KODI_WEB_AUDIO_S16BE,
      KODI_WEB_AUDIO_U8};
  uint32_t count = 0;
  for (bool direct : {true, false})
  {
    for (uint32_t encoding : PREFERRED)
    {
      PcmLayout layout;
      if (GetPcmLayout(encoding, layout) && IsEngineLayout(layout) == direct &&
          count < KODI_WEB_AUDIO_MAX_PCM)
        encodings[count++] = encoding;
    }
  }
  return count;
}

CAEStreamInfo::DataType RawType(uint32_t encoding)
{
  switch (encoding)
  {
    case KODI_WEB_AUDIO_AC3:
      return CAEStreamInfo::STREAM_TYPE_AC3;
    case KODI_WEB_AUDIO_EAC3:
      return CAEStreamInfo::STREAM_TYPE_EAC3;
    case KODI_WEB_AUDIO_DTS:
      return CAEStreamInfo::STREAM_TYPE_DTS_512;
    default:
      return CAEStreamInfo::STREAM_TYPE_NULL;
  }
}

bool SupportsPassthrough(CAEStreamInfo::DataType type, unsigned int sampleRate)
{
  IAE* ae = CServiceBroker::GetActiveAE();
  if (!ae || type == CAEStreamInfo::STREAM_TYPE_NULL)
    return false;

  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_RAW;
  format.m_streamInfo.m_type = type;
  format.m_streamInfo.m_sampleRate = sampleRate;
  return ae->SupportsRaw(format);
}

uint32_t PassthroughRates(uint32_t encoding)
{
  constexpr uint32_t RATES[] = KODI_WEB_AUDIO_RATES;
  uint32_t rates = 0;
  for (size_t i = 0; i < std::size(RATES); ++i)
  {
    if (SupportsPassthrough(RawType(encoding), RATES[i]))
      rates |= 1u << i;
  }
  return rates;
}

AEChannel PositionToChannel(uint8_t position, unsigned int index)
{
  switch (position)
  {
    case KODI_WEB_AUDIO_POS_FL:
      return AE_CH_FL;
    case KODI_WEB_AUDIO_POS_FR:
      return AE_CH_FR;
    case KODI_WEB_AUDIO_POS_FC:
      return AE_CH_FC;
    case KODI_WEB_AUDIO_POS_LFE:
      return AE_CH_LFE;
    case KODI_WEB_AUDIO_POS_BL:
      return AE_CH_BL;
    case KODI_WEB_AUDIO_POS_BR:
      return AE_CH_BR;
    case KODI_WEB_AUDIO_POS_FLOC:
      return AE_CH_FLOC;
    case KODI_WEB_AUDIO_POS_FROC:
      return AE_CH_FROC;
    case KODI_WEB_AUDIO_POS_BC:
      return AE_CH_BC;
    case KODI_WEB_AUDIO_POS_SL:
      return AE_CH_SL;
    case KODI_WEB_AUDIO_POS_SR:
      return AE_CH_SR;
    case KODI_WEB_AUDIO_POS_TFL:
      return AE_CH_TFL;
    case KODI_WEB_AUDIO_POS_TFR:
      return AE_CH_TFR;
    case KODI_WEB_AUDIO_POS_TFC:
      return AE_CH_TFC;
    case KODI_WEB_AUDIO_POS_TC:
      return AE_CH_TC;
    case KODI_WEB_AUDIO_POS_TBL:
      return AE_CH_TBL;
    case KODI_WEB_AUDIO_POS_TBR:
      return AE_CH_TBR;
    case KODI_WEB_AUDIO_POS_TBC:
      return AE_CH_TBC;
    default:
      return static_cast<AEChannel>(AE_CH_UNKNOWN1 + index);
  }
}

} // unnamed namespace

CWebAudioStream::CWebAudioStream(int fd)
  : CThread("webaudio"),
    m_fd(fd),
    m_message(KODI_WEB_AUDIO_MAX_MESSAGE),
    m_frame(MAX_IEC61937_PACKET)
{
}

CWebAudioStream::~CWebAudioStream()
{
  Stop();
  CloseStream();
  if (m_shared)
    munmap(m_shared, sizeof(*m_shared));
  for (int fd : {m_fd, m_eventFd, m_stopFd})
  {
    if (fd >= 0)
      close(fd);
  }
  delete[] m_parsed;
}

void CWebAudioStream::Start()
{
  m_stopFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (m_stopFd < 0)
  {
    CLog::Log(LOGERROR, "CWebAudioStream::{}: eventfd failed: {}", __FUNCTION__, strerror(errno));
    m_done = true;
    return;
  }
  Create();
}

void CWebAudioStream::Stop()
{
  if (m_stopFd < 0)
    return;

  m_bStop = true;
  Signal(m_stopFd);
  StopThread(true);
}

void CWebAudioStream::Process()
{
  if (!Handshake())
  {
    m_done = true;
    return;
  }

  while (!m_bStop)
  {
    ApplyControl();
    if (m_feedable && !m_paused && Feed())
      continue;

    // nothing more is read while data waits, so a full engine holds the sink back
    pollfd fds[3] = {{m_stopFd, POLLIN, 0}, {m_eventFd, POLLIN, 0}, {m_fd, POLLIN, 0}};
    const nfds_t count = m_feedable ? 2 : 3;
    if (poll(fds, count, -1) < 0)
    {
      if (errno == EINTR)
        continue;
      CLog::Log(LOGERROR, "CWebAudioStream::{}: poll failed: {}", __FUNCTION__, strerror(errno));
      break;
    }

    if (fds[0].revents)
      break;
    if (fds[1].revents)
      Drain(m_eventFd);
    if (count > 2 && (fds[2].revents & POLLIN) && !ReceiveMessage())
      break;
    if (count > 2 && !(fds[2].revents & POLLIN) && (fds[2].revents & (POLLHUP | POLLERR)))
      break;
  }

  CloseStream();
  m_done = true;
}

bool CWebAudioStream::Handshake()
{
  kodi_web_audio_hello hello{};
  if (recv(m_fd, &hello, sizeof(hello), 0) != sizeof(hello) || hello.type != KODI_WEB_AUDIO_HELLO ||
      hello.version != KODI_WEB_AUDIO_VERSION)
  {
    CLog::Log(LOGERROR, "CWebAudioStream::{}: the sink speaks another protocol", __FUNCTION__);
    return false;
  }

  const int shm = memfd_create("kodi-web-audio", MFD_CLOEXEC);
  m_eventFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (shm < 0 || m_eventFd < 0 || ftruncate(shm, sizeof(kodi_web_audio_shared)) < 0)
  {
    CLog::Log(LOGERROR, "CWebAudioStream::{}: cannot set up the shared state: {}", __FUNCTION__,
              strerror(errno));
    if (shm >= 0)
      close(shm);
    return false;
  }

  void* shared =
      mmap(nullptr, sizeof(kodi_web_audio_shared), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
  if (shared == MAP_FAILED)
  {
    CLog::Log(LOGERROR, "CWebAudioStream::{}: mmap failed: {}", __FUNCTION__, strerror(errno));
    close(shm);
    return false;
  }
  m_shared = static_cast<kodi_web_audio_shared*>(shared);

  kodi_web_audio_hello_reply reply{};
  reply.type = KODI_WEB_AUDIO_HELLO_REPLY;
  reply.version = KODI_WEB_AUDIO_VERSION;
  reply.pcm_count = PcmEncodings(reply.pcm);
  reply.passthrough[KODI_WEB_AUDIO_PT_AC3] = PassthroughRates(KODI_WEB_AUDIO_AC3);
  reply.passthrough[KODI_WEB_AUDIO_PT_EAC3] = PassthroughRates(KODI_WEB_AUDIO_EAC3);
  reply.passthrough[KODI_WEB_AUDIO_PT_DTS] = PassthroughRates(KODI_WEB_AUDIO_DTS);
  reply.max_channels = KODI_WEB_AUDIO_MAX_CHANNELS;

  const int fds[2] = {shm, m_eventFd};
  char control[CMSG_SPACE(sizeof(fds))] = {};
  iovec iov{&reply, sizeof(reply)};
  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);
  cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(fds));
  memcpy(CMSG_DATA(cmsg), fds, sizeof(fds));

  const bool sent = sendmsg(m_fd, &msg, MSG_NOSIGNAL) == sizeof(reply);
  close(shm);
  if (!sent)
  {
    CLog::Log(LOGERROR, "CWebAudioStream::{}: cannot answer the sink: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  CLog::Log(LOGINFO,
            "CWebAudioStream::{}: web audio connected, passthrough rates AC3 0x{:x}, E-AC3 0x{:x}, "
            "DTS 0x{:x}",
            __FUNCTION__, reply.passthrough[KODI_WEB_AUDIO_PT_AC3],
            reply.passthrough[KODI_WEB_AUDIO_PT_EAC3], reply.passthrough[KODI_WEB_AUDIO_PT_DTS]);
  return true;
}

bool CWebAudioStream::ReceiveMessage()
{
  const ssize_t size = recv(m_fd, m_message.data(), m_message.size(), 0);
  if (size < 0 && errno == EINTR)
    return true;
  if (size < static_cast<ssize_t>(sizeof(uint32_t)))
    return false;

  uint32_t type;
  memcpy(&type, m_message.data(), sizeof(type));
  if (type == KODI_WEB_AUDIO_FORMAT && size == sizeof(kodi_web_audio_format))
  {
    kodi_web_audio_format format;
    memcpy(&format, m_message.data(), sizeof(format));

    kodi_web_audio_format_reply reply{};
    reply.type = KODI_WEB_AUDIO_FORMAT_REPLY;
    reply.accepted = HandleFormat(format) ? 1 : 0;
    return send(m_fd, &reply, sizeof(reply), MSG_NOSIGNAL) == sizeof(reply);
  }

  if (type == KODI_WEB_AUDIO_DATA && size >= static_cast<ssize_t>(sizeof(kodi_web_audio_data)))
  {
    kodi_web_audio_data header;
    memcpy(&header, m_message.data(), sizeof(header));
    HandleData(header.generation, m_message.data() + sizeof(header), size - sizeof(header));
    return true;
  }

  CLog::Log(LOGWARNING, "CWebAudioStream::{}: ignoring message {} of {} bytes", __FUNCTION__, type,
            size);
  return true;
}

bool CWebAudioStream::HandleFormat(const kodi_web_audio_format& format)
{
  CloseStream();
  Discard();
  m_generation = format.generation;
  m_encoding = format.encoding;
  m_frameBytes = 0;
  m_wireFrameBytes = 0;

  bool accepted = false;
  PcmLayout pcm;
  if (GetPcmLayout(format.encoding, pcm) && format.channels > 0 &&
      format.channels <= KODI_WEB_AUDIO_MAX_CHANNELS)
  {
    CAEChannelInfo layout;
    for (unsigned int i = 0; i < format.channels; ++i)
      layout += PositionToChannel(format.positions[i], i);

    IAE* ae = CServiceBroker::GetActiveAE();
    m_format = AEAudioFormat();
    m_format.m_dataFormat = pcm.format;
    m_format.m_sampleRate = format.rate;
    m_format.m_channelLayout = layout;
    if (ae && layout.IsLayoutValid())
      m_stream = ae->MakeStream(m_format);
    if (m_stream && m_paused)
      m_stream->Pause();

    m_pcm = pcm;
    m_frameBytes = format.channels * pcm.engineBytes;
    m_wireFrameBytes = format.channels * pcm.wireBytes;
    accepted = m_stream != nullptr;
  }
  else if (!IsPcm(format.encoding))
  {
    accepted = SupportsPassthrough(RawType(format.encoding), format.rate);
  }

  CLog::Log(LOGINFO, "CWebAudioStream::{}: encoding {}, {} Hz, {} channels: {}", __FUNCTION__,
            format.encoding, format.rate, format.channels, accepted ? "playing" : "refused");
  Publish();
  return accepted;
}

void CWebAudioStream::HandleData(uint32_t generation, const uint8_t* data, size_t size)
{
  if (generation != m_generation || size == 0)
    return;

  if (m_pendingOffset)
  {
    m_pending.erase(m_pending.begin(), m_pending.begin() + m_pendingOffset);
    m_pendingOffset = 0;
  }
  if (IsPcm(m_encoding))
    AppendPcm(m_pcm, data, size, m_pending);
  else
    m_pending.insert(m_pending.end(), data, data + size);
  m_feedable = true;
}

void CWebAudioStream::ApplyControl()
{
  const uint32_t generation =
      std::atomic_ref<uint32_t>(m_shared->generation).load(std::memory_order_acquire);
  const bool paused = std::atomic_ref<uint32_t>(m_shared->paused).load(std::memory_order_acquire);
  bool changed = false;

  if (generation != m_generation)
  {
    Discard();
    m_generation = generation;
    changed = true;
  }

  if (paused != m_paused)
  {
    m_paused = paused;
    if (m_stream && paused)
      m_stream->Pause();
    else if (m_stream)
      m_stream->Resume();
    changed = true;
  }

  if (changed)
    Publish();
}

void CWebAudioStream::Discard()
{
  if (m_stream)
    m_stream->Flush();
  m_pending.clear();
  m_pendingOffset = 0;
  m_packets.clear();
  m_parser.Reset();
  m_unaccounted = 0;
  m_consumed = 0;
  m_feedable = false;
}

void CWebAudioStream::CloseStream()
{
  if (!m_stream)
    return;

  m_stream.get_deleter().setFinish(false);
  m_stream.reset();
}

bool CWebAudioStream::Feed()
{
  return IsPcm(m_encoding) ? FeedPcm() : FeedRaw();
}

bool CWebAudioStream::FeedPcm()
{
  if (!m_stream)
  {
    m_pending.clear();
    m_pendingOffset = 0;
    m_feedable = false;
    return false;
  }

  const size_t available = m_pending.size() - m_pendingOffset;
  const unsigned int frames = available / m_frameBytes;
  if (frames == 0)
  {
    m_feedable = false;
    return false;
  }

  const uint8_t* data = m_pending.data() + m_pendingOffset;
  const unsigned int added = m_stream->AddData(&data, 0, frames, nullptr);
  m_pendingOffset += added * m_frameBytes;
  m_consumed += added * m_wireFrameBytes;
  Publish();

  if (added == frames && available - added * m_frameBytes < m_frameBytes)
  {
    m_feedable = false;
    return false;
  }
  return true;
}

bool CWebAudioStream::FeedRaw()
{
  while (true)
  {
    while (!m_packets.empty())
    {
      if (!AddPacket(m_packets.front()))
      {
        if (m_stream)
          return true;
        m_packets.clear();
        m_feedable = false;
        return false;
      }
      m_consumed += m_packets.front().bytes;
      m_packets.pop_front();
      Publish();
    }

    if (!Unpack())
    {
      m_feedable = false;
      return false;
    }
  }
}

bool CWebAudioStream::Unpack()
{
  unsigned int offset = m_pendingOffset;
  const unsigned int size =
      CAEPackIEC61937::Unpack(m_pending.data(), m_pending.size(), &offset, m_frame.data());
  m_unaccounted += offset - m_pendingOffset;
  m_pendingOffset = offset;
  if (size == 0)
    return false;

  uint8_t* data = m_frame.data();
  unsigned int left = size;
  while (left)
  {
    unsigned int parsed = m_parsedCapacity;
    const int used = m_parser.AddData(data, left, &m_parsed, &parsed);
    m_parsedCapacity = std::max(m_parsedCapacity, parsed);
    data += used;
    left -= used;

    if (parsed)
    {
      m_packets.push_back({std::vector<uint8_t>(m_parsed, m_parsed + parsed),
                           m_parser.GetStreamInfo(), m_parser.GetSampleRate(),
                           m_parser.GetChannels(), m_unaccounted});
      m_unaccounted = 0;
    }
    else if (used == 0)
    {
      break;
    }
  }
  return true;
}

bool CWebAudioStream::AddPacket(const Packet& packet)
{
  if (!m_stream || !(m_format.m_streamInfo == packet.info) ||
      m_format.m_sampleRate != packet.sampleRate ||
      m_format.m_channelLayout.Count() != packet.channels)
  {
    CloseStream();

    IAE* ae = CServiceBroker::GetActiveAE();
    m_format = AEAudioFormat();
    m_format.m_dataFormat = AE_FMT_RAW;
    m_format.m_streamInfo = packet.info;
    m_format.m_sampleRate = packet.sampleRate;
    m_format.m_frameSize = 1;
    for (unsigned int i = 0; i < packet.channels; ++i)
      m_format.m_channelLayout += AE_CH_RAW;

    if (ae)
      m_stream = ae->MakeStream(m_format);
    if (!m_stream)
    {
      CLog::Log(LOGERROR, "CWebAudioStream::{}: cannot open a passthrough stream", __FUNCTION__);
      return false;
    }
    if (m_paused)
      m_stream->Pause();
  }

  const uint8_t* data = packet.data.data();
  return m_stream->AddData(&data, 0, packet.data.size(), nullptr) > 0;
}

void CWebAudioStream::Publish()
{
  std::atomic_ref<uint32_t> sequence(m_shared->sequence);
  const uint32_t current = sequence.load(std::memory_order_relaxed);

  sequence.store(current + 1, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_release);
  m_shared->applied_generation = m_generation;
  m_shared->consumed = m_consumed;
  m_shared->delay_ns = m_stream ? std::llround(m_stream->GetDelay() * 1e9) : 0;
  m_shared->stamp_ns = MonotonicNs();
  m_shared->playing = m_stream && !m_paused ? 1 : 0;
  sequence.store(current + 2, std::memory_order_release);
}

CWebAudioServer::CWebAudioServer() : CThread("webaudioserver")
{
}

CWebAudioServer::~CWebAudioServer()
{
  Stop();
}

bool CWebAudioServer::Start(const std::string& path)
{
  sockaddr_un addr{};
  if (path.size() >= sizeof(addr.sun_path))
  {
    CLog::Log(LOGERROR, "CWebAudioServer::{}: socket path {} is too long", __FUNCTION__, path);
    return false;
  }
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, path.c_str(), path.size());

  m_listenFd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  m_stopFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  unlink(path.c_str());
  if (m_listenFd < 0 || m_stopFd < 0 ||
      bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
      chmod(path.c_str(), S_IRUSR | S_IWUSR) < 0 || listen(m_listenFd, SOMAXCONN) < 0)
  {
    CLog::Log(LOGERROR, "CWebAudioServer::{}: cannot serve {}: {}", __FUNCTION__, path,
              strerror(errno));
    for (int* fd : {&m_listenFd, &m_stopFd})
    {
      if (*fd >= 0)
        close(*fd);
      *fd = -1;
    }
    return false;
  }

  m_path = path;
  Create();
  CLog::Log(LOGINFO, "CWebAudioServer::{}: web audio at {}", __FUNCTION__, path);
  return true;
}

void CWebAudioServer::Stop()
{
  if (m_listenFd < 0)
    return;

  m_bStop = true;
  Signal(m_stopFd);
  StopThread(true);
  m_streams.clear();

  close(m_listenFd);
  close(m_stopFd);
  m_listenFd = -1;
  m_stopFd = -1;
  unlink(m_path.c_str());
}

void CWebAudioServer::Process()
{
  while (!m_bStop)
  {
    pollfd fds[2] = {{m_stopFd, POLLIN, 0}, {m_listenFd, POLLIN, 0}};
    if (poll(fds, 2, -1) < 0)
    {
      if (errno == EINTR)
        continue;
      CLog::Log(LOGERROR, "CWebAudioServer::{}: poll failed: {}", __FUNCTION__, strerror(errno));
      return;
    }

    if (fds[0].revents)
      return;
    if (fds[1].revents)
      Accept();
  }
}

void CWebAudioServer::Accept()
{
  const int fd = accept4(m_listenFd, nullptr, nullptr, SOCK_CLOEXEC);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "CWebAudioServer::{}: accept failed: {}", __FUNCTION__, strerror(errno));
    return;
  }

  std::erase_if(m_streams, [](const auto& stream) { return stream->IsDone(); });
  auto stream = std::make_unique<CWebAudioStream>(fd);
  stream->Start();
  m_streams.push_back(std::move(stream));
}
