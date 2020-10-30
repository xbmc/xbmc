/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActiveAE.h"

using namespace AE;
using namespace ActiveAE;
#include "ActiveAESettings.h"
#include "ActiveAESound.h"
#include "ActiveAEStream.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/AudioEngine/AEResampleFactory.h"
#include "cores/AudioEngine/Encoders/AEEncoderFFmpeg.h"

#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "windowing/WinSystem.h"
#include "utils/log.h"

#define MAX_CACHE_LEVEL 0.4   // total cache time of stream in seconds
#define MAX_WATER_LEVEL 0.2   // buffered time after stream stages in seconds
#define MAX_BUFFER_TIME 0.1   // max time of a buffer in seconds

void CEngineStats::Reset(unsigned int sampleRate, bool pcm)
{
  CSingleLock lock(m_lock);
  m_sinkDelay.SetDelay(0.0);
  m_sinkSampleRate = sampleRate;
  m_bufferedSamples = 0;
  m_suspended = false;
  m_pcmOutput = pcm;
}

void CEngineStats::UpdateSinkDelay(const AEDelayStatus& status, int samples)
{
  CSingleLock lock(m_lock);
  m_sinkDelay = status;
  if (samples > m_bufferedSamples)
  {
    CLog::Log(LOGERROR, "CEngineStats::UpdateSinkDelay - inconsistency in buffer time");
  }
  else
    m_bufferedSamples -= samples;
}

void CEngineStats::AddSamples(int samples, std::list<CActiveAEStream*> &streams)
{
  CSingleLock lock(m_lock);
  m_bufferedSamples += samples;

  for (auto stream : streams)
  {
    UpdateStream(stream);
  }
}

void CEngineStats::GetDelay(AEDelayStatus& status)
{
  CSingleLock lock(m_lock);
  status = m_sinkDelay;
  if (m_pcmOutput)
    status.delay += (double)m_bufferedSamples / m_sinkSampleRate;
  else
    status.delay += (double)m_bufferedSamples * m_sinkFormat.m_streamInfo.GetDuration() / 1000;
}

void CEngineStats::AddStream(unsigned int streamid)
{
  StreamStats stream;
  stream.m_streamId = streamid;
  stream.m_bufferedTime = 0;
  stream.m_resampleRatio = 1.0;
  stream.m_syncError = 0;
  stream.m_syncState = CAESyncInfo::AESyncState::SYNC_OFF;
  m_streamStats.push_back(stream);
}

void CEngineStats::RemoveStream(unsigned int streamid)
{
  for (auto it = m_streamStats.begin(); it != m_streamStats.end(); ++it)
  {
    if (it->m_streamId == streamid)
    {
      m_streamStats.erase(it);
      return;
    }
  }
}

void CEngineStats::UpdateStream(CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  for (auto &str : m_streamStats)
  {
    if (str.m_streamId == stream->m_id)
    {
      float delay = 0;
      str.m_syncState = stream->m_syncState;
      str.m_syncError = stream->m_syncError.GetLastError(str.m_errorTime);
      if (stream->m_processingBuffers)
      {
        str.m_resampleRatio = stream->m_processingBuffers->GetRR();
        delay += stream->m_processingBuffers->GetDelay();
      }
      else
      {
        str.m_resampleRatio = 1.0;
      }

      CSingleLock lock(stream->m_statsLock);
      std::deque<CSampleBuffer*>::iterator itBuf;
      for(itBuf=stream->m_processingSamples.begin(); itBuf!=stream->m_processingSamples.end(); ++itBuf)
      {
        if (m_pcmOutput)
          delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
        else
          delay += m_sinkFormat.m_streamInfo.GetDuration() / 1000;
      }
      str.m_bufferedTime = delay;
      stream->m_bufferedTime = 0;
      break;
    }
  }
}

// this is used to sync a/v so we need to add sink latency here
void CEngineStats::GetDelay(AEDelayStatus& status, CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  status = m_sinkDelay;
  status.delay += m_sinkLatency;
  if (m_pcmOutput)
    status.delay += (double)m_bufferedSamples / m_sinkSampleRate;
  else
    status.delay += (double)m_bufferedSamples * m_sinkFormat.m_streamInfo.GetDuration() / 1000;

  for (auto &str : m_streamStats)
  {
    if (str.m_streamId == stream->m_id)
    {
      CSingleLock lock(stream->m_statsLock);
      float buffertime = str.m_bufferedTime + stream->m_bufferedTime;
      status.delay += buffertime / str.m_resampleRatio;
      return;
    }
  }
}

// this is used to sync a/v so we need to add sink latency here
void CEngineStats::GetSyncInfo(CAESyncInfo& info, CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  AEDelayStatus status;
  status = m_sinkDelay;
  if (m_pcmOutput)
    status.delay += (double)m_bufferedSamples / m_sinkSampleRate;
  else
    status.delay += (double)m_bufferedSamples * m_sinkFormat.m_streamInfo.GetDuration() / 1000;

  status.delay += m_sinkLatency;

  for (auto &str : m_streamStats)
  {
    if (str.m_streamId == stream->m_id)
    {
      CSingleLock lock(stream->m_statsLock);
      float buffertime = str.m_bufferedTime + stream->m_bufferedTime;
      status.delay += buffertime / str.m_resampleRatio;
      info.delay = status.GetDelay();
      info.error = str.m_syncError;
      info.errortime = str.m_errorTime;
      info.state = str.m_syncState;
      info.rr = str.m_resampleRatio;
      return;
    }
  }
}

float CEngineStats::GetCacheTime(CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  float delay = 0;

  for (auto &str : m_streamStats)
  {
    if (str.m_streamId == stream->m_id)
    {
      CSingleLock lock(stream->m_statsLock);
      float buffertime = str.m_bufferedTime + stream->m_bufferedTime;
      delay += buffertime / str.m_resampleRatio;
      break;
    }
  }
  return delay;
}

float CEngineStats::GetCacheTotal()
{
  return MAX_CACHE_LEVEL;
}

float CEngineStats::GetMaxDelay() const
{
  return MAX_CACHE_LEVEL + MAX_WATER_LEVEL + m_sinkCacheTotal;
}

float CEngineStats::GetWaterLevel()
{
  CSingleLock lock(m_lock);
  if (m_pcmOutput)
    return (float)m_bufferedSamples / m_sinkSampleRate;
  else
    return (float)m_bufferedSamples * m_sinkFormat.m_streamInfo.GetDuration() / 1000;
}

void CEngineStats::SetSuspended(bool state)
{
  CSingleLock lock(m_lock);
  m_suspended = state;
}

bool CEngineStats::IsSuspended()
{
  CSingleLock lock(m_lock);
  return m_suspended;
}

void CEngineStats::SetCurrentSinkFormat(const AEAudioFormat& SinkFormat)
{
  CSingleLock lock(m_lock);
  m_sinkFormat = SinkFormat;
}

AEAudioFormat CEngineStats::GetCurrentSinkFormat()
{
  CSingleLock lock(m_lock);
  return m_sinkFormat;
}

CActiveAE::CActiveAE() :
  CThread("ActiveAE"),
  m_controlPort("OutputControlPort", &m_inMsgEvent, &m_outMsgEvent),
  m_dataPort("OutputDataPort", &m_inMsgEvent, &m_outMsgEvent),
  m_sink(&m_outMsgEvent)
{
  m_sinkBuffers = NULL;
  m_silenceBuffers = NULL;
  m_encoderBuffers = NULL;
  m_vizBuffers = NULL;
  m_vizBuffersInput = NULL;
  m_volume = 1.0;
  m_volumeScaled = 1.0;
  m_aeVolume = 1.0;
  m_muted = false;
  m_aeMuted = false;
  m_mode = MODE_PCM;
  m_encoder = NULL;
  m_vizInitialized = false;
  m_sinkHasVolume = false;
  m_aeGUISoundForce = false;
  m_stats.Reset(44100, true);
  m_streamIdGen = 0;

  m_settingsHandler.reset(new CActiveAESettings(*this));
}

CActiveAE::~CActiveAE()
{
  m_settingsHandler.reset();

  Dispose();
}

void CActiveAE::Dispose()
{
  if (m_isWinSysReg)
  {
    CWinSystemBase *winsystem = CServiceBroker::GetWinSystem();
    if (winsystem)
      winsystem->Unregister(this);
    m_isWinSysReg = false;
  }

  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();
  m_sink.Dispose();
}

//-----------------------------------------------------------------------------
// Behavior
//-----------------------------------------------------------------------------

enum AE_STATES
{
  AE_TOP = 0,                      // 0
  AE_TOP_WAIT_PRECOND,             // 1
  AE_TOP_ERROR,                    // 2
  AE_TOP_UNCONFIGURED,             // 3
  AE_TOP_RECONFIGURING,            // 4
  AE_TOP_CONFIGURED,               // 5
  AE_TOP_CONFIGURED_SUSPEND,       // 6
  AE_TOP_CONFIGURED_IDLE,          // 6
  AE_TOP_CONFIGURED_PLAY,          // 7
};

int AE_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //AE_TOP_WAIT_PRECOND
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    0, //TOP_RECONFIGURING
    5, //TOP_CONFIGURED_SUSPEND
    5, //TOP_CONFIGURED_IDLE
    5, //TOP_CONFIGURED_PLAY
};

