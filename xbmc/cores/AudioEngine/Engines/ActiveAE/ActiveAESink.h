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
#include "Interfaces/AE.h"
#include "Interfaces/AESink.h"
#include "AESinkFactory.h"
#include "ActiveAEResample.h"
#include "Utils/AEConvert.h"

namespace ActiveAE
{
using namespace Actor;

class CEngineStats;

struct SinkConfig
{
  AEAudioFormat format;
  CEngineStats *stats;
};

class CSinkControlProtocol : public Protocol
{
public:
  CSinkControlProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    CONFIGURE,
    UNCONFIGURE,
    SILENCEMODE,
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
  void EnumerateSinkList();
  void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  std::string GetDefaultDevice(bool passthrough);
  void Start();
  void Dispose();
  bool IsCompatible(const AEAudioFormat format, const std::string &device);
  bool HasVolume();
  CSinkControlProtocol m_controlPort;
  CSinkDataProtocol m_dataPort;

protected:
  void Process();
  void StateMachine(int signal, Protocol *port, Message *msg);
  void PrintSinks();
  void GetDeviceFriendlyName(std::string &device);
  void OpenSink();
  void ReturnBuffers();

  unsigned int OutputSamples(CSampleBuffer* samples);
  void ConvertInit(CSampleBuffer* samples);
  inline void EnsureConvertBuffer(CSampleBuffer* samples);
  inline uint8_t* Convert(CSampleBuffer* samples);

  void GenerateNoise();

  CEvent m_outMsgEvent;
  CEvent *m_inMsgEvent;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  int m_extTimeout;
  bool m_extError;
  bool m_extSilence;
  int m_extCycleCounter;

  CSampleBuffer m_sampleOfSilence;
  CSampleBuffer m_sampleOfNoise;
  uint8_t *m_convertBuffer;
  int m_convertBufferSampleSize;
  CAEConvert::AEConvertFrFn m_convertFn;
  enum
  {
    CHECK_CONVERT,
    NEED_CONVERT,
    NEED_BYTESWAP,
    SKIP_CONVERT,
  } m_convertState;

  std::string m_deviceFriendlyName;
  AESinkInfoList m_sinkInfoList;
  IAESink *m_sink;
  AEAudioFormat m_sinkFormat, m_requestedFormat;
  CEngineStats *m_stats;
  float m_volume;
};

}
