/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "threads/Event.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"
#include "utils/ActorProtocol.h"

#include <memory>
#include <utility>

class CAEBitstreamPacker;

namespace ActiveAE
{
using namespace Actor;

class CEngineStats;

struct SinkConfig
{
  AEAudioFormat format;
  CEngineStats *stats;
  const std::string *device;
};

struct SinkReply
{
  AEAudioFormat format;
  float cacheTotal;
  float latency;
  bool hasVolume;
};

class CSinkControlProtocol : public Protocol
{
public:
  CSinkControlProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    CONFIGURE,
    UNCONFIGURE,
    STREAMING,
    APPFOCUSED,
    VOLUME,
    FLUSH,
    TIMEOUT,
    SETSILENCETIMEOUT,
    SETNOISETYPE,
  };
  enum InSignal
  {
    ACC,
    ERR,
    STATS,
  };
};

class CSinkDataProtocol : public Protocol
{
public:
  CSinkDataProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    SAMPLE = 0,
    DRAIN,
  };
  enum InSignal
  {
    RETURNSAMPLE,
    ACC,
  };
};

class CActiveAESink : private CThread
{
public:
  explicit CActiveAESink(CEvent *inMsgEvent);
  ~CActiveAESink();

  void EnumerateSinkList(bool force, std::string driver);
  void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  std::string ValidateOuputDevice(const std::string& device, bool passthrough) const;
  void Start();
  void Dispose();
  AEDeviceType GetDeviceType(const std::string &device);
  bool HasPassthroughDevice();
  bool SupportsFormat(const std::string &device, AEAudioFormat &format);
  bool DeviceExist(std::string driver, const std::string& device);
  bool NeedIecPack() const { return m_needIecPack; }
  CSinkControlProtocol m_controlPort;
  CSinkDataProtocol m_dataPort;

protected:
  void Process() override;
  void StateMachine(int signal, Protocol *port, Message *msg);
  void PrintSinks(std::string& driver);
  void GetDeviceFriendlyName(const std::string& device);
  void OpenSink();
  void ReturnBuffers();
  void SetSilenceTimer();
  bool NeedIECPacking();

  unsigned int OutputSamples(CSampleBuffer* samples);
  void SwapInit(CSampleBuffer* samples);

  void GenerateNoise();

  CEvent m_outMsgEvent;
  CEvent *m_inMsgEvent;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  std::chrono::milliseconds m_extTimeout;
  std::chrono::minutes m_silenceTimeOut{std::chrono::minutes::zero()};
  bool m_extError;
  std::chrono::milliseconds m_extSilenceTimeout;
  bool m_extAppFocused;
  bool m_extStreaming;
  XbmcThreads::EndTime<> m_extSilenceTimer;

  CSampleBuffer m_sampleOfSilence;
  enum
  {
    CHECK_SWAP,
    NEED_CONVERT,
    NEED_BYTESWAP,
    SKIP_SWAP,
  } m_swapState;

  std::vector<uint8_t> m_mergeBuffer;

  std::string m_deviceFriendlyName;
  std::string m_device;
  std::vector<AE::AESinkInfo> m_sinkInfoList;
  std::unique_ptr<IAESink> m_sink;
  AEAudioFormat m_sinkFormat, m_requestedFormat;
  CEngineStats *m_stats;
  float m_volume;
  int m_sinkLatency;
  std::unique_ptr<CAEBitstreamPacker> m_packer;
  bool m_needIecPack{false};
  bool m_streamNoise;
};

}