void CActiveAE::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = AE_parentStates[state])
  {
    switch (state)
    {
    case AE_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::GETSTATE:
          msg->Reply(CActiveAEControlProtocol::ACC, &m_state, sizeof(m_state));
          return;
        case CActiveAEControlProtocol::VOLUME:
          m_volume = *(float*)msg->data;
          m_volumeScaled = CAEUtil::GainToScale(CAEUtil::PercentToGain(m_volume));
          if (m_sinkHasVolume)
            m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::VOLUME, &m_volume, sizeof(float));
          return;
        case CActiveAEControlProtocol::MUTE:
          m_muted = *(bool*)msg->data;
          return;
        case CActiveAEControlProtocol::KEEPCONFIG:
          m_extKeepConfig = *(unsigned int*)msg->data;
          return;
        case CActiveAEControlProtocol::DISPLAYRESET:
          return;
        case CActiveAEControlProtocol::APPFOCUSED:
          m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::APPFOCUSED, msg->data, sizeof(bool));
          return;
        case CActiveAEControlProtocol::STREAMRESAMPLEMODE:
          MsgStreamParameter *par;
          par = reinterpret_cast<MsgStreamParameter*>(msg->data);
          if (par->stream)
          {
            par->stream->m_resampleMode = par->parameter.int_par;
            par->stream->m_resampleIntegral = 0.0;
          }
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CActiveAEDataProtocol::NEWSOUND:
          CActiveAESound *sound;
          sound = *(CActiveAESound**)msg->data;
          if (sound)
          {
            m_sounds.push_back(sound);
            ResampleSounds();
          }
          return;
        case CActiveAEDataProtocol::FREESTREAM:
          MsgStreamFree *msgStreamFree;
          msgStreamFree = *(MsgStreamFree**)msg->data;
          DiscardStream(msgStreamFree->stream);
          msg->Reply(CActiveAEDataProtocol::ACC);
          return;
        case CActiveAEDataProtocol::FREESOUND:
          sound = *(CActiveAESound**)msg->data;
          DiscardSound(sound);
          return;
        case CActiveAEDataProtocol::DRAINSTREAM:
          CActiveAEStream *stream;
          stream = *(CActiveAEStream**)msg->data;
          stream->m_drain = true;
          stream->m_processingBuffers->SetDrain(true);
          msg->Reply(CActiveAEDataProtocol::ACC);
          stream->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMDRAINED);
          return;
        default:
          break;
        }
      }
      else if (port == &m_sink.m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::RETURNSAMPLE:
          CSampleBuffer **buffer;
          buffer = (CSampleBuffer**)msg->data;
          if (buffer)
          {
            (*buffer)->Return();
          }
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "CActiveAE::%s - signal: %d from port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case AE_TOP_WAIT_PRECOND:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
          case CActiveAEControlProtocol::INIT:
            LoadSettings();
            if (!m_settings.device.empty() && CAESinkFactory::HasSinks())
            {
              m_state = AE_TOP_UNCONFIGURED;
              m_bStateMachineSelfTrigger = true;
            }
            else
            {
              // Application can't handle error case and work without an AE
              msg->Reply(CActiveAEControlProtocol::ACC);
            }
            return;

          case CActiveAEControlProtocol::DEVICECHANGE:
          case CActiveAEControlProtocol::DEVICECOUNTCHANGE:
            LoadSettings();
            if (!m_settings.device.empty() && CAESinkFactory::HasSinks())
            {
              m_controlPort.SendOutMessage(CActiveAEControlProtocol::INIT);
            }
            return;

          default:
            break;
        }
      }
      break;

    case AE_TOP_ERROR:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          m_extError = false;
          LoadSettings();
          Configure();
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_PLAY;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 500;
          }
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::INIT:
          m_extError = false;
          m_sink.EnumerateSinkList(false, "");
          LoadSettings();
          Configure();
          if (!m_isWinSysReg)
          {
            CWinSystemBase *winsystem = CServiceBroker::GetWinSystem();
            if (winsystem)
            {
              winsystem->Register(this);
              m_isWinSysReg = true;
            }
          }
          msg->Reply(CActiveAEControlProtocol::ACC);
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_IDLE;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 500;
          }
          return;

        default:
          break;
        }
      }
      break;

    case AE_TOP_RECONFIGURING:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          // drain
          if (RunStages())
          {
            m_extTimeout = 0;
            return;
          }
          if (!m_sinkBuffers->m_inputSamples.empty() || !m_sinkBuffers->m_outputSamples.empty())
          {
            m_extTimeout = 100;
            return;
          }
          if (NeedReconfigureSink())
            DrainSink();

          if (!m_extError)
            Configure();
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_PLAY;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 500;
          }
          m_extDeferData = false;
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        bool streaming;
        switch (signal)
        {
        case CActiveAEControlProtocol::RECONFIGURE:
        {
          if (m_streams.empty())
          {
            streaming = false;
            m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::STREAMING, &streaming, sizeof(bool));
          }
          LoadSettings();
          m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SETNOISETYPE, &m_settings.streamNoise, sizeof(bool));
          m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SETSILENCETIMEOUT, &m_settings.silenceTimeout, sizeof(int));
          ChangeResamplers();
          if (!NeedReconfigureBuffers() && !NeedReconfigureSink())
            return;
          m_state = AE_TOP_RECONFIGURING;
          m_extTimeout = 0;
          // don't accept any data until we are reconfigured
          m_extDeferData = true;
          return;
        }
        case CActiveAEControlProtocol::SUSPEND:
          UnconfigureSink();
          m_stats.SetSuspended(true);
          m_state = AE_TOP_CONFIGURED_SUSPEND;
          m_extDeferData = true;
          m_extSuspended = true;
          return;
        case CActiveAEControlProtocol::DISPLAYLOST:
          if (m_sink.GetDeviceType(m_mode == MODE_PCM ? m_settings.device : m_settings.passthroughdevice) == AE_DEVTYPE_HDMI)
          {
            UnconfigureSink();
            m_stats.SetSuspended(true);
            m_state = AE_TOP_CONFIGURED_SUSPEND;
            m_extDeferData = true;
          }
          msg->Reply(CActiveAEControlProtocol::ACC);
          return;
        case CActiveAEControlProtocol::DEVICECHANGE:
          time_t now;
          time(&now);
          CLog::Log(LOGDEBUG,"CActiveAE - device change event");
          while (!m_extLastDeviceChange.empty() && (now - m_extLastDeviceChange.front() > 0))
          {
            m_extLastDeviceChange.pop();
          }
          if (m_extLastDeviceChange.size() > 2)
          {
            CLog::Log(LOGWARNING,"CActiveAE - received %ld device change events within one second", m_extLastDeviceChange.size());
            return;
          }
          m_extLastDeviceChange.push(now);
          UnconfigureSink();
          m_controlPort.PurgeOut(CActiveAEControlProtocol::DEVICECHANGE);
          m_sink.EnumerateSinkList(true, "");
          LoadSettings();
          m_extError = false;
          Configure();
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_PLAY;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 500;
          }
          return;
        case CActiveAEControlProtocol::DEVICECOUNTCHANGE:
          const char* param;
          param = reinterpret_cast<const char*>(msg->data);
          CLog::Log(LOGDEBUG, "CActiveAE - device count change event from driver: %s", param);
          m_sink.EnumerateSinkList(true, param);
          if (!m_sink.DeviceExist(m_settings.driver, m_currDevice))
          {
            UnconfigureSink();
            LoadSettings();
            m_extError = false;
            Configure();
            if (!m_extError)
            {
              m_state = AE_TOP_CONFIGURED_PLAY;
              m_extTimeout = 0;
            }
            else
            {
              m_state = AE_TOP_ERROR;
              m_extTimeout = 500;
            }
          }
          return;
        case CActiveAEControlProtocol::PAUSESTREAM:
          CActiveAEStream *stream;
          stream = *(CActiveAEStream**)msg->data;
          if (!stream->m_paused && m_streams.size() == 1)
          {
            FlushEngine();
            streaming = false;
            m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::STREAMING, &streaming, sizeof(bool));
          }
          stream->m_paused = true;
          return;
        case CActiveAEControlProtocol::RESUMESTREAM:
          stream = *(CActiveAEStream**)msg->data;
          if (stream->m_paused)
            stream->m_syncState = CAESyncInfo::AESyncState::SYNC_START;
          stream->m_paused = false;
          streaming = true;
          m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::STREAMING, &streaming, sizeof(bool));
          m_extTimeout = 0;
          return;
        case CActiveAEControlProtocol::FLUSHSTREAM:
          stream = *(CActiveAEStream**)msg->data;
          SFlushStream(stream);
          msg->Reply(CActiveAEControlProtocol::ACC);
          m_extTimeout = 0;
          return;
        case CActiveAEControlProtocol::STREAMAMP:
          MsgStreamParameter *par;
          par = reinterpret_cast<MsgStreamParameter*>(msg->data);
          par->stream->m_limiter.SetAmplification(par->parameter.float_par);
          par->stream->m_amplify = par->parameter.float_par;
          return;
        case CActiveAEControlProtocol::STREAMVOLUME:
          par = reinterpret_cast<MsgStreamParameter*>(msg->data);
          par->stream->m_volume = par->parameter.float_par;
          return;
        case CActiveAEControlProtocol::STREAMRGAIN:
          par = reinterpret_cast<MsgStreamParameter*>(msg->data);
          par->stream->m_rgain = par->parameter.float_par;
          return;
        case CActiveAEControlProtocol::STREAMRESAMPLERATIO:
          par = reinterpret_cast<MsgStreamParameter*>(msg->data);
          if (par->stream->m_processingBuffers)
          {
            par->stream->m_processingBuffers->SetRR(par->parameter.double_par, m_settings.atempoThreshold);
          }
          return;
        case CActiveAEControlProtocol::STREAMFFMPEGINFO:
          MsgStreamFFmpegInfo *info;
          info = reinterpret_cast<MsgStreamFFmpegInfo*>(msg->data);
          info->stream->m_profile = info->profile;
          info->stream->m_matrixEncoding = info->matrix_encoding;
          info->stream->m_audioServiceType = info->audio_service_type;
          return;
        case CActiveAEControlProtocol::STREAMFADE:
          MsgStreamFade *fade;
          fade = reinterpret_cast<MsgStreamFade*>(msg->data);
          fade->stream->m_fadingBase = fade->from;
          fade->stream->m_fadingTarget = fade->target;
          fade->stream->m_fadingTime = fade->millis;
          fade->stream->m_fadingSamples = -1;
          return;
        case CActiveAEControlProtocol::STOPSOUND:
          CActiveAESound *sound;
          sound = *(CActiveAESound**)msg->data;
          SStopSound(sound);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CActiveAEDataProtocol::PLAYSOUND:
          CActiveAESound *sound;
          sound = *(CActiveAESound**)msg->data;
          if (sound)
          {
            if (m_settings.guisoundmode == AE_SOUND_OFF ||
               (m_settings.guisoundmode == AE_SOUND_IDLE && !m_streams.empty()))
              return;

            SoundState st = {sound, 0};
            m_sounds_playing.push_back(st);
            m_extTimeout = 0;
            m_state = AE_TOP_CONFIGURED_PLAY;
          }
          return;
        case CActiveAEDataProtocol::NEWSTREAM:
          MsgStreamNew *streamMsg;
          CActiveAEStream *stream;
          streamMsg = reinterpret_cast<MsgStreamNew*>(msg->data);
          stream = CreateStream(streamMsg);
          if(stream)
          {
            msg->Reply(CActiveAEDataProtocol::ACC, &stream, sizeof(CActiveAEStream*));
            LoadSettings();
            Configure();
            if (!m_extError)
            {
              m_state = AE_TOP_CONFIGURED_PLAY;
              m_extTimeout = 0;
            }
            else
            {
              m_state = AE_TOP_ERROR;
              m_extTimeout = 500;
            }
          }
          else
            msg->Reply(CActiveAEDataProtocol::ERR);
          return;
        case CActiveAEDataProtocol::STREAMSAMPLE:
          MsgStreamSample *msgData;
          CSampleBuffer *samples;
          msgData = reinterpret_cast<MsgStreamSample*>(msg->data);
          samples = msgData->stream->m_processingSamples.front();
          msgData->stream->m_processingSamples.pop_front();
          if (samples != msgData->buffer)
            CLog::Log(LOGERROR, "CActiveAE - inconsistency in stream sample message");
          if (msgData->buffer->pkt->nb_samples == 0)
            msgData->buffer->Return();
          else
            msgData->stream->m_processingBuffers->m_inputSamples.push_back(msgData->buffer);
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        case CActiveAEDataProtocol::FREESTREAM:
          MsgStreamFree *msgStreamFree;
          msgStreamFree = reinterpret_cast<MsgStreamFree*>(msg->data);
          DiscardStream(msgStreamFree->stream);
          msg->Reply(CActiveAEDataProtocol::ACC);
          if (m_streams.empty())
          {
            if (m_extKeepConfig)
              m_extDrainTimer.Set(m_extKeepConfig);
            else
            {
              AEDelayStatus status;
              m_stats.GetDelay(status);
              if (msgStreamFree->finish)
                m_extDrainTimer.Set(status.GetDelay() * 1000);
              else
                m_extDrainTimer.Set(status.GetDelay() * 1000 + 1000);
            }
            m_extDrain = true;
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        case CActiveAEDataProtocol::DRAINSTREAM:
          stream = *(CActiveAEStream**)msg->data;
          stream->m_drain = true;
          stream->m_processingBuffers->SetDrain(true);
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          msg->Reply(CActiveAEDataProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_sink.m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::RETURNSAMPLE:
          CSampleBuffer **buffer;
          buffer = (CSampleBuffer**)msg->data;
          if (buffer)
          {
            (*buffer)->Return();
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED_SUSPEND:
      if (port == &m_controlPort)
      {
        bool displayReset = false;
        switch (signal)
        {
        case CActiveAEControlProtocol::DISPLAYRESET:
          if (m_extSuspended)
            return;
          CLog::Log(LOGDEBUG,"CActiveAE - display reset event");
          displayReset = true;
        case CActiveAEControlProtocol::INIT:
          m_extError = false;
          m_extSuspended = false;
          if (!displayReset)
          {
            m_controlPort.PurgeOut(CActiveAEControlProtocol::DEVICECHANGE);
            m_controlPort.PurgeOut(CActiveAEControlProtocol::DEVICECOUNTCHANGE);
            m_sink.EnumerateSinkList(true, "");
            LoadSettings();
          }
          Configure();
          if (!displayReset)
            msg->Reply(CActiveAEControlProtocol::ACC);
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_PLAY;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 500;
          }
          m_stats.SetSuspended(false);
          m_extDeferData = false;
          return;
        case CActiveAEControlProtocol::DEVICECHANGE:
        case CActiveAEControlProtocol::DEVICECOUNTCHANGE:
          return;
        default:
          break;
        }
      }
      else if (port == &m_sink.m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::RETURNSAMPLE:
          CSampleBuffer **buffer;
          buffer = (CSampleBuffer**)msg->data;
          if (buffer)
          {
            (*buffer)->Return();
          }
          return;
        default:
          break;
        }
      }
      else if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          m_extTimeout = 1000;
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED_IDLE:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::RESUMESTREAM:
          CActiveAEStream *stream;
          stream = *(CActiveAEStream**)msg->data;
          stream->m_paused = false;
          stream->m_syncState = CAESyncInfo::AESyncState::SYNC_START;
          m_state = AE_TOP_CONFIGURED_PLAY;
          m_extTimeout = 0;
          return;
        case CActiveAEControlProtocol::FLUSHSTREAM:
          stream = *(CActiveAEStream**)msg->data;
          SFlushStream(stream);
          msg->Reply(CActiveAEControlProtocol::ACC);
          m_state = AE_TOP_CONFIGURED_PLAY;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      else if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          ResampleSounds();
          ClearDiscardedBuffers();
          if (m_extDrain)
          {
            if (m_extDrainTimer.IsTimePast())
            {
              Configure();
              if (!m_extError)
              {
                m_state = AE_TOP_CONFIGURED_PLAY;
                m_extTimeout = 0;
              }
              else
              {
                m_state = AE_TOP_ERROR;
                m_extTimeout = 500;
              }
            }
            else
              m_extTimeout = m_extDrainTimer.MillisLeft();
          }
          else
            m_extTimeout = 5000;
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED_PLAY:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          if (m_extError)
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 100;
            return;
          }
          if (RunStages())
          {
            m_extTimeout = 0;
            return;
          }
          if (!m_extDrain && HasWork())
          {
            ClearDiscardedBuffers();
            m_extTimeout = 100;
            return;
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_IDLE;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CActiveAE::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CActiveAE::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;
  XbmcThreads::EndTime timer;

  m_state = AE_TOP_WAIT_PRECOND;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;
  m_extDrain = false;
  m_extDeferData = false;
  m_extKeepConfig = 0;

  // start sink
  m_sink.Start();

  while (!m_bStop)
  {
    gotMsg = false;
    timer.Set(m_extTimeout);

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check sink data port
    else if (m_sink.m_dataPort.ReceiveInMessage(&msg))
    {
      gotMsg = true;
      port = &m_sink.m_dataPort;
    }
    else if (!m_extDeferData)
    {
      // check data port
      if (m_dataPort.ReceiveOutMessage(&msg))
      {
        gotMsg = true;
        port = &m_dataPort;
      }
      // stream data ports
      else
      {
        std::list<CActiveAEStream*>::iterator it;
        for(it=m_streams.begin(); it!=m_streams.end(); ++it)
        {
          if((*it)->m_streamPort->ReceiveOutMessage(&msg))
          {
            gotMsg = true;
            port = &m_dataPort;
            break;
          }
        }
      }
    }

    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      m_extTimeout = timer.MillisLeft();
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = CActiveAEControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
}

AEAudioFormat CActiveAE::GetInputFormat(AEAudioFormat *desiredFmt)
{
  AEAudioFormat inputFormat;

  if (m_streams.empty())
  {
    inputFormat.m_dataFormat    = AE_FMT_FLOAT;
    inputFormat.m_sampleRate    = 44100;
    inputFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
    inputFormat.m_frames        = 0;
    inputFormat.m_frameSize     = 0;
  }
  // force input format after unpausing slave
  else if (desiredFmt != NULL)
  {
    inputFormat = *desiredFmt;
  }
  // keep format when having multiple streams
  else if (m_streams.size() > 1 && m_silenceBuffers == NULL)
  {
    inputFormat = m_inputFormat;
  }
  else
  {
    inputFormat = m_streams.front()->m_format;
    m_inputFormat = inputFormat;
  }

  return inputFormat;
}

void CActiveAE::Configure(AEAudioFormat *desiredFmt)
{
  bool initSink = false;

  AEAudioFormat sinkInputFormat, inputFormat;
  AEAudioFormat oldInternalFormat = m_internalFormat;
  AEAudioFormat oldSinkRequestFormat = m_sinkRequestFormat;

  inputFormat = GetInputFormat(desiredFmt);

  m_sinkRequestFormat = inputFormat;
  ApplySettingsToFormat(m_sinkRequestFormat, m_settings, (int*)&m_mode);
  m_extKeepConfig = 0;

  std::string device = (m_sinkRequestFormat.m_dataFormat == AE_FMT_RAW) ? m_settings.passthroughdevice : m_settings.device;
  std::string driver;
  CAESinkFactory::ParseDevice(device, driver);
  if ((!CompareFormat(m_sinkRequestFormat, m_sinkFormat) && !CompareFormat(m_sinkRequestFormat, oldSinkRequestFormat)) ||
      m_currDevice.compare(device) != 0 ||
      m_settings.driver.compare(driver) != 0)
  {
    FlushEngine();
    if (!InitSink())
      return;
    m_settings.driver = driver;
    m_currDevice = device;
    initSink = true;
    m_stats.Reset(m_sinkFormat.m_sampleRate, m_mode == MODE_PCM);
    m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::VOLUME, &m_volume, sizeof(float));

    if (m_sinkRequestFormat.m_dataFormat != AE_FMT_RAW)
    {
      // limit buffer size in case of sink returns large buffer
      double buffertime = (double)m_sinkFormat.m_frames / m_sinkFormat.m_sampleRate;
      if (buffertime > MAX_BUFFER_TIME)
      {
        CLog::Log(LOGWARNING, "ActiveAE::%s - sink returned large buffer of %d ms, reducing to %d ms", __FUNCTION__, (int)(buffertime * 1000), (int)(MAX_BUFFER_TIME*1000));
        m_sinkFormat.m_frames = MAX_BUFFER_TIME * m_sinkFormat.m_sampleRate;
      }
    }
  }

  if (m_silenceBuffers)
  {
    m_discardBufferPools.push_back(m_silenceBuffers);
    m_silenceBuffers = NULL;
  }

  // buffers for driving gui sounds if no streams are active
  if (m_streams.empty())
  {
    inputFormat = m_sinkFormat;
    if (m_sinkFormat.m_channelLayout.Count() > m_sinkRequestFormat.m_channelLayout.Count())
    {
      inputFormat.m_channelLayout = m_sinkRequestFormat.m_channelLayout;
      inputFormat.m_channelLayout.ResolveChannels(m_sinkFormat.m_channelLayout);
    }
    inputFormat.m_dataFormat = AE_FMT_FLOAT;
    inputFormat.m_frameSize = inputFormat.m_channelLayout.Count() *
                              (CAEUtil::DataFormatToBits(inputFormat.m_dataFormat) >> 3);
    m_silenceBuffers = new CActiveAEBufferPool(inputFormat);
    m_silenceBuffers->Create(MAX_WATER_LEVEL*1000);
    sinkInputFormat = inputFormat;
    m_internalFormat = inputFormat;

    bool streaming = false;
    m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::STREAMING, &streaming, sizeof(bool));

    delete m_encoder;
    m_encoder = NULL;

    if (m_encoderBuffers)
    {
      m_discardBufferPools.push_back(m_encoderBuffers);
      m_encoderBuffers = NULL;
    }
    if (m_vizBuffers)
    {
      m_discardBufferPools.push_back(m_vizBuffers);
      m_vizBuffers = NULL;
    }
    if (m_vizBuffersInput)
    {
      m_discardBufferPools.push_back(m_vizBuffersInput);
      m_vizBuffersInput = NULL;
    }
  }
  // resample buffers for streams
  else
  {
    bool streaming = true;
    m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::STREAMING, &streaming, sizeof(bool));

    AEAudioFormat outputFormat;
    if (m_mode == MODE_RAW)
    {
      inputFormat.m_frames = m_sinkFormat.m_frames;
      outputFormat = inputFormat;
      sinkInputFormat = m_sinkFormat;
    }
    // transcode everything with more than 2 channels
    else if (m_mode == MODE_TRANSCODE)
    {
      outputFormat = inputFormat;
      outputFormat.m_dataFormat = AE_FMT_FLOATP;
      outputFormat.m_sampleRate = 48000;

      // setup encoder
      if (!m_encoder)
      {
        m_encoder = new CAEEncoderFFmpeg();
        m_encoder->Initialize(outputFormat, true);
        m_encoderFormat = outputFormat;
      }
      else
        outputFormat = m_encoderFormat;

      outputFormat.m_channelLayout = m_encoderFormat.m_channelLayout;
      outputFormat.m_frames = m_encoderFormat.m_frames;

      // encoder buffer
      if (m_encoder->GetCodecID() == AV_CODEC_ID_AC3)
      {
        AEAudioFormat format;
        format.m_channelLayout += AE_CH_FC;
        format.m_dataFormat = AE_FMT_RAW;
        format.m_sampleRate = 48000;
        format.m_channelLayout = AE_CH_LAYOUT_2_0;
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
        format.m_streamInfo.m_channels = 2;
        format.m_streamInfo.m_sampleRate = 48000;
        format.m_streamInfo.m_ac3FrameSize = m_encoderFormat.m_frames;
        //! @todo implement
        if (m_encoderBuffers && initSink)
        {
          m_discardBufferPools.push_back(m_encoderBuffers);
          m_encoderBuffers = NULL;
        }
        if (!m_encoderBuffers)
        {
          m_encoderBuffers = new CActiveAEBufferPool(format);
          m_encoderBuffers->Create(MAX_WATER_LEVEL*1000);
        }
      }

      sinkInputFormat = m_sinkFormat;
    }
    else
    {
      outputFormat = m_sinkFormat;
      outputFormat.m_dataFormat = AE_IS_PLANAR(outputFormat.m_dataFormat) ? AE_FMT_FLOATP : AE_FMT_FLOAT;
      outputFormat.m_frameSize = outputFormat.m_channelLayout.Count() *
                                 (CAEUtil::DataFormatToBits(outputFormat.m_dataFormat) >> 3);

      // due to channel ordering of the driver, a sink may return more channels than
      // requested, i.e. 2.1 request returns FL,FR,BL,BR,FC,LFE for ALSA
      // in this case we need to downmix to requested format
      if (m_sinkFormat.m_channelLayout.Count() > m_sinkRequestFormat.m_channelLayout.Count())
      {
        outputFormat.m_channelLayout = m_sinkRequestFormat.m_channelLayout;
        outputFormat.m_channelLayout.ResolveChannels(m_sinkFormat.m_channelLayout);
      }

      // internally we use ffmpeg layouts, means that layout won't change in resample
      // stage. preserve correct layout for sink stage where remapping is done
      uint64_t avlayout = CAEUtil::GetAVChannelLayout(outputFormat.m_channelLayout);
      outputFormat.m_channelLayout = CAEUtil::GetAEChannelLayout(avlayout);

      //! @todo adjust to decoder
      sinkInputFormat = outputFormat;
    }
    m_internalFormat = outputFormat;

    std::list<CActiveAEStream*>::iterator it;
    for(it=m_streams.begin(); it!=m_streams.end(); ++it)
    {
      if (!(*it)->m_inputBuffers)
      {
        // align input buffers with period of sink or encoder
        (*it)->m_format.m_frames = m_internalFormat.m_frames * ((float)(*it)->m_format.m_sampleRate / m_internalFormat.m_sampleRate);

        // create buffer pool
        (*it)->m_inputBuffers = new CActiveAEBufferPool((*it)->m_format);
        (*it)->m_inputBuffers->Create(MAX_CACHE_LEVEL*1000);
        (*it)->m_streamSpace = (*it)->m_format.m_frameSize * (*it)->m_format.m_frames;

        // if input format does not follow ffmpeg channel mask, we may need to remap channels
        (*it)->InitRemapper();
      }
      if (initSink && (*it)->m_processingBuffers)
      {
        (*it)->m_processingBuffers->Flush();
        m_discardBufferPools.push_back((*it)->m_processingBuffers->GetResampleBuffers());
        m_discardBufferPools.push_back((*it)->m_processingBuffers->GetAtempoBuffers());
        delete (*it)->m_processingBuffers;
        (*it)->m_processingBuffers = nullptr;
      }
      if (!(*it)->m_processingBuffers)
      {
        (*it)->m_processingBuffers = new CActiveAEStreamBuffers((*it)->m_inputBuffers->m_format, outputFormat, m_settings.resampleQuality);
        (*it)->m_processingBuffers->ForceResampler((*it)->m_forceResampler);

        (*it)->m_processingBuffers->Create(MAX_CACHE_LEVEL*1000, false, m_settings.stereoupmix, m_settings.normalizelevels);
      }
      if (m_mode == MODE_TRANSCODE || m_streams.size() > 1)
        (*it)->m_processingBuffers->FillBuffer();

      // amplification
      (*it)->m_limiter.SetSamplerate(outputFormat.m_sampleRate);
    }

    // update buffered time of streams
    m_stats.AddSamples(0, m_streams);

    // buffers for viz
    if (!(inputFormat.m_dataFormat == AE_FMT_RAW))
    {
      if (initSink && m_vizBuffers)
      {
        m_discardBufferPools.push_back(m_vizBuffers);
        m_vizBuffers = NULL;
        m_discardBufferPools.push_back(m_vizBuffersInput);
        m_vizBuffersInput = NULL;
      }
      if (!m_vizBuffers && !m_audioCallback.empty())
      {
        AEAudioFormat vizFormat = m_internalFormat;
        vizFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
        vizFormat.m_dataFormat = AE_FMT_FLOAT;
        vizFormat.m_sampleRate = 44100;
        vizFormat.m_frames =
            m_internalFormat.m_frames *
            (static_cast<float>(vizFormat.m_sampleRate) / m_internalFormat.m_sampleRate);

        // input buffers
        m_vizBuffersInput = new CActiveAEBufferPool(m_internalFormat);
        m_vizBuffersInput->Create(2000 + m_stats.GetMaxDelay() * 1000);

        // resample buffers
        m_vizBuffers = new CActiveAEBufferPoolResample(m_internalFormat, vizFormat, m_settings.resampleQuality);
        //! @todo use cache of sync + water level
        m_vizBuffers->Create(2000 + m_stats.GetMaxDelay() * 1000, false, false);
        m_vizInitialized = false;
      }
    }

    // buffers need to sync
    m_silenceBuffers = new CActiveAEBufferPool(outputFormat);
    m_silenceBuffers->Create(500);
  }

  // resample buffers for sink
  if (m_sinkBuffers &&
     (!CompareFormat(m_sinkBuffers->m_format,m_sinkFormat) ||
      !CompareFormat(m_sinkBuffers->m_inputFormat, sinkInputFormat) ||
      m_sinkBuffers->m_format.m_frames != m_sinkFormat.m_frames))
  {
    m_discardBufferPools.push_back(m_sinkBuffers);
    m_sinkBuffers = NULL;
  }
  if (!m_sinkBuffers)
  {
    m_sinkBuffers = new CActiveAEBufferPoolResample(sinkInputFormat, m_sinkFormat, m_settings.resampleQuality);
    m_sinkBuffers->Create(MAX_WATER_LEVEL*1000, true, false);
  }

  // reset gui sounds
  if (!CompareFormat(oldInternalFormat, m_internalFormat))
  {
    if (m_settings.guisoundmode == AE_SOUND_ALWAYS ||
       (m_settings.guisoundmode == AE_SOUND_IDLE && m_streams.empty()) ||
       m_aeGUISoundForce)
    {
      std::vector<CActiveAESound*>::iterator it;
      for (it = m_sounds.begin(); it != m_sounds.end(); ++it)
      {
        (*it)->SetConverted(false);
      }
    }
    m_sounds_playing.clear();
  }

  ClearDiscardedBuffers();
  m_extDrain = false;
}

