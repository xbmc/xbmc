/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include "platform/posix/utils/FileHandle.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>

typedef struct _GstElement GstElement;
typedef struct _GstPad GstPad;
typedef struct _GstPadProbeInfo GstPadProbeInfo;
typedef struct _GstSegment GstSegment;

class CDVDAudioCodecPassthrough;
class CDVDStreamInfo;
class CProcessInfo;
struct DemuxPacket;

class CAudioBypassWebOS
{
public:
  CAudioBypassWebOS(CProcessInfo& processInfo, CAEStreamInfo::DataType type, int profile);
  explicit CAudioBypassWebOS(const AEAudioFormat& format);
  ~CAudioBypassWebOS();

  static std::optional<CAEStreamInfo::DataType> GetPassthroughType(const CDVDStreamInfo& hint);
  static std::optional<AEAudioFormat> GetPCMFormat(const CDVDStreamInfo& hint);

  const AEAudioFormat& GetFormat() const { return m_format; }
  bool IsBitstream() const { return m_parser != nullptr; }
  bool IsDecoding() const { return m_idleParser != nullptr; }
  std::string GetName() const;
  unsigned int GetQueuedBytes() const;

  void Attach(GstElement* pipeline, GstElement* videoSink, const char* mediaId);
  void SetPlaying(bool playing);
  void Update(bool started);
  void Decode();
  void Bitstream();
  void Flush();
  bool AddPacket(const DemuxPacket& packet);

private:
  bool CreatePipeline(unsigned int rate, unsigned int channels);
  void DestroyPipeline();
  void Report(const std::string& pipelineInfo, const std::string& categoryInfo);
  bool IsMixed() const;
  void MixOutput();
  void Sync();
  void OnBuffer(GstPadProbeInfo* info);

  void Push(const GstSegment& segment, double pts, const std::uint8_t* data, unsigned int size);

  std::unique_ptr<CDVDAudioCodecPassthrough> m_parser;
  std::unique_ptr<CDVDAudioCodecPassthrough> m_idleParser;
  CAEBitstreamPacker m_packer;
  AEAudioFormat m_format;
  AEAudioFormat m_idleFormat;
  double m_nextPts{0.0};
  bool m_atmos{false};
  KODI::UTILS::POSIX::CFileHandle m_controlFd;
  KODI::UTILS::POSIX::CFileHandle m_ipcFd;

  mutable std::mutex m_mutex;
  GstElement* m_pipeline{nullptr};
  GstElement* m_source{nullptr};
  GstElement* m_sink{nullptr};
  GstElement* m_starfish{nullptr};
  GstPad* m_videoPad{nullptr};
  std::string m_umiPipeline;
  unsigned int m_rate{0};
  unsigned int m_channels{0};
  std::uint64_t m_staleBaseTime{UINT64_MAX};
  std::uint32_t m_staleSegment{0};
  bool m_playing{true};
  bool m_started{false};
  std::set<unsigned int> m_preexisting;
  unsigned int m_buffers{0};
};
