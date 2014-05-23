#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/ActorProtocol.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResample.h"
#include "cores/AudioEngine/Utils/AEConvert.h"

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
  CSinkControlProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    CONFIGURE,
    UNCONFIGURE,
    STREAMING,
    APPFOCUSED,
    VOLUME,
    FLUSH,
    TIMEOUT,
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
  CSinkDataProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
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
  CActiveAESink(CEvent *inMsgEvent);
  void EnumerateSinkList(bool force);
  void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  std::string GetDefaultDevice(bool passthrough);
  void Start();
  void Dispose();
  AEDeviceType GetDeviceType(const std::string &device);
  bool HasPassthroughDevice();
  bool SupportsFormat(const std::string &device, AEDataFormat format, int samplerate);
  CSinkControlProtocol m_controlPort;
  CSinkDataProtocol m_dataPort;

protected:
  void Process();
  void StateMachine(int signal, Protocol *port, Message *msg);
  void PrintSinks();
  void GetDeviceFriendlyName(std::string &device);
  void OpenSink();
  void ReturnBuffers();
  void SetSilenceTimer();

  unsigned int OutputSamples(CSampleBuffer* samples);
  void ConvertInit(CSampleBuffer* samples);
  inline void EnsureConvertBuffer(CSampleBuffer* samples);
  inline void Convert(CSampleBuffer* samples);

  void GenerateNoise();

  CEvent m_outMsgEvent;
  CEvent *m_inMsgEvent;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  int m_extTimeout;
  bool m_extError;
  unsigned int m_extSilenceTimeout;
  bool m_extAppFocused;
  bool m_extStreaming;
  XbmcThreads::EndTime m_extSilenceTimer;

  CSampleBuffer m_sampleOfSilence;
  CSampleBuffer m_convertBuffer;
  CAEConvert::AEConvertFrFn m_convertFn;
  enum
  {
    CHECK_CONVERT,
    NEED_CONVERT,
    NEED_BYTESWAP,
    SKIP_CONVERT,
  } m_convertState;

  std::string m_deviceFriendlyName;
  std::string m_device;
  AESinkInfoList m_sinkInfoList;
  IAESink *m_sink;
  AEAudioFormat m_sinkFormat, m_requestedFormat;
  CEngineStats *m_stats;
  float m_volume;
};

}