CActiveAEStream* CActiveAE::CreateStream(MsgStreamNew *streamMsg)
{
  // we only can handle a single pass through stream
  bool hasRawStream = false;
  bool hasStream = false;
  std::list<CActiveAEStream*>::iterator it;
  for(it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    if ((*it)->IsDrained())
      continue;
    if ((*it)->m_format.m_dataFormat == AE_FMT_RAW)
      hasRawStream = true;
    hasStream = true;
  }
  if (hasRawStream || (hasStream && (streamMsg->format.m_dataFormat == AE_FMT_RAW)))
  {
    return NULL;
  }

  // create the stream
  CActiveAEStream *stream;
  stream = new CActiveAEStream(&streamMsg->format, m_streamIdGen++, this);
  stream->m_streamPort = new CActiveAEDataProtocol("stream",
                             &stream->m_inMsgEvent, &m_outMsgEvent);

  // create buffer pool
  stream->m_inputBuffers = NULL; // create in Configure when we know the sink format
  stream->m_processingBuffers = NULL; // create in Configure when we know the sink format
  stream->m_fadingSamples = 0;
  stream->m_started = false;
  stream->m_resampleMode = 0;
  stream->m_syncState = CAESyncInfo::AESyncState::SYNC_OFF;

  if (streamMsg->options & AESTREAM_PAUSED)
  {
    stream->m_paused = true;
    stream->m_streamIsBuffering = true;
  }

  if (streamMsg->options & AESTREAM_FORCE_RESAMPLE)
    stream->m_forceResampler = true;

  stream->m_pClock = streamMsg->clock;

  m_streams.push_back(stream);
  m_stats.AddStream(stream->m_id);

  return stream;
}

