/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "threads/Thread.h"

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

struct kodi_web_audio_format;
struct kodi_web_audio_shared;

namespace KODI
{
namespace WEB
{

/*!
 * \brief How a PCM encoding of the web audio protocol reaches the audio engine
 */
struct PcmLayout
{
  AEDataFormat format;
  unsigned int wireBytes;
  unsigned int engineBytes;
  bool bigEndian;
  bool repack;
};

/*!
 * \brief One audio sink of the web client, played as a stream of Kodi's
 * audio engine. Runs a thread of its own for the connection.
 */
class CWebAudioStream : private CThread
{
public:
  explicit CWebAudioStream(int fd);
  ~CWebAudioStream() override;

  void Start();
  void Stop();
  bool IsDone() const { return m_done; }

private:
  struct Packet
  {
    std::vector<uint8_t> data;
    CAEStreamInfo info;
    unsigned int sampleRate;
    unsigned int channels;
    uint64_t bytes;
  };

  void Process() override;
  bool Handshake();
  bool ReceiveMessage();
  bool HandleFormat(const kodi_web_audio_format& format);
  void HandleData(uint32_t generation, const uint8_t* data, size_t size);
  void ApplyControl();
  void Discard();
  void CloseStream();
  bool Feed();
  bool FeedPcm();
  bool FeedRaw();
  bool Unpack();
  bool AddPacket(const Packet& packet);
  void Publish();

  int m_fd;
  int m_eventFd{-1};
  int m_stopFd{-1};
  kodi_web_audio_shared* m_shared{nullptr};
  std::atomic<bool> m_done{false};
  std::vector<uint8_t> m_message;

  IAE::StreamPtr m_stream;
  AEAudioFormat m_format;
  uint32_t m_encoding{0};
  PcmLayout m_pcm{};
  unsigned int m_frameBytes{0};
  unsigned int m_wireFrameBytes{0};
  uint32_t m_generation{0};
  bool m_paused{false};
  bool m_feedable{false};
  uint64_t m_consumed{0};

  std::vector<uint8_t> m_pending;
  size_t m_pendingOffset{0};

  CAEStreamParser m_parser;
  std::vector<uint8_t> m_frame;
  uint8_t* m_parsed{nullptr};
  unsigned int m_parsedCapacity{0};
  uint64_t m_unaccounted{0};
  std::deque<Packet> m_packets;
};

/*!
 * \brief Serves the socket the web client's audio sinks connect to, so the
 * audio of web pages plays through Kodi's audio engine
 */
class CWebAudioServer : private CThread
{
public:
  CWebAudioServer();
  ~CWebAudioServer() override;

  bool Start(const std::string& path);
  void Stop();
  const std::string& GetPath() const { return m_path; }

private:
  void Process() override;
  void Accept();

  std::string m_path;
  int m_listenFd{-1};
  int m_stopFd{-1};
  std::vector<std::unique_ptr<CWebAudioStream>> m_streams;
};

} // namespace WEB
} // namespace KODI