void CActiveAE::DiscardStream(CActiveAEStream *stream)
{
  std::list<CActiveAEStream*>::iterator it;
  for (it=m_streams.begin(); it!=m_streams.end(); )
  {
    if (stream == (*it))
    {
      while (!(*it)->m_processingSamples.empty())
      {
        (*it)->m_processingSamples.front()->Return();
        (*it)->m_processingSamples.pop_front();
      }
      if ((*it)->m_inputBuffers)
        m_discardBufferPools.push_back((*it)->m_inputBuffers);
      if ((*it)->m_processingBuffers)
      {
        (*it)->m_processingBuffers->Flush();
        m_discardBufferPools.push_back((*it)->m_processingBuffers->GetResampleBuffers());
        m_discardBufferPools.push_back((*it)->m_processingBuffers->GetAtempoBuffers());
      }
      delete (*it)->m_processingBuffers;
      CLog::Log(LOGDEBUG, "CActiveAE::DiscardStream - audio stream deleted");
      m_stats.RemoveStream((*it)->m_id);
      delete (*it)->m_streamPort;
      delete (*it);
      it = m_streams.erase(it);
    }
    else
      ++it;
  }

  ClearDiscardedBuffers();
}

void CActiveAE::SFlushStream(CActiveAEStream *stream)
{
  while (!stream->m_processingSamples.empty())
  {
    stream->m_processingSamples.front()->Return();
    stream->m_processingSamples.pop_front();
  }
  stream->m_processingBuffers->Flush();
  stream->m_streamPort->Purge();
  stream->m_bufferedTime = 0.0;
  stream->m_paused = false;
  stream->m_syncState = CAESyncInfo::AESyncState::SYNC_START;
  stream->m_syncError.Flush();
  stream->ResetFreeBuffers();

  // flush the engine if we only have a single stream
  if (m_streams.size() == 1)
  {
    FlushEngine();
  }

  m_stats.UpdateStream(stream);
}

void CActiveAE::FlushEngine()
{
  if (m_sinkBuffers)
    m_sinkBuffers->Flush();
  if (m_vizBuffers)
    m_vizBuffers->Flush();

  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::FLUSH,
                                           &reply, 2000))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC;
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error on flush", __FUNCTION__);
      m_extError = true;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to flush", __FUNCTION__);
    m_extError = true;
  }
  m_stats.Reset(m_sinkFormat.m_sampleRate, m_mode == MODE_PCM);
}

void CActiveAE::ClearDiscardedBuffers()
{
  auto it = m_discardBufferPools.begin();
  while (it != m_discardBufferPools.end())
  {
    CActiveAEBufferPoolResample *rbuf = dynamic_cast<CActiveAEBufferPoolResample*>(*it);
    if (rbuf)
    {
      rbuf->Flush();
    }
    // if all buffers have returned, we can delete the buffer pool
    if ((*it)->m_allSamples.size() == (*it)->m_freeSamples.size())
    {
      delete (*it);
      CLog::Log(LOGDEBUG, "CActiveAE::ClearDiscardedBuffers - buffer pool deleted");
      it = m_discardBufferPools.erase(it);
    }
    else
      ++it;
  }
}

void CActiveAE::SStopSound(CActiveAESound *sound)
{
  std::list<SoundState>::iterator it;
  for (it=m_sounds_playing.begin(); it!=m_sounds_playing.end(); ++it)
  {
    if (it->sound == sound)
    {
      if (sound->GetChannel() != AE_CH_NULL)
        m_aeGUISoundForce = false;
      m_sounds_playing.erase(it);
      return;
    }
  }
}

void CActiveAE::DiscardSound(CActiveAESound *sound)
{
  SStopSound(sound);

  std::vector<CActiveAESound*>::iterator it;
  for (it=m_sounds.begin(); it!=m_sounds.end(); ++it)
  {
    if ((*it) == sound)
    {
      m_sounds.erase(it);
      delete sound;
      return;
    }
  }
}

void CActiveAE::ChangeResamplers()
{
  std::list<CActiveAEStream*>::iterator it;
  for(it=m_streams.begin(); it!=m_streams.end(); ++it)
  {
    (*it)->m_processingBuffers->ConfigureResampler(m_settings.normalizelevels, m_settings.stereoupmix, m_settings.resampleQuality);
  }
}

void CActiveAE::ApplySettingsToFormat(AEAudioFormat &format, AudioSettings &settings, int *mode)
{
  int oldMode = m_mode;
  if (mode)
    *mode = MODE_PCM;

  // raw pass through
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    if (mode)
      *mode = MODE_RAW;
  }
  // transcode
  else if (settings.channels <= AE_CH_LAYOUT_2_0 && // no multichannel pcm
           settings.passthrough &&
           settings.ac3passthrough &&
           settings.ac3transcode &&
           !m_streams.empty() &&
           (format.m_channelLayout.Count() > 2 || settings.stereoupmix))
  {
    format.m_dataFormat = AE_FMT_RAW;
    format.m_sampleRate = 48000;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
    format.m_streamInfo.m_channels = 2;
    format.m_streamInfo.m_sampleRate = 48000;
    if (mode)
      *mode = MODE_TRANSCODE;
  }
  else
  {
    format.m_dataFormat = AE_IS_PLANAR(format.m_dataFormat) ? AE_FMT_FLOATP : AE_FMT_FLOAT;
    // consider user channel layout for those cases
    // 1. input stream is multichannel
    // 2. stereo upmix is selected
    // 3. fixed mode
    if ((format.m_channelLayout.Count() > 2) ||
         settings.stereoupmix ||
         (settings.config == AE_CONFIG_FIXED))
    {
      CAEChannelInfo stdLayout;
      switch (settings.channels)
      {
        default:
        case  0: stdLayout = AE_CH_LAYOUT_2_0; break;
        case  1: stdLayout = AE_CH_LAYOUT_2_0; break;
        case  2: stdLayout = AE_CH_LAYOUT_2_1; break;
        case  3: stdLayout = AE_CH_LAYOUT_3_0; break;
        case  4: stdLayout = AE_CH_LAYOUT_3_1; break;
        case  5: stdLayout = AE_CH_LAYOUT_4_0; break;
        case  6: stdLayout = AE_CH_LAYOUT_4_1; break;
        case  7: stdLayout = AE_CH_LAYOUT_5_0; break;
        case  8: stdLayout = AE_CH_LAYOUT_5_1; break;
        case  9: stdLayout = AE_CH_LAYOUT_7_0; break;
        case 10: stdLayout = AE_CH_LAYOUT_7_1; break;
      }

      if (m_settings.config == AE_CONFIG_FIXED || (settings.stereoupmix && format.m_channelLayout.Count() <= 2))
        format.m_channelLayout = stdLayout;
      else if (m_extKeepConfig && (settings.config == AE_CONFIG_AUTO) && (oldMode != MODE_RAW))
        format.m_channelLayout = m_internalFormat.m_channelLayout;
      else
      {
        if (stdLayout == AE_CH_LAYOUT_5_0 || stdLayout == AE_CH_LAYOUT_5_1)
        {
          std::vector<CAEChannelInfo> alts;
          alts.push_back(stdLayout);
          stdLayout.ReplaceChannel(AE_CH_BL, AE_CH_SL);
          stdLayout.ReplaceChannel(AE_CH_BR, AE_CH_SR);
          alts.push_back(stdLayout);
          int bestMatch = format.m_channelLayout.BestMatch(alts);
          stdLayout = alts[bestMatch];
        }
        format.m_channelLayout.ResolveChannels(stdLayout);
      }
    }
    // don't change from multi to stereo in AUTO mode
    else if ((settings.config == AE_CONFIG_AUTO) &&
              m_stats.GetWaterLevel() > 0 && m_internalFormat.m_channelLayout.Count() > 2)
    {
      format.m_channelLayout = m_internalFormat.m_channelLayout;
    }

    if (m_sink.GetDeviceType(m_settings.device) == AE_DEVTYPE_IEC958)
    {
      if (format.m_sampleRate > m_settings.samplerate)
      {
        format.m_sampleRate = m_settings.samplerate;
        CLog::Log(LOGINFO, "CActiveAE::ApplySettings - limit samplerate for SPDIF to %d", format.m_sampleRate);
      }
      format.m_channelLayout = AE_CH_LAYOUT_2_0;
    }

    if (m_settings.config == AE_CONFIG_FIXED)
    {
      format.m_sampleRate = m_settings.samplerate;
      format.m_dataFormat = AE_FMT_FLOAT;
      CLog::Log(LOGINFO, "CActiveAE::ApplySettings - Forcing samplerate to %d", format.m_sampleRate);
    }

    // sinks may not support mono
    if (format.m_channelLayout.Count() == 1)
    {
      format.m_channelLayout = AE_CH_LAYOUT_2_0;
    }
  }
}

bool CActiveAE::NeedReconfigureBuffers()
{
  AEAudioFormat newFormat = GetInputFormat();
  ApplySettingsToFormat(newFormat, m_settings);

  return newFormat.m_dataFormat != m_sinkRequestFormat.m_dataFormat ||
      newFormat.m_channelLayout != m_sinkRequestFormat.m_channelLayout ||
      newFormat.m_sampleRate != m_sinkRequestFormat.m_sampleRate;
}

bool CActiveAE::NeedReconfigureSink()
{
  AEAudioFormat newFormat = GetInputFormat();
  ApplySettingsToFormat(newFormat, m_settings);

  std::string device = (newFormat.m_dataFormat == AE_FMT_RAW) ? m_settings.passthroughdevice : m_settings.device;
  std::string driver;
  CAESinkFactory::ParseDevice(device, driver);

  return !CompareFormat(newFormat, m_sinkFormat) ||
      m_currDevice.compare(device) != 0 ||
      m_settings.driver.compare(driver) != 0;
}

bool CActiveAE::InitSink()
{
  SinkConfig config;
  config.format = m_sinkRequestFormat;
  config.stats = &m_stats;
  config.device = (m_sinkRequestFormat.m_dataFormat == AE_FMT_RAW) ? &m_settings.passthroughdevice :
                                                                     &m_settings.device;

  // send message to sink
  m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SETNOISETYPE, &m_settings.streamNoise, sizeof(bool));
  m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SETSILENCETIMEOUT, &m_settings.silenceTimeout, sizeof(int));

  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::CONFIGURE,
                                                 &reply,
                                                 5000,
                                                 &config, sizeof(config)))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      m_extError = true;
      return false;
    }
    SinkReply *data;
    data = reinterpret_cast<SinkReply*>(reply->data);
    if (data)
    {
      m_sinkFormat = data->format;
      m_sinkHasVolume = data->hasVolume;
      m_stats.SetSinkCacheTotal(data->cacheTotal);
      m_stats.SetSinkLatency(data->latency);
      m_stats.SetCurrentSinkFormat(m_sinkFormat);
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
    m_stats.SetSinkCacheTotal(0);
    m_stats.SetSinkLatency(0);
    AEAudioFormat invalidFormat;
    invalidFormat.m_dataFormat = AE_FMT_INVALID;
    m_stats.SetCurrentSinkFormat(invalidFormat);
    m_extError = true;
    return false;
  }

  m_inMsgEvent.Reset();
  return true;
}

void CActiveAE::DrainSink()
{
  // send message to sink
  Message *reply;
  if (m_sink.m_dataPort.SendOutMessageSync(CSinkDataProtocol::DRAIN,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == CSinkDataProtocol::ACC;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error on drain", __FUNCTION__);
      m_extError = true;
      return;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to drain", __FUNCTION__);
    m_extError = true;
    return;
  }
}

void CActiveAE::UnconfigureSink()
{
  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::UNCONFIGURE,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC;
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      m_extError = true;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to unconfigure", __FUNCTION__);
    m_extError = true;
  }

  // make sure we open sink on next configure
  m_currDevice = "";

  m_inMsgEvent.Reset();
}


bool CActiveAE::RunStages()
{
  bool busy = false;

  // serve input streams
  std::list<CActiveAEStream*>::iterator it;
  for (it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    if ((*it)->m_processingBuffers && !(*it)->m_paused)
      busy = (*it)->m_processingBuffers->ProcessBuffers();

    if ((*it)->m_streamIsBuffering &&
        (*it)->m_processingBuffers &&
        ((*it)->m_processingBuffers->HasInputLevel(50)))
    {
      CSingleLock lock((*it)->m_streamLock);
      (*it)->m_streamIsBuffering = false;
    }

    // provide buffers to stream
    float time = m_stats.GetCacheTime((*it));
    CSampleBuffer *buffer;
    if (!(*it)->m_drain)
    {
      float buftime = (float)(*it)->m_inputBuffers->m_format.m_frames / (*it)->m_inputBuffers->m_format.m_sampleRate;
      if ((*it)->m_inputBuffers->m_format.m_dataFormat == AE_FMT_RAW)
        buftime = (*it)->m_inputBuffers->m_format.m_streamInfo.GetDuration() / 1000;
      while ((time < MAX_CACHE_LEVEL || (*it)->m_streamIsBuffering) && !(*it)->m_inputBuffers->m_freeSamples.empty())
      {
        buffer = (*it)->m_inputBuffers->GetFreeBuffer();
        (*it)->m_processingSamples.push_back(buffer);
        (*it)->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMBUFFER, &buffer, sizeof(CSampleBuffer*));
        (*it)->IncFreeBuffers();
        time += buftime;
      }
    }
    else
    {
      if ((*it)->m_processingBuffers->IsDrained() &&
          (*it)->m_processingSamples.empty())
      {
        (*it)->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMDRAINED);
        (*it)->m_drain = false;
        (*it)->m_processingBuffers->SetDrain(false);
        (*it)->m_started = false;

        // set variables being polled via stream interface
        CSingleLock lock((*it)->m_streamLock);
        if ((*it)->m_streamSlave)
        {
          CActiveAEStream *slave = (CActiveAEStream*)((*it)->m_streamSlave);
          slave->m_paused = false;

          //! @todo find better solution for this gapless bites audiophile
          if (m_settings.config == AE_CONFIG_MATCH)
            Configure(&slave->m_format);

          (*it)->m_streamSlave = NULL;
        }
        (*it)->m_streamDrained = true;
        (*it)->m_streamDraining = false;
        (*it)->m_streamFading = false;
      }
    }
  }

  if (m_stats.GetWaterLevel() < MAX_WATER_LEVEL &&
     (m_mode != MODE_TRANSCODE || (m_encoderBuffers && !m_encoderBuffers->m_freeSamples.empty())))
  {
    // calculate sync error
    for (it = m_streams.begin(); it != m_streams.end(); ++it)
    {
      if ((*it)->m_paused || !(*it)->m_started || !(*it)->m_processingBuffers || !(*it)->m_pClock)
        continue;

      if ((*it)->m_processingBuffers->m_outputSamples.empty())
        continue;

      CSampleBuffer *buf = (*it)->m_processingBuffers->m_outputSamples.front();
      if (buf->timestamp)
      {
        AEDelayStatus status;
        m_stats.GetDelay(status);
        double pts = buf->timestamp - (buf->pkt_start_offset * 1000 / buf->pkt->config.sample_rate);
        double delay = status.GetDelay() * 1000;
        double playingPts = pts - delay;
        double maxError = ((*it)->m_syncState == CAESyncInfo::SYNC_INSYNC) ? 1000 : 5000;
        double error = playingPts - (*it)->m_pClock->GetClock();
        if (error > maxError)
        {
          CLog::Log(LOGWARNING, "ActiveAE - large audio sync error: %f", error);
          error = maxError;
        }
        else if (error < -maxError)
        {
          CLog::Log(LOGWARNING, "ActiveAE - large audio sync error: %f", error);
          error = -maxError;
        }
        (*it)->m_syncError.Add(error);
      }
    }

    // mix streams and sounds sounds
    if (m_mode != MODE_RAW)
    {
      CSampleBuffer *out = NULL;
      if (!m_sounds_playing.empty() && m_streams.empty())
      {
        if (m_silenceBuffers && !m_silenceBuffers->m_freeSamples.empty())
        {
          out = m_silenceBuffers->GetFreeBuffer();
          for (int i=0; i<out->pkt->planes; i++)
          {
            memset(out->pkt->data[i], 0, out->pkt->linesize);
          }
          out->pkt->nb_samples = out->pkt->max_nb_samples;
        }
      }

      // mix streams
      std::list<CActiveAEStream*>::iterator it;

      // if we deal with more than a single stream, all streams
      // must provide samples for mixing
      bool allStreamsReady = true;
      for (it = m_streams.begin(); it != m_streams.end(); ++it)
      {
        if ((*it)->m_paused || !(*it)->m_started || !(*it)->m_processingBuffers)
          continue;

        if ((*it)->m_processingBuffers->m_outputSamples.empty())
          allStreamsReady = false;
      }

      bool needClamp = false;
      for (it = m_streams.begin(); it != m_streams.end() && allStreamsReady; ++it)
      {
        if ((*it)->m_paused || !(*it)->m_processingBuffers)
          continue;

        if (!(*it)->m_processingBuffers->m_outputSamples.empty())
        {
          CSampleBuffer *tmp = SyncStream(*it);
          m_stats.UpdateStream(*it);
          if (tmp)
          {
            if (!out)
              out = tmp;
            continue;
          }

          (*it)->m_started = true;

          if (!out)
          {
            out = (*it)->m_processingBuffers->m_outputSamples.front();
            (*it)->m_processingBuffers->m_outputSamples.pop_front();

            int nb_floats = out->pkt->nb_samples * out->pkt->config.channels / out->pkt->planes;
            int nb_loops = 1;
            float fadingStep = 0.0f;

            // fading
            if ((*it)->m_fadingSamples == -1)
            {
              (*it)->m_fadingSamples = m_internalFormat.m_sampleRate * (float)(*it)->m_fadingTime / 1000.0f;
              if ((*it)->m_fadingSamples > 0)
                (*it)->m_volume = (*it)->m_fadingBase;
              else
              {
                (*it)->m_volume = (*it)->m_fadingTarget;
                CSingleLock lock((*it)->m_streamLock);
                (*it)->m_streamFading = false;
              }
            }
            if ((*it)->m_fadingSamples > 0)
            {
              nb_floats = out->pkt->config.channels / out->pkt->planes;
              nb_loops = out->pkt->nb_samples;
              float delta = (*it)->m_fadingTarget - (*it)->m_fadingBase;
              int samples = m_internalFormat.m_sampleRate * (float)(*it)->m_fadingTime / 1000.0f;
              fadingStep = delta / samples;
            }

            // for stream amplification,
            // turned off downmix normalization,
            // or if sink format is float (in order to prevent from clipping)
            // we need to run on a per sample basis
            if ((*it)->m_amplify != 1.0 || !(*it)->m_processingBuffers->DoesNormalize() || (m_sinkFormat.m_dataFormat == AE_FMT_FLOAT))
            {
              nb_floats = out->pkt->config.channels / out->pkt->planes;
              nb_loops = out->pkt->nb_samples;
            }

            for(int i=0; i<nb_loops; i++)
            {
              if ((*it)->m_fadingSamples > 0)
              {
                (*it)->m_volume += fadingStep;
                (*it)->m_fadingSamples--;

                if ((*it)->m_fadingSamples == 0)
                {
                  // set variables being polled via stream interface
                  CSingleLock lock((*it)->m_streamLock);
                  (*it)->m_streamFading = false;
                }
              }

              // volume for stream
              float volume = (*it)->m_volume * (*it)->m_rgain;
              if(nb_loops > 1)
                volume *= (*it)->m_limiter.Run((float**)out->pkt->data, out->pkt->config.channels, i*nb_floats, out->pkt->planes > 1);

              for(int j=0; j<out->pkt->planes; j++)
              {
#if defined(HAVE_SSE) && defined(__SSE__)
                CAEUtil::SSEMulArray((float*)out->pkt->data[j]+i*nb_floats, volume, nb_floats);
#else
                float* fbuffer = (float*) out->pkt->data[j]+i*nb_floats;
                for (int k = 0; k < nb_floats; ++k)
                {
                  fbuffer[k] *= volume;
                }
#endif
              }
            }
          }
          else
          {
            CSampleBuffer *mix = NULL;
            mix = (*it)->m_processingBuffers->m_outputSamples.front();
            (*it)->m_processingBuffers->m_outputSamples.pop_front();

            int nb_floats = mix->pkt->nb_samples * mix->pkt->config.channels / mix->pkt->planes;
            int nb_loops = 1;
            float fadingStep = 0.0f;

            // fading
            if ((*it)->m_fadingSamples == -1)
            {
              (*it)->m_fadingSamples = m_internalFormat.m_sampleRate * (float)(*it)->m_fadingTime / 1000.0f;
              (*it)->m_volume = (*it)->m_fadingBase;
            }
            if ((*it)->m_fadingSamples > 0)
            {
              nb_floats = mix->pkt->config.channels / mix->pkt->planes;
              nb_loops = mix->pkt->nb_samples;
              float delta = (*it)->m_fadingTarget - (*it)->m_fadingBase;
              int samples = m_internalFormat.m_sampleRate * (float)(*it)->m_fadingTime / 1000.0f;
              fadingStep = delta / samples;
            }

            // for streams amplification of turned off downmix normalization
            // we need to run on a per sample basis
            if ((*it)->m_amplify != 1.0 || !(*it)->m_processingBuffers->DoesNormalize())
            {
              nb_floats = out->pkt->config.channels / out->pkt->planes;
              nb_loops = out->pkt->nb_samples;
            }

            for(int i=0; i<nb_loops; i++)
            {
              if ((*it)->m_fadingSamples > 0)
              {
                (*it)->m_volume += fadingStep;
                (*it)->m_fadingSamples--;

                if ((*it)->m_fadingSamples == 0)
                {
                  // set variables being polled via stream interface
                  CSingleLock lock((*it)->m_streamLock);
                  (*it)->m_streamFading = false;
                }
              }

              // volume for stream
              float volume = (*it)->m_volume * (*it)->m_rgain;
              if(nb_loops > 1)
                volume *= (*it)->m_limiter.Run((float**)mix->pkt->data, mix->pkt->config.channels, i*nb_floats, mix->pkt->planes > 1);

              for(int j=0; j<out->pkt->planes && j<mix->pkt->planes; j++)
              {
                float *dst = (float*)out->pkt->data[j]+i*nb_floats;
                float *src = (float*)mix->pkt->data[j]+i*nb_floats;
#if defined(HAVE_SSE) && defined(__SSE__)
                CAEUtil::SSEMulAddArray(dst, src, volume, nb_floats);
                for (int k = 0; k < nb_floats; ++k)
                {
                  if (fabs(dst[k]) > 1.0f)
                  {
                    needClamp = true;
                    break;
                  }
                }
#else
                for (int k = 0; k < nb_floats; ++k)
                {
                  dst[k] += src[k] * volume;
                  if (fabs(dst[k]) > 1.0f)
                    needClamp = true;
                }
#endif
              }
            }
            mix->Return();
          }
          busy = true;
        }
      }// for

      // finally clamp samples
      if (out && needClamp)
      {
        int nb_floats = out->pkt->nb_samples * out->pkt->config.channels / out->pkt->planes;
        for (int i=0; i<out->pkt->planes; i++)
        {
          CAEUtil::ClampArray((float*)out->pkt->data[i], nb_floats);
        }
      }

      // process output buffer, gui sounds, encode, viz
      if (out)
      {
        // viz
        {
          CSingleLock lock(m_vizLock);
          if (!m_audioCallback.empty() && !m_streams.empty())
          {
            if (!m_vizInitialized || !m_vizBuffers)
            {
              Configure();
              for (auto& it : m_audioCallback)
                it->OnInitialize(2, m_vizBuffers->m_format.m_sampleRate, 32);
              m_vizInitialized = true;
            }

            if (!m_vizBuffersInput->m_freeSamples.empty())
            {
              // copy the samples into the viz input buffer
              CSampleBuffer *viz = m_vizBuffersInput->GetFreeBuffer();
              int samples = out->pkt->nb_samples;
              int bytes = samples * out->pkt->config.channels / out->pkt->planes * out->pkt->bytes_per_sample;
              for(int i= 0; i < out->pkt->planes; i++)
              {
                memcpy(viz->pkt->data[i], out->pkt->data[i], bytes);
              }
              viz->pkt->nb_samples = samples;
              m_vizBuffers->m_inputSamples.push_back(viz);
            }
            else
              CLog::Log(LOGWARNING,"ActiveAE::%s - viz ran out of free buffers", __FUNCTION__);
            AEDelayStatus status;
            m_stats.GetDelay(status);
            int64_t now = XbmcThreads::SystemClockMillis();
            int64_t timestamp = now + status.GetDelay() * 1000;
            busy |= m_vizBuffers->ResampleBuffers(timestamp);
            while(!m_vizBuffers->m_outputSamples.empty())
            {
              CSampleBuffer *buf = m_vizBuffers->m_outputSamples.front();
              if ((now - buf->timestamp) < 0)
                break;
              else
              {
                unsigned int samples = static_cast<unsigned int>(buf->pkt->nb_samples) *
                                       buf->pkt->config.channels / buf->pkt->planes;
                for (auto& it : m_audioCallback)
                  it->OnAudioData((float*)(buf->pkt->data[0]), samples);
                buf->Return();
                m_vizBuffers->m_outputSamples.pop_front();
              }
            }
          }
          else if (m_vizBuffers)
            m_vizBuffers->Flush();
        }

        // mix gui sounds
        MixSounds(*(out->pkt));
        if (!m_sinkHasVolume || m_muted)
          Deamplify(*(out->pkt));

        if (m_mode == MODE_TRANSCODE && m_encoder)
        {
          CSampleBuffer *buf = nullptr;
          if (out->pkt->nb_samples)
          {
            buf = m_encoderBuffers->GetFreeBuffer();
            buf->pkt->nb_samples = m_encoder->Encode(out->pkt->data[0], out->pkt->planes*out->pkt->linesize,
                                                     buf->pkt->data[0], buf->pkt->planes*buf->pkt->linesize);

            // set pts of last sample
            buf->pkt_start_offset = buf->pkt->nb_samples;
            buf->timestamp = out->timestamp;
          }

          out->Return();
          out = buf;
        }
        busy = true;
      }

      // update stats
      if(out)
      {
        int samples = (m_mode == MODE_TRANSCODE) ? 1 : out->pkt->nb_samples;
        m_stats.AddSamples(samples, m_streams);
        m_sinkBuffers->m_inputSamples.push_back(out);
      }
    }
    // pass through
    else
    {
      std::list<CActiveAEStream*>::iterator it;
      CSampleBuffer *buffer;
      for (it = m_streams.begin(); it != m_streams.end(); ++it)
      {
        if (!(*it)->m_processingBuffers->m_outputSamples.empty() && !(*it)->m_paused)
        {
          (*it)->m_started = true;
          buffer = SyncStream(*it);
          m_stats.UpdateStream(*it);
          if (!buffer)
          {
            buffer = (*it)->m_processingBuffers->m_outputSamples.front();
            (*it)->m_processingBuffers->m_outputSamples.pop_front();
          }
          m_stats.AddSamples(1, m_streams);
          m_sinkBuffers->m_inputSamples.push_back(buffer);
        }
      }
    }
  }

  // serve sink buffers
  busy |= m_sinkBuffers->ResampleBuffers();
  while(!m_sinkBuffers->m_outputSamples.empty())
  {
    CSampleBuffer *out = NULL;
    out = m_sinkBuffers->m_outputSamples.front();
    m_sinkBuffers->m_outputSamples.pop_front();
    m_sink.m_dataPort.SendOutMessage(CSinkDataProtocol::SAMPLE,
        &out, sizeof(CSampleBuffer*));
    busy = true;
  }

  return busy;
}

bool CActiveAE::HasWork()
{
  if (!m_sounds_playing.empty())
    return true;
  if (!m_sinkBuffers->m_inputSamples.empty())
    return true;
  if (!m_sinkBuffers->m_outputSamples.empty())
    return true;

  std::list<CActiveAEStream*>::iterator it;
  for (it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    if (!(*it)->m_processingBuffers->HasWork())
      return true;
    if (!(*it)->m_processingSamples.empty())
      return true;
  }

  return false;
}

CSampleBuffer* CActiveAE::SyncStream(CActiveAEStream *stream)
{
  CSampleBuffer *ret = NULL;

  if (!stream->m_pClock)
    return ret;

  if (stream->m_syncState == CAESyncInfo::AESyncState::SYNC_START)
  {
    stream->m_syncState = CAESyncInfo::AESyncState::SYNC_MUTE;
    stream->m_syncError.Flush(100);
    stream->m_processingBuffers->SetRR(1.0, m_settings.atempoThreshold);
    stream->m_resampleIntegral = 0;
    CLog::Log(LOGDEBUG,"ActiveAE - start sync of audio stream");
  }

  double error;
  double threshold = 100;
  if (stream->m_resampleMode)
  {
    threshold *= 2;
    if (stream->m_pClock)
    {
      double clockspeed = stream->m_pClock->GetClockSpeed();
      if (clockspeed >= 1.05 || clockspeed <= 0.95)
        threshold *= 5;
    }
  }

  int timeout = (stream->m_syncState != CAESyncInfo::AESyncState::SYNC_INSYNC) ? 100 : stream->GetErrorInterval();
  bool newerror = stream->m_syncError.Get(error, timeout);

  if (newerror && fabs(error) > threshold && stream->m_syncState == CAESyncInfo::AESyncState::SYNC_INSYNC)
  {
    stream->m_syncState = CAESyncInfo::AESyncState::SYNC_ADJUST;
    stream->m_processingBuffers->SetRR(1.0, m_settings.atempoThreshold);
    stream->m_resampleIntegral = 0;
    stream->m_lastSyncError = error;
    CLog::Log(LOGDEBUG,"ActiveAE::SyncStream - average error %f above threshold of %f", error, threshold);
  }
  else if (newerror && stream->m_syncState == CAESyncInfo::AESyncState::SYNC_MUTE)
  {
    stream->m_syncState = CAESyncInfo::AESyncState::SYNC_ADJUST;
    stream->m_lastSyncError = error;
    CLog::Log(LOGDEBUG,"ActiveAE::SyncStream - average error of %f, start adjusting", error);
  }

  if (stream->m_syncState == CAESyncInfo::AESyncState::SYNC_MUTE)
  {
    CSampleBuffer *buf = stream->m_processingBuffers->m_outputSamples.front();
    if (m_mode == MODE_RAW)
    {
      buf->pkt->nb_samples = 0;
      buf->pkt->pause_burst_ms = stream->m_processingBuffers->m_inputFormat.m_streamInfo.GetDuration();
    }
    else
    {
      for(int i=0; i<buf->pkt->planes; i++)
      {
        memset(buf->pkt->data[i], 0, buf->pkt->linesize);
      }
    }
  }
  else if (stream->m_syncState == CAESyncInfo::AESyncState::SYNC_ADJUST)
  {
    if (error > 0)
    {
      ret = m_silenceBuffers->GetFreeBuffer();
      if (ret)
      {
        ret->pkt->nb_samples = 0;
        ret->pkt->pause_burst_ms = 0;
        int framesToDelay = error / 1000 * ret->pkt->config.sample_rate;
        if (framesToDelay > ret->pkt->max_nb_samples)
          framesToDelay = ret->pkt->max_nb_samples;
        if (m_mode == MODE_TRANSCODE)
        {
          if (framesToDelay > (int) (m_encoderFormat.m_frames / 2))
            framesToDelay = m_encoderFormat.m_frames;
          else
            framesToDelay = 0;
        }
        ret->pkt->nb_samples = framesToDelay;
        if (m_mode == MODE_RAW)
        {
          ret->pkt->nb_samples = 0;
          ret->pkt->pause_burst_ms = error;
          if (error > stream->m_format.m_streamInfo.GetDuration())
            ret->pkt->pause_burst_ms = stream->m_format.m_streamInfo.GetDuration();

          stream->m_syncError.Correction(-ret->pkt->pause_burst_ms);
          error -= ret->pkt->pause_burst_ms;
        }
        else
        {
          stream->m_syncError.Correction(-framesToDelay*1000/ret->pkt->config.sample_rate);
          error -= framesToDelay*1000/ret->pkt->config.sample_rate;
          for(int i=0; i<ret->pkt->planes; i++)
          {
            memset(ret->pkt->data[i], 0, ret->pkt->linesize);
          }
        }

        if ((ret->pkt->nb_samples == 0) && (ret->pkt->pause_burst_ms == 0))
        {
          ret->Return();
          ret = nullptr;
        }
      }
    }
    else
    {
      CSampleBuffer *buf = stream->m_processingBuffers->m_outputSamples.front();
      int framesToSkip = -error / 1000 * buf->pkt->config.sample_rate;
      if (framesToSkip > buf->pkt->nb_samples)
        framesToSkip = buf->pkt->nb_samples;
      if (m_mode == MODE_TRANSCODE)
      {
        if (framesToSkip > (int) (m_encoderFormat.m_frames / 2))
          framesToSkip = buf->pkt->nb_samples;
        else
          framesToSkip = 0;
      }
      if (m_mode == MODE_RAW)
      {
        if (-error > stream->m_format.m_streamInfo.GetDuration() / 2)
        {
          stream->m_syncError.Correction(stream->m_format.m_streamInfo.GetDuration());
          error += stream->m_format.m_streamInfo.GetDuration();
          buf->pkt->nb_samples = 0;
        }
      }
      else
      {
        int bytesToSkip = framesToSkip * buf->pkt->bytes_per_sample *
                                  buf->pkt->config.channels / buf->pkt->planes;
        for (int i=0; i<buf->pkt->planes; i++)
        {
          memmove(buf->pkt->data[i], buf->pkt->data[i]+bytesToSkip, buf->pkt->linesize - bytesToSkip);
        }
        buf->pkt->nb_samples -= framesToSkip;
        stream->m_syncError.Correction((double)framesToSkip * 1000 / buf->pkt->config.sample_rate);
        error += (double)framesToSkip * 1000 / buf->pkt->config.sample_rate;
      }
    }

    if (fabs(error) < 30)
    {
      if (stream->m_lastSyncError > threshold * 2)
      {
        stream->m_syncState = CAESyncInfo::AESyncState::SYNC_MUTE;
        stream->m_syncError.Flush(100);
        CLog::Log(LOGDEBUG,"ActiveAE::SyncStream - average error %f, last average error: %f", error, stream->m_lastSyncError);
        stream->m_lastSyncError = error;
      }
      else
      {
        stream->m_syncState = CAESyncInfo::AESyncState::SYNC_INSYNC;
        stream->m_syncError.Flush(1000);
        stream->m_resampleIntegral = 0;
        stream->m_processingBuffers->SetRR(1.0, m_settings.atempoThreshold);
        CLog::Log(LOGDEBUG,"ActiveAE::SyncStream - average error %f below threshold of %f", error, 30.0);
      }
    }

    return ret;
  }

  if (!newerror || stream->m_syncState != CAESyncInfo::AESyncState::SYNC_INSYNC)
    return ret;

  if (stream->m_resampleMode)
  {
    if (stream->m_processingBuffers)
    {
      stream->m_processingBuffers->SetRR(stream->CalcResampleRatio(error), m_settings.atempoThreshold);
    }
  }
  else if (stream->m_processingBuffers)
  {
    stream->m_processingBuffers->SetRR(1.0, m_settings.atempoThreshold);
  }

  stream->m_syncError.SetErrorInterval(stream->GetErrorInterval());

  return ret;
}

void CActiveAE::MixSounds(CSoundPacket &dstSample)
{
  if (m_sounds_playing.empty())
    return;

  float volume;
  float *out;
  float *sample_buffer;
  int max_samples = dstSample.nb_samples;

  std::list<SoundState>::iterator it;
  for (it = m_sounds_playing.begin(); it != m_sounds_playing.end(); )
  {
    if (!it->sound->IsConverted())
      ResampleSound(it->sound);
    int available_samples = it->sound->GetSound(false)->nb_samples - it->samples_played;
    int mix_samples = std::min(max_samples, available_samples);
    int start = it->samples_played *
                av_get_bytes_per_sample(it->sound->GetSound(false)->config.fmt) *
                it->sound->GetSound(false)->config.channels /
                it->sound->GetSound(false)->planes;

    for(int j=0; j<dstSample.planes; j++)
    {
      volume = it->sound->GetVolume();
      out = (float*)dstSample.data[j];
      sample_buffer = (float*)(it->sound->GetSound(false)->data[j]+start);
      int nb_floats = mix_samples * dstSample.config.channels / dstSample.planes;
#if defined(HAVE_SSE) && defined(__SSE__)
      CAEUtil::SSEMulAddArray(out, sample_buffer, volume, nb_floats);
#else
      for (int k = 0; k < nb_floats; ++k)
        *out++ += *sample_buffer++ * volume;
#endif
    }

    it->samples_played += mix_samples;

    // no more frames, so remove it from the list
    if (it->samples_played >= it->sound->GetSound(false)->nb_samples)
    {
      it = m_sounds_playing.erase(it);
      continue;
    }
    ++it;
  }
}

void CActiveAE::Deamplify(CSoundPacket &dstSample)
{
  if (m_volumeScaled < 1.0 || m_muted)
  {
    int nb_floats = dstSample.nb_samples * dstSample.config.channels / dstSample.planes;
    float volume = m_muted ? 0.0f : m_volumeScaled;

    for(int j=0; j<dstSample.planes; j++)
    {
      float* buffer = reinterpret_cast<float*>(dstSample.data[j]);
#if defined(HAVE_SSE) && defined(__SSE__)
      CAEUtil::SSEMulArray(buffer, volume, nb_floats);
#else
      float *fbuffer = buffer;
      for (int i = 0; i < nb_floats; i++)
        *fbuffer++ *= volume;
#endif
    }
  }
}

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------

void CActiveAE::LoadSettings()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_settings.device = settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
  m_settings.passthroughdevice = settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE);

  m_settings.config = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG);
  m_settings.channels = (m_sink.GetDeviceType(m_settings.device) == AE_DEVTYPE_IEC958) ? AE_CH_LAYOUT_2_0 : settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);
  m_settings.samplerate = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE);

  m_settings.stereoupmix = IsSettingVisible(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX) ? settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX) : false;
  m_settings.normalizelevels = !settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME);
  m_settings.guisoundmode = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE);

  m_settings.passthrough = m_settings.config == AE_CONFIG_FIXED ? false : settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
  if (!m_sink.HasPassthroughDevice())
    m_settings.passthrough = false;
  m_settings.ac3passthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH);
  m_settings.ac3transcode = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE);
  m_settings.eac3passthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH);
  m_settings.truehdpassthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH);
  m_settings.dtspassthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH);
  m_settings.dtshdpassthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH);

  m_settings.resampleQuality = static_cast<AEQuality>(settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY));
  m_settings.atempoThreshold = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD) / 100.0;
  m_settings.streamNoise = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE);
  m_settings.silenceTimeout = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE) * 60000;
}

void CActiveAE::Start()
{
  Create();
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::INIT,
                                                 &reply,
                                                 10000))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
  }

  m_inMsgEvent.Reset();
}

void CActiveAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  m_sink.EnumerateOutputDevices(devices, passthrough);
}

void CActiveAE::OnSettingsChange()
{
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::RECONFIGURE);
}

bool CActiveAE::SupportsRaw(AEAudioFormat &format)
{
  // check if passthrough is enabled
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH))
    return false;

  // fixed config disabled passthrough
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) == AE_CONFIG_FIXED)
    return false;

  // check if the format is enabled in settings
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_AC3 && !m_settings.ac3passthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_512 && !m_settings.dtspassthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_1024 && !m_settings.dtspassthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_2048 && !m_settings.dtspassthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_CORE && !m_settings.dtspassthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3 && !m_settings.eac3passthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD && !m_settings.truehdpassthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD && !m_settings.dtshdpassthrough)
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_MA && !m_settings.dtshdpassthrough)
    return false;

  if (!m_sink.SupportsFormat(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE), format))
    return false;

  return true;
}

bool CActiveAE::SupportsSilenceTimeout()
{
  return true;
}

bool CActiveAE::HasStereoAudioChannelCount()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string device = settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
  int numChannels = (m_sink.GetDeviceType(device) == AE_DEVTYPE_IEC958) ? AE_CH_LAYOUT_2_0 : settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);
  bool passthrough = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) == AE_CONFIG_FIXED ? false : settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
  return numChannels == AE_CH_LAYOUT_2_0 && !passthrough;
}

bool CActiveAE::HasHDAudioChannelCount()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string device = settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
  int numChannels = (m_sink.GetDeviceType(device) == AE_DEVTYPE_IEC958) ? AE_CH_LAYOUT_2_0 : settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);
  return numChannels > AE_CH_LAYOUT_5_1;
}

bool CActiveAE::SupportsQualityLevel(enum AEQuality level)
{
  if (level == AE_QUALITY_LOW || level == AE_QUALITY_MID || level == AE_QUALITY_HIGH)
    return true;

  return false;
}

bool CActiveAE::IsSettingVisible(const std::string &settingId)
{
  if (settingId == CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE)
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (m_sink.GetDeviceType(settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE)) == AE_DEVTYPE_IEC958)
      return true;
    if (settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) == AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_CHANNELS)
  {
    if (m_sink.GetDeviceType(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE)) != AE_DEVTYPE_IEC958)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH)
  {
    if (m_sink.HasPassthroughDevice() && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) != AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH)
  {
    AEAudioFormat format;
    format.m_dataFormat = AE_FMT_RAW;
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTS_512;
    format.m_sampleRate = 48000;
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (m_sink.SupportsFormat(settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE), format) &&
        settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) != AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH)
  {
    AEAudioFormat format;
    format.m_dataFormat = AE_FMT_RAW;
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
    format.m_streamInfo.m_sampleRate = 192000;
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (m_sink.SupportsFormat(settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE), format) &&
        settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) != AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH)
  {
    AEAudioFormat format;
    format.m_dataFormat = AE_FMT_RAW;
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD;
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (m_sink.SupportsFormat(settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE), format) &&
        settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) != AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH)
  {
    AEAudioFormat format;
    format.m_dataFormat = AE_FMT_RAW;
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (m_sink.SupportsFormat(settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE), format) &&
        settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) != AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX)
  {
    if (m_sink.HasPassthroughDevice() ||
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOOUTPUT_CHANNELS) > AE_CH_LAYOUT_2_0)
    return true;
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE)
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (m_sink.HasPassthroughDevice() &&
        settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH) &&
        settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) != AE_CONFIG_FIXED &&
        (settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_CHANNELS) <= AE_CH_LAYOUT_2_0 || m_sink.GetDeviceType(settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE)) == AE_DEVTYPE_IEC958))
      return true;
  }
  return false;
}

void CActiveAE::Shutdown()
{
  Dispose();
}

bool CActiveAE::Suspend()
{
  return m_controlPort.SendOutMessage(CActiveAEControlProtocol::SUSPEND);
}

bool CActiveAE::Resume()
{
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::INIT,
                                                 &reply,
                                                 5000))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
    return false;
  }

  m_inMsgEvent.Reset();
  return true;
}

bool CActiveAE::IsSuspended()
{
  return m_stats.IsSuspended();
}

float CActiveAE::GetVolume()
{
  return m_aeVolume;
}

void CActiveAE::SetVolume(const float volume)
{
  m_aeVolume = std::max( 0.0f, std::min(1.0f, volume));
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::VOLUME, &m_aeVolume, sizeof(float));
}

void CActiveAE::SetMute(const bool enabled)
{
  m_aeMuted = enabled;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::MUTE, &m_aeMuted, sizeof(bool));
}

bool CActiveAE::IsMuted()
{
  return m_aeMuted;
}

void CActiveAE::KeepConfiguration(unsigned int millis)
{
  unsigned int timeMs = millis;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::KEEPCONFIG, &timeMs, sizeof(unsigned int));
}

void CActiveAE::DeviceChange()
{
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::DEVICECHANGE);
}

void CActiveAE::DeviceCountChange(const std::string& driver)
{
  const char* name = driver.c_str();
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::DEVICECOUNTCHANGE, name,
                               driver.length() + 1);
}

bool CActiveAE::GetCurrentSinkFormat(AEAudioFormat &SinkFormat)
{
  SinkFormat = m_stats.GetCurrentSinkFormat();
  return true;
}

void CActiveAE::OnLostDisplay()
{
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::DISPLAYLOST,
                                                 &reply,
                                                 5000))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - timed out", __FUNCTION__);
  }
}

void CActiveAE::OnResetDisplay()
{
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::DISPLAYRESET);
}

void CActiveAE::OnAppFocusChange(bool focus)
{
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::APPFOCUSED, &focus, sizeof(focus));
}

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------

uint8_t **CActiveAE::AllocSoundSample(SampleConfig &config, int &samples, int &bytes_per_sample, int &planes, int &linesize)
{
  uint8_t **buffer;
  planes = av_sample_fmt_is_planar(config.fmt) ? config.channels : 1;
  buffer = new uint8_t*[planes];

  // align buffer to 16 in order to be compatible with sse in CAEConvert
  av_samples_alloc(buffer, &linesize, config.channels,
                                 samples, config.fmt, 16);
  bytes_per_sample = av_get_bytes_per_sample(config.fmt);
  return buffer;
}

void CActiveAE::FreeSoundSample(uint8_t **data)
{
  av_freep(data);
  delete [] data;
}

bool CActiveAE::CompareFormat(AEAudioFormat &lhs, AEAudioFormat &rhs)
{
  if (lhs.m_channelLayout != rhs.m_channelLayout ||
      lhs.m_dataFormat != rhs.m_dataFormat ||
      lhs.m_sampleRate != rhs.m_sampleRate)
    return false;
  else if (lhs.m_dataFormat == AE_FMT_RAW && rhs.m_dataFormat == AE_FMT_RAW &&
           lhs.m_streamInfo.m_type != rhs.m_streamInfo.m_type)
    return false;
  else
    return true;
}

//-----------------------------------------------------------------------------
// GUI Sounds
//-----------------------------------------------------------------------------

/**
 * load sound from an audio file and store original format
 * register the sound in ActiveAE
 * later when the engine is idle it will convert the sound to sink format
 */

IAESound *CActiveAE::MakeSound(const std::string& file)
{
  AVFormatContext *fmt_ctx = nullptr;
  AVCodecContext *dec_ctx = nullptr;
  AVIOContext *io_ctx;
  AVInputFormat *io_fmt = nullptr;
  AVCodec *dec = nullptr;
  CActiveAESound *sound = nullptr;
  SampleConfig config;

  sound = new CActiveAESound(file, this);
  if (!sound->Prepare())
  {
    delete sound;
    return nullptr;
  }
  int fileSize = sound->GetFileSize();

  int bufferSize = 4096;
  int blockSize = sound->GetChunkSize();
  if (blockSize > 1)
    bufferSize = blockSize;

  fmt_ctx = avformat_alloc_context();
  unsigned char* buffer = (unsigned char*)av_malloc(bufferSize);
  io_ctx = avio_alloc_context(buffer, bufferSize, 0,
                              sound, CActiveAESound::Read, NULL, CActiveAESound::Seek);

  io_ctx->max_packet_size = bufferSize;

  if (!sound->IsSeekPossible())
  {
    io_ctx->seekable = 0;
    io_ctx->max_packet_size = 0;
  }

  fmt_ctx->pb = io_ctx;

  av_probe_input_buffer(io_ctx, &io_fmt, file.c_str(), nullptr, 0, 0);
  if (!io_fmt)
  {
    avformat_close_input(&fmt_ctx);
    if (io_ctx)
    {
      av_freep(&io_ctx->buffer);
      av_freep(&io_ctx);
    }
    delete sound;
    return nullptr;
  }

  // find decoder
  if (avformat_open_input(&fmt_ctx, file.c_str(), nullptr, nullptr) == 0)
  {
    fmt_ctx->flags |= AVFMT_FLAG_NOPARSE;
    if (avformat_find_stream_info(fmt_ctx, nullptr) >= 0)
    {
      AVCodecID codecId = fmt_ctx->streams[0]->codecpar->codec_id;
      dec = avcodec_find_decoder(codecId);
      config.sample_rate = fmt_ctx->streams[0]->codecpar->sample_rate;
      config.channels = fmt_ctx->streams[0]->codecpar->channels;
      config.channel_layout = fmt_ctx->streams[0]->codecpar->channel_layout;
    }
  }
  if (dec == nullptr)
  {
    avformat_close_input(&fmt_ctx);
    if (io_ctx)
    {
      av_freep(&io_ctx->buffer);
      av_freep(&io_ctx);
    }
    delete sound;
    return nullptr;
  }

  dec_ctx = avcodec_alloc_context3(dec);
  dec_ctx->sample_rate = config.sample_rate;
  dec_ctx->channels = config.channels;
  if (!config.channel_layout)
    config.channel_layout = av_get_default_channel_layout(config.channels);
  dec_ctx->channel_layout = config.channel_layout;

  AVPacket avpkt;
  AVFrame *decoded_frame = NULL;
  decoded_frame = av_frame_alloc();
  bool error = false;

  if (avcodec_open2(dec_ctx, dec, nullptr) >= 0)
  {
    bool init = false;

    // decode until eof
    av_init_packet(&avpkt);
    int ret;
    while (av_read_frame(fmt_ctx, &avpkt) >= 0 && !error)
    {
      ret = avcodec_send_packet(dec_ctx, &avpkt);
      if (ret < 0)
      {
        error = true;
        break;
      }

      while ((ret = avcodec_receive_frame(dec_ctx, decoded_frame)) == 0)
      {
        if (!init)
        {
          int samples = fileSize / av_get_bytes_per_sample(dec_ctx->sample_fmt) / config.channels;
          config.fmt = dec_ctx->sample_fmt;
          config.bits_per_sample = dec_ctx->bits_per_coded_sample;
          sound->InitSound(true, config, samples);
          init = true;
        }
        sound->StoreSound(true, decoded_frame->extended_data,
                          decoded_frame->nb_samples, decoded_frame->linesize[0]);
      }
      av_packet_unref(&avpkt);

      if (ret < 0 && ret != AVERROR(EAGAIN))
      {
        error = true;
        break;
      }
    }
    ret = avcodec_send_packet(dec_ctx, nullptr);
    while ((ret = avcodec_receive_frame(dec_ctx, decoded_frame)) != AVERROR_EOF)
    {
      if (ret == 0)
      {
        sound->StoreSound(true, decoded_frame->extended_data,
                          decoded_frame->nb_samples, decoded_frame->linesize[0]);
      }
      else
      {
        error = true;
        break;
      }
    }
  }

  av_frame_free(&decoded_frame);
  avcodec_free_context(&dec_ctx);
  avformat_close_input(&fmt_ctx);
  if (io_ctx)
  {
    av_freep(&io_ctx->buffer);
    av_freep(&io_ctx);
  }

  if (error)
  {
    delete sound;
    return nullptr;
  }

  sound->Finish();

  // register sound
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::NEWSOUND, &sound, sizeof(CActiveAESound*));

  return sound;
}

void CActiveAE::FreeSound(IAESound *sound)
{
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::FREESOUND, &sound, sizeof(CActiveAESound*));
}

void CActiveAE::PlaySound(CActiveAESound *sound)
{
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::PLAYSOUND, &sound, sizeof(CActiveAESound*));
}

void CActiveAE::StopSound(CActiveAESound *sound)
{
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STOPSOUND, &sound, sizeof(CActiveAESound*));
}

/**
 * resample sounds to destination format for mixing
 * destination format is either format of stream or
 * default sink format when no stream is playing
 */
void CActiveAE::ResampleSounds()
{
  if ((m_settings.guisoundmode == AE_SOUND_OFF ||
      (m_settings.guisoundmode == AE_SOUND_IDLE && !m_streams.empty())) &&
      !m_aeGUISoundForce)
    return;

  std::vector<CActiveAESound*>::iterator it;
  for (it = m_sounds.begin(); it != m_sounds.end(); ++it)
  {
    if (!(*it)->IsConverted())
    {
      ResampleSound(*it);
      // only do one sound, then yield to main loop
      break;
    }
  }
}

bool CActiveAE::ResampleSound(CActiveAESound *sound)
{
  SampleConfig orig_config, dst_config;
  uint8_t **dst_buffer;
  int dst_samples;

  if (m_mode == MODE_RAW || m_internalFormat.m_dataFormat == AE_FMT_INVALID)
    return false;

  if (!sound->GetSound(true))
    return false;

  orig_config = sound->GetSound(true)->config;

  dst_config.channel_layout = CAEUtil::GetAVChannelLayout(m_internalFormat.m_channelLayout);
  dst_config.channels = m_internalFormat.m_channelLayout.Count();
  dst_config.sample_rate = m_internalFormat.m_sampleRate;
  dst_config.fmt = CAEUtil::GetAVSampleFormat(m_internalFormat.m_dataFormat);
  dst_config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_internalFormat.m_dataFormat);
  dst_config.dither_bits = CAEUtil::DataFormatToDitherBits(m_internalFormat.m_dataFormat);

  AEChannel testChannel = sound->GetChannel();
  CAEChannelInfo outChannels;
  if (sound->GetSound(true)->config.channels == 1 && testChannel != AE_CH_NULL)
  {
    for (unsigned int out=0; out < m_internalFormat.m_channelLayout.Count(); out++)
    {
      if (m_internalFormat.m_channelLayout[out] == AE_CH_FC && testChannel != AE_CH_FC) /// To become center clear on position test ??????
        outChannels += AE_CH_FL;
      else if (m_internalFormat.m_channelLayout[out] == testChannel)
        outChannels += AE_CH_FC;
      else
        outChannels += m_internalFormat.m_channelLayout[out];
    }
  }

  IAEResample *resampler = CAEResampleFactory::Create(AERESAMPLEFACTORY_QUICK_RESAMPLE);

  resampler->Init(dst_config, orig_config,
                  false,
                  true,
                  M_SQRT1_2,
                  outChannels.Count() > 0 ? &outChannels : nullptr,
                  m_settings.resampleQuality,
                  false);

  dst_samples = resampler->CalcDstSampleCount(sound->GetSound(true)->nb_samples,
                                              m_internalFormat.m_sampleRate,
                                              orig_config.sample_rate);

  dst_buffer = sound->InitSound(false, dst_config, dst_samples);
  if (!dst_buffer)
  {
    delete resampler;
    return false;
  }
  int samples = resampler->Resample(dst_buffer, dst_samples,
                                    sound->GetSound(true)->data,
                                    sound->GetSound(true)->nb_samples,
                                    1.0);

  sound->GetSound(false)->nb_samples = samples;

  delete resampler;
  sound->SetConverted(true);
  return true;
}

//-----------------------------------------------------------------------------
// Streams
//-----------------------------------------------------------------------------

IAEStream *CActiveAE::MakeStream(AEAudioFormat &audioFormat, unsigned int options, IAEClockCallback *clock)
{
  if (audioFormat.m_dataFormat <= AE_FMT_INVALID || audioFormat.m_dataFormat >= AE_FMT_MAX)
  {
    return nullptr;
  }

  if (IsSuspended())
    return NULL;

  //! @todo pass number of samples in audio packet

  AEAudioFormat format = audioFormat;
  format.m_frames = format.m_sampleRate / 10;

  if (format.m_dataFormat != AE_FMT_RAW)
  {
    format.m_frameSize = format.m_channelLayout.Count() *
                         (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  }
  else
    format.m_frameSize = 1;

  MsgStreamNew msg;
  msg.format = format;
  msg.options = options;
  msg.clock = clock;

  Message *reply;
  if (m_dataPort.SendOutMessageSync(CActiveAEDataProtocol::NEWSTREAM,
                                    &reply,10000,
                                    &msg, sizeof(MsgStreamNew)))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC;
    if (success)
    {
      CActiveAEStream *stream = *(CActiveAEStream**)reply->data;
      reply->Release();
      return stream;
    }
    reply->Release();
  }

  CLog::Log(LOGERROR, "ActiveAE::%s - could not create stream", __FUNCTION__);
  return NULL;
}

bool CActiveAE::FreeStream(IAEStream *stream, bool finish)
{
  MsgStreamFree msg;
  msg.stream = static_cast<CActiveAEStream*>(stream);
  msg.finish = finish;

  Message *reply;
  if (m_dataPort.SendOutMessageSync(CActiveAEDataProtocol::FREESTREAM,
                                    &reply,1000,
                                    &msg, sizeof(MsgStreamFree)))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC;
    reply->Release();
    if (success)
    {
      return true;
    }
  }
  CLog::Log(LOGERROR, "CActiveAE::FreeStream - failed");
  return false;
}

void CActiveAE::FlushStream(CActiveAEStream *stream)
{
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::FLUSHSTREAM,
                                       &reply,1000,
                                       &stream, sizeof(CActiveAEStream*)))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "CActiveAE::FlushStream - failed");
    }
  }
}

void CActiveAE::PauseStream(CActiveAEStream *stream, bool pause)
{
  //! @todo pause sink, needs api change
  if (pause)
    m_controlPort.SendOutMessage(CActiveAEControlProtocol::PAUSESTREAM,
                                   &stream, sizeof(CActiveAEStream*));
  else
    m_controlPort.SendOutMessage(CActiveAEControlProtocol::RESUMESTREAM,
                                   &stream, sizeof(CActiveAEStream*));
}

void CActiveAE::SetStreamAmplification(CActiveAEStream *stream, float amplify)
{
  MsgStreamParameter msg;
  msg.stream = stream;
  msg.parameter.float_par = amplify;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMAMP,
                                     &msg, sizeof(MsgStreamParameter));
}

void CActiveAE::SetStreamReplaygain(CActiveAEStream *stream, float rgain)
{
  MsgStreamParameter msg;
  msg.stream = stream;
  msg.parameter.float_par = rgain;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMRGAIN,
                                     &msg, sizeof(MsgStreamParameter));
}

void CActiveAE::SetStreamVolume(CActiveAEStream *stream, float volume)
{
  MsgStreamParameter msg;
  msg.stream = stream;
  msg.parameter.float_par = volume;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMVOLUME,
                                     &msg, sizeof(MsgStreamParameter));
}

void CActiveAE::SetStreamResampleRatio(CActiveAEStream *stream, double ratio)
{
  MsgStreamParameter msg;
  msg.stream = stream;
  msg.parameter.double_par = ratio;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMRESAMPLERATIO,
                                     &msg, sizeof(MsgStreamParameter));
}

void CActiveAE::SetStreamResampleMode(CActiveAEStream *stream, int mode)
{
  MsgStreamParameter msg;
  msg.stream = stream;
  msg.parameter.int_par = mode;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMRESAMPLEMODE,
                               &msg, sizeof(MsgStreamParameter));
}

void CActiveAE::SetStreamFFmpegInfo(CActiveAEStream *stream, int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type)
{
  MsgStreamFFmpegInfo msg;
  msg.stream = stream;
  msg.profile = profile;
  msg.matrix_encoding = matrix_encoding;
  msg.audio_service_type = audio_service_type;

  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMFFMPEGINFO, &msg, sizeof(MsgStreamFFmpegInfo));
}

void CActiveAE::SetStreamFade(CActiveAEStream *stream, float from, float target, unsigned int millis)
{
  MsgStreamFade msg;
  msg.stream = stream;
  msg.from = from;
  msg.target = target;
  msg.millis = millis;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::STREAMFADE,
                                     &msg, sizeof(MsgStreamFade));
}

void CActiveAE::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock(m_vizLock);
  m_audioCallback.push_back(pCallback);
  m_vizInitialized = false;
}

void CActiveAE::UnregisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock(m_vizLock);
  auto it = std::find(m_audioCallback.begin(), m_audioCallback.end(), pCallback);
  if (it != m_audioCallback.end())
    m_audioCallback.erase(it);
}
