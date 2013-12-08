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

#include "ActiveAE.h"

using namespace ActiveAE;
#include "ActiveAESound.h"
#include "ActiveAEStream.h"
#include "Utils/AEUtil.h"
#include "Encoders/AEEncoderFFmpeg.h"

#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "windowing/WindowingFactory.h"

#define MAX_CACHE_LEVEL 0.5   // total cache time of stream in seconds
#define MAX_WATER_LEVEL 0.25  // buffered time after stream stages in seconds

void CEngineStats::Reset(unsigned int sampleRate)
{
  CSingleLock lock(m_lock);
  m_sinkUpdate = XbmcThreads::SystemClockMillis();
  m_sinkDelay = 0;
  m_sinkSampleRate = sampleRate;
  m_bufferedSamples = 0;
  m_suspended = false;
}

void CEngineStats::UpdateSinkDelay(double delay, int samples)
{
  CSingleLock lock(m_lock);
  m_sinkUpdate = XbmcThreads::SystemClockMillis();
  m_sinkDelay = delay;
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

  //update buffered time of streams
  std::list<CActiveAEStream*>::iterator it;
  for(it=streams.begin(); it!=streams.end(); ++it)
  {
    float delay = 0;
    std::deque<CSampleBuffer*>::iterator itBuf;
    for(itBuf=(*it)->m_processingSamples.begin(); itBuf!=(*it)->m_processingSamples.end(); ++itBuf)
    {
      delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
    }
    delay += (*it)->m_resampleBuffers->GetDelay();
    (*it)->m_bufferedTime = delay;
  }
}

float CEngineStats::GetDelay()
{
  CSingleLock lock(m_lock);
  unsigned int now = XbmcThreads::SystemClockMillis();
  float delay = m_sinkDelay - (double)(now-m_sinkUpdate) / 1000;
  delay += (float)m_bufferedSamples / m_sinkSampleRate;

  if (delay < 0)
    delay = 0.0;

  return delay;
}

float CEngineStats::GetDelay(CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  unsigned int now = XbmcThreads::SystemClockMillis();
  float delay = m_sinkDelay - (double)(now-m_sinkUpdate) / 1000;
  delay += (float)m_bufferedSamples / m_sinkSampleRate;

  if (delay < 0)
    delay = 0.0;

  delay += stream->m_bufferedTime;
  return delay;
}

float CEngineStats::GetCacheTime(CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  float delay = (float)m_bufferedSamples / m_sinkSampleRate;

  delay += stream->m_bufferedTime;
  return delay;
}

float CEngineStats::GetCacheTotal(CActiveAEStream *stream)
{
  return MAX_CACHE_LEVEL + m_sinkCacheTotal;
}

float CEngineStats::GetWaterLevel()
{
  return (float)m_bufferedSamples / m_sinkSampleRate;
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
  m_aeVolume = 1.0;
  m_muted = false;
  m_aeMuted = false;
  m_mode = MODE_PCM;
  m_encoder = NULL;
  m_audioCallback = NULL;
  m_vizInitialized = false;
  m_sinkHasVolume = false;
}

CActiveAE::~CActiveAE()
{
  Dispose();
}

void CActiveAE::Dispose()
{
#if defined(HAS_GLX) || defined(TARGET_DARWIN_OSX)
  g_Windowing.Unregister(this);
#endif

  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();
  m_sink.Dispose();

  m_dllAvFormat.Unload();
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
}

//-----------------------------------------------------------------------------
// Behavior
//-----------------------------------------------------------------------------

enum AE_STATES
{
  AE_TOP = 0,                      // 0
  AE_TOP_ERROR,                    // 1
  AE_TOP_UNCONFIGURED,             // 2
  AE_TOP_RECONFIGURING,            // 3
  AE_TOP_CONFIGURED,               // 4
  AE_TOP_CONFIGURED_SUSPEND,       // 5
  AE_TOP_CONFIGURED_IDLE,          // 6
  AE_TOP_CONFIGURED_PLAY,          // 7
};

int AE_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    0, //TOP_RECONFIGURING
    4, //TOP_CONFIGURED_SUSPEND
    4, //TOP_CONFIGURED_IDLE
    4, //TOP_CONFIGURED_PLAY
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
        case CActiveAEControlProtocol::SOUNDMODE:
          m_soundMode = *(int*)msg->data;
          return;
        case CActiveAEControlProtocol::VOLUME:
          m_volume = *(float*)msg->data;
          if (m_sinkHasVolume)
            m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::VOLUME, &m_volume, sizeof(float));
          return;
        case CActiveAEControlProtocol::MUTE:
          m_muted = *(bool*)msg->data;
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
          CActiveAEStream *stream;
          stream = *(CActiveAEStream**)msg->data;
          DiscardStream(stream);
          return;
        case CActiveAEDataProtocol::FREESOUND:
          sound = *(CActiveAESound**)msg->data;
          DiscardSound(sound);
          return;
        case CActiveAEDataProtocol::DRAINSTREAM:
          stream = *(CActiveAEStream**)msg->data;
          stream->m_drain = true;
          stream->m_resampleBuffers->m_drain = true;
          msg->Reply(CActiveAEDataProtocol::ACC);
          stream->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMDRAINED);
          return;
        default:
          break;
        }
      }
      else if (&m_sink.m_dataPort)
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

    case AE_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::INIT:
          m_extError = false;
          m_sink.EnumerateSinkList(false);
          LoadSettings();
          Configure();
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
        switch (signal)
        {
        case CActiveAEControlProtocol::RECONFIGURE:
          if (m_streams.empty())
          {
            bool silence = false;
            m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SILENCEMODE, &silence, sizeof(bool));
          }
          LoadSettings();
          ChangeResamplers();
          if (!NeedReconfigureBuffers() && !NeedReconfigureSink())
            return;
          m_state = AE_TOP_RECONFIGURING;
          m_extTimeout = 0;
          // don't accept any data until we are reconfigured
          m_extDeferData = true;
          return;
        case CActiveAEControlProtocol::SUSPEND:
          UnconfigureSink();
          m_stats.SetSuspended(true);
          m_state = AE_TOP_CONFIGURED_SUSPEND;
          m_extDeferData = true;
          return;
        case CActiveAEControlProtocol::DISPLAYLOST:
          if (m_sink.GetDeviceType(m_mode == MODE_PCM ? m_settings.device : m_settings.passthoughdevice) == AE_DEVTYPE_HDMI)
          {
            UnconfigureSink();
            m_stats.SetSuspended(true);
            m_state = AE_TOP_CONFIGURED_SUSPEND;
            m_extDeferData = true;
          }
          return;
        case CActiveAEControlProtocol::PAUSESTREAM:
          CActiveAEStream *stream;
          stream = *(CActiveAEStream**)msg->data;
          if (stream->m_paused != true && m_streams.size() == 1)
            FlushEngine();
          stream->m_paused = true;
          return;
        case CActiveAEControlProtocol::RESUMESTREAM:
          stream = *(CActiveAEStream**)msg->data;
          stream->m_paused = false;
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
        case CActiveAEControlProtocol::STREAMAMP:
          MsgStreamParameter *par;
          par = (MsgStreamParameter*)msg->data;
          par->stream->m_limiter.SetAmplification(par->parameter.float_par);
          par->stream->m_amplify = par->parameter.float_par;
          return;
        case CActiveAEControlProtocol::STREAMVOLUME:
          par = (MsgStreamParameter*)msg->data;
          par->stream->m_volume = par->parameter.float_par;
          return;
        case CActiveAEControlProtocol::STREAMRGAIN:
          par = (MsgStreamParameter*)msg->data;
          par->stream->m_rgain = par->parameter.float_par;
          return;
        case CActiveAEControlProtocol::STREAMRESAMPLERATIO:
          par = (MsgStreamParameter*)msg->data;
          if (par->stream->m_resampleBuffers)
          {
            par->stream->m_resampleBuffers->m_resampleRatio = par->parameter.double_par;
          }
          return;
        case CActiveAEControlProtocol::STREAMFADE:
          MsgStreamFade *fade;
          fade = (MsgStreamFade*)msg->data;
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
          if (m_soundMode == AE_SOUND_OFF ||
             (m_soundMode == AE_SOUND_IDLE && !m_streams.empty()))
            return;
          if (sound)
          {
            SoundState st = {sound, 0};
            m_sounds_playing.push_back(st);
            m_extTimeout = 0;
            m_state = AE_TOP_CONFIGURED_PLAY;
          }
          return;
        case CActiveAEDataProtocol::NEWSTREAM:
          MsgStreamNew *streamMsg;
          CActiveAEStream *stream;
          streamMsg = (MsgStreamNew*)msg->data;
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
          msgData = (MsgStreamSample*)msg->data;
          samples = msgData->stream->m_processingSamples.front();
          msgData->stream->m_processingSamples.pop_front();
          if (samples != msgData->buffer)
            CLog::Log(LOGERROR, "CActiveAE - inconsistency in stream sample message");
          if (msgData->buffer->pkt->nb_samples == 0)
            msgData->buffer->Return();
          else
            msgData->stream->m_resampleBuffers->m_inputSamples.push_back(msgData->buffer);
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        case CActiveAEDataProtocol::FREESTREAM:
          stream = *(CActiveAEStream**)msg->data;
          DiscardStream(stream);
          if (m_streams.empty())
          {
            m_extDrainTimer.Set(m_stats.GetDelay() * 1000);
            m_extDrain = true;
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        case CActiveAEDataProtocol::DRAINSTREAM:
          stream = *(CActiveAEStream**)msg->data;
          stream->m_drain = true;
          stream->m_resampleBuffers->m_drain = true;
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          msg->Reply(CActiveAEDataProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (&m_sink.m_dataPort)
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
          displayReset = true;
        case CActiveAEControlProtocol::INIT:
          m_extError = false;
          if (!displayReset)
          {
            m_sink.EnumerateSinkList(true);
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
        default:
          break;
        }
      }
      else if (&m_sink.m_dataPort)
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
      break;

    case AE_TOP_CONFIGURED_IDLE:
      if (port == NULL) // timeout
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
            ResampleSounds();
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

  m_state = AE_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;
  m_extDrain = false;
  m_extDeferData = false;

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
    inputFormat.m_encodedRate   = 0;
    inputFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
    inputFormat.m_frames        = 0;
    inputFormat.m_frameSamples  = 0;
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

  inputFormat = GetInputFormat(desiredFmt);

  m_sinkRequestFormat = inputFormat;
  ApplySettingsToFormat(m_sinkRequestFormat, m_settings, (int*)&m_mode);
  std::string device = AE_IS_RAW(m_sinkRequestFormat.m_dataFormat) ? m_settings.passthoughdevice : m_settings.device;
  std::string driver;
  CAESinkFactory::ParseDevice(device, driver);
  if (!IsSinkCompatible(m_sinkRequestFormat, device) || m_settings.driver.compare(driver) != 0)
  {
    if (!InitSink())
      return;
    m_settings.driver = driver;
    initSink = true;
    m_stats.Reset(m_sinkFormat.m_sampleRate);
    m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::VOLUME, &m_volume, sizeof(float));

    // limit buffer size in case of sink returns large buffer
    unsigned int buffertime = (m_sinkFormat.m_frames*1000) / m_sinkFormat.m_sampleRate;
    if (buffertime > 80)
    {
      CLog::Log(LOGWARNING, "ActiveAE::%s - sink returned large buffer of %d ms, reducing to 80 ms", __FUNCTION__, buffertime);
      m_sinkFormat.m_frames = 80 * m_sinkFormat.m_sampleRate / 1000;
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
    inputFormat.m_dataFormat = AE_FMT_FLOAT;
    inputFormat.m_frameSize = inputFormat.m_channelLayout.Count() *
                              (CAEUtil::DataFormatToBits(inputFormat.m_dataFormat) >> 3);
    m_silenceBuffers = new CActiveAEBufferPool(inputFormat);
    m_silenceBuffers->Create(MAX_WATER_LEVEL*1000);
    sinkInputFormat = inputFormat;
    m_internalFormat = inputFormat;

    bool silence = false;
    m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SILENCEMODE, &silence, sizeof(bool));

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
    bool silence = true;
    m_sink.m_controlPort.SendOutMessage(CSinkControlProtocol::SILENCEMODE, &silence, sizeof(bool));

    AEAudioFormat outputFormat;
    if (m_mode == MODE_RAW)
    {
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
        format.m_channelLayout = AE_CH_LAYOUT_2_0;
        format.m_dataFormat = AE_FMT_S16NE;
        format.m_frameSize = 2* (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
        format.m_frames = AC3_FRAME_SIZE;
        format.m_sampleRate = 48000;
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
      outputFormat.m_dataFormat = AE_FMT_FLOAT;
      outputFormat.m_frameSize = outputFormat.m_channelLayout.Count() *
                                 (CAEUtil::DataFormatToBits(outputFormat.m_dataFormat) >> 3);
      // TODO: adjust to decoder
      sinkInputFormat = outputFormat;
    }
    m_internalFormat = outputFormat;

    std::list<CActiveAEStream*>::iterator it;
    for(it=m_streams.begin(); it!=m_streams.end(); ++it)
    {
      // check if we support input format of stream
      if (!AE_IS_RAW((*it)->m_format.m_dataFormat) && 
          CActiveAEResample::GetAVSampleFormat((*it)->m_format.m_dataFormat) == AV_SAMPLE_FMT_FLT &&
          (*it)->m_format.m_dataFormat != AE_FMT_FLOAT)
      {
        (*it)->m_convertFn = CAEConvert::ToFloat((*it)->m_format.m_dataFormat);
        (*it)->m_format.m_dataFormat = AE_FMT_FLOAT;
      }

      if (!(*it)->m_inputBuffers)
      {
        // align input buffers with period of sink or encoder
        (*it)->m_format.m_frames = m_internalFormat.m_frames * ((float)(*it)->m_format.m_sampleRate / m_internalFormat.m_sampleRate);

        // create buffer pool
        (*it)->m_inputBuffers = new CActiveAEBufferPool((*it)->m_format);
        (*it)->m_inputBuffers->Create(MAX_CACHE_LEVEL*1000);
        (*it)->m_streamSpace = (*it)->m_format.m_frameSize * (*it)->m_format.m_frames;
      }
      if (initSink && (*it)->m_resampleBuffers)
      {
        m_discardBufferPools.push_back((*it)->m_resampleBuffers);
        (*it)->m_resampleBuffers = NULL;
      }
      if (!(*it)->m_resampleBuffers)
      {
        (*it)->m_resampleBuffers = new CActiveAEBufferPoolResample((*it)->m_inputBuffers->m_format, outputFormat, m_settings.resampleQuality);
        (*it)->m_resampleBuffers->m_changeResampler = (*it)->m_forceResampler;
        (*it)->m_resampleBuffers->Create(MAX_CACHE_LEVEL*1000, false, m_settings.stereoupmix, m_settings.normalizelevels);
      }
      if (m_mode == MODE_TRANSCODE || m_streams.size() > 1)
        (*it)->m_resampleBuffers->m_fillPackets = true;

      // amplification
      (*it)->m_limiter.SetSamplerate(outputFormat.m_sampleRate);
    }

    // buffers for viz
    if (!AE_IS_RAW(inputFormat.m_dataFormat))
    {
      if (initSink && m_vizBuffers)
      {
        m_discardBufferPools.push_back(m_vizBuffers);
        m_vizBuffers = NULL;
        m_discardBufferPools.push_back(m_vizBuffersInput);
        m_vizBuffersInput = NULL;
      }
      if (!m_vizBuffers)
      {
        AEAudioFormat vizFormat = m_internalFormat;
        vizFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
        vizFormat.m_dataFormat = AE_FMT_FLOAT;

        // input buffers
        m_vizBuffersInput = new CActiveAEBufferPool(m_internalFormat);
        m_vizBuffersInput->Create(2000);

        // resample buffers
        m_vizBuffers = new CActiveAEBufferPoolResample(m_internalFormat, vizFormat, m_settings.resampleQuality);
        // TODO use cache of sync + water level
        m_vizBuffers->Create(2000, false, false);
        m_vizInitialized = false;
      }
    }
  }

  // resample buffers for sink
  if (m_sinkBuffers && 
     (!CompareFormat(m_sinkBuffers->m_format,m_sinkFormat) || !CompareFormat(m_sinkBuffers->m_inputFormat, sinkInputFormat)))
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
    std::vector<CActiveAESound*>::iterator it;
    for (it = m_sounds.begin(); it != m_sounds.end(); ++it)
    {
      (*it)->SetConverted(false);
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
    if((*it)->IsDrained())
      continue;
    if(AE_IS_RAW((*it)->m_format.m_dataFormat))
      hasRawStream = true;
    hasStream = true;
  }
  if (hasRawStream || (hasStream && AE_IS_RAW(streamMsg->format.m_dataFormat)))
  {
    return NULL;
  }

  // create the stream
  CActiveAEStream *stream;
  stream = new CActiveAEStream(&streamMsg->format);
  stream->m_streamPort = new CActiveAEDataProtocol("stream",
                             &stream->m_inMsgEvent, &m_outMsgEvent);

  // create buffer pool
  stream->m_inputBuffers = NULL; // create in Configure when we know the sink format
  stream->m_resampleBuffers = NULL; // create in Configure when we know the sink format
  stream->m_statsLock = m_stats.GetLock();
  stream->m_fadingSamples = 0;
  stream->m_started = false;

  if (streamMsg->options & AESTREAM_PAUSED)
    stream->m_paused = true;

  if (streamMsg->options & AESTREAM_FORCE_RESAMPLE)
    stream->m_forceResampler = true;

  m_streams.push_back(stream);

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
      m_discardBufferPools.push_back((*it)->m_inputBuffers);
      m_discardBufferPools.push_back((*it)->m_resampleBuffers);
      CLog::Log(LOGDEBUG, "CActiveAE::DiscardStream - audio stream deleted");
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
  stream->m_resampleBuffers->Flush();
  stream->m_streamPort->Purge();
  stream->m_bufferedTime = 0.0;
  stream->m_paused = false;

  // flush the engine if we only have a single stream
  if (m_streams.size() == 1)
  {
    FlushEngine();
  }
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
    bool success = reply->signal == CSinkControlProtocol::ACC ? true : false;
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
  m_stats.Reset(m_sinkFormat.m_sampleRate);
}

void CActiveAE::ClearDiscardedBuffers()
{
  std::list<CActiveAEBufferPool*>::iterator it;
  for (it=m_discardBufferPools.begin(); it!=m_discardBufferPools.end(); ++it)
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
      m_discardBufferPools.erase(it);
      return;
    }
  }
}

void CActiveAE::SStopSound(CActiveAESound *sound)
{
  std::list<SoundState>::iterator it;
  for (it=m_sounds_playing.begin(); it!=m_sounds_playing.end(); ++it)
  {
    if (it->sound == sound)
    {
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
      return;
    }
  }
}

void CActiveAE::ChangeResamplers()
{
  std::list<CActiveAEStream*>::iterator it;
  for(it=m_streams.begin(); it!=m_streams.end(); ++it)
  {
    bool normalize = true;
    if (((*it)->m_resampleBuffers->m_format.m_channelLayout.Count() <
         (*it)->m_resampleBuffers->m_inputFormat.m_channelLayout.Count()) &&
         !m_settings.normalizelevels)
      normalize = false;

    if ((*it)->m_resampleBuffers && (*it)->m_resampleBuffers->m_resampler &&
        (((*it)->m_resampleBuffers->m_resampleQuality != m_settings.resampleQuality) ||
        (((*it)->m_resampleBuffers->m_stereoUpmix != m_settings.stereoupmix)) ||
        ((*it)->m_resampleBuffers->m_normalize != normalize)))
    {
      (*it)->m_resampleBuffers->m_changeResampler = true;
    }
    (*it)->m_resampleBuffers->m_resampleQuality = m_settings.resampleQuality;
    (*it)->m_resampleBuffers->m_stereoUpmix = m_settings.stereoupmix;
    (*it)->m_resampleBuffers->m_normalize = normalize;
  }
}

void CActiveAE::ApplySettingsToFormat(AEAudioFormat &format, AudioSettings &settings, int *mode)
{
  if (mode)
    *mode = MODE_PCM;

  // raw pass through
  if (AE_IS_RAW(format.m_dataFormat))
  {
    if ((format.m_dataFormat == AE_FMT_AC3 && !settings.ac3passthrough) ||
        (format.m_dataFormat == AE_FMT_EAC3 && !settings.eac3passthrough) ||
        (format.m_dataFormat == AE_FMT_TRUEHD && !settings.truehdpassthrough) ||
        (format.m_dataFormat == AE_FMT_DTS && !settings.dtspassthrough) ||
        (format.m_dataFormat == AE_FMT_DTSHD && !settings.dtshdpassthrough))
    {
      CLog::Log(LOGERROR, "CActiveAE::ApplySettingsToFormat - input audio format is wrong");
    }
    if (mode)
      *mode = MODE_RAW;
  }
  // transcode
  else if (settings.channels <= AE_CH_LAYOUT_2_0 && // no multichannel pcm
           settings.passthrough &&
           settings.ac3passthrough &&
           !m_streams.empty() &&
           (format.m_channelLayout.Count() > 2 || settings.stereoupmix))
  {
    format.m_dataFormat = AE_FMT_AC3;
    format.m_sampleRate = 48000;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
    if (mode)
      *mode = MODE_TRANSCODE;
  }
  else
  {
    format.m_dataFormat = AE_FMT_FLOAT;
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
      else
        format.m_channelLayout.ResolveChannels(stdLayout);
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

  if (newFormat.m_dataFormat != m_sinkRequestFormat.m_dataFormat ||
      newFormat.m_channelLayout != m_sinkRequestFormat.m_channelLayout ||
      newFormat.m_sampleRate != m_sinkRequestFormat.m_sampleRate)
    return true;

  return false;
}

bool CActiveAE::NeedReconfigureSink()
{
  AEAudioFormat newFormat = GetInputFormat();
  ApplySettingsToFormat(newFormat, m_settings);

  std::string device = AE_IS_RAW(newFormat.m_dataFormat) ? m_settings.passthoughdevice : m_settings.device;
  std::string driver;
  CAESinkFactory::ParseDevice(device, driver);
  if (m_settings.driver.compare(driver) != 0)
    return true;

  if (!IsSinkCompatible(newFormat, device))
    return true;

  return false;
}

bool CActiveAE::InitSink()
{
  SinkConfig config;
  config.format = m_sinkRequestFormat;
  config.stats = &m_stats;
  config.device = AE_IS_RAW(m_sinkRequestFormat.m_dataFormat) ? &m_settings.passthoughdevice :
                                                                &m_settings.device;

  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::CONFIGURE,
                                                 &reply,
                                                 5000,
                                                 &config, sizeof(config)))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC ? true : false;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      m_extError = true;
      return false;
    }
    AEAudioFormat *data;
    data = (AEAudioFormat*)reply->data;
    if (data)
    {
      m_sinkFormat = *data;
    }
    m_sinkHasVolume = m_sink.HasVolume();
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
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
    bool success = reply->signal == CSinkDataProtocol::ACC ? true : false;
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

bool CActiveAE::IsSinkCompatible(const AEAudioFormat format, const std::string &device)
{
  bool compatible = false;
  SinkConfig config;
  config.format = format;
  config.device = &device;

  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::ISCOMPATIBLE,
                                                 &reply,
                                                 1000,
                                                 &config, sizeof(config)))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC ? true : false;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      m_extError = true;
      return false;
    }
    compatible = *(bool*)reply->data;
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to query compatibility", __FUNCTION__);
    m_extError = true;
    return false;
  }
  return compatible;
}

void CActiveAE::UnconfigureSink()
{
  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::UNCONFIGURE,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC ? true : false;
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

  m_inMsgEvent.Reset();
}


bool CActiveAE::RunStages()
{
  bool busy = false;

  // serve input streams
  std::list<CActiveAEStream*>::iterator it;
  for (it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    if ((*it)->m_resampleBuffers && !(*it)->m_paused)
      busy = (*it)->m_resampleBuffers->ResampleBuffers();
    else if ((*it)->m_resampleBuffers && 
            ((*it)->m_resampleBuffers->m_inputSamples.size() > (*it)->m_resampleBuffers->m_allSamples.size() * 0.5))
    {
      CSingleLock lock((*it)->m_streamLock);
      (*it)->m_streamIsBuffering = false;
    }

    // provide buffers to stream
    float time = m_stats.GetCacheTime((*it));
    CSampleBuffer *buffer;
    if (!(*it)->m_drain)
    {
      while (time < MAX_CACHE_LEVEL && !(*it)->m_inputBuffers->m_freeSamples.empty())
      {
        buffer = (*it)->m_inputBuffers->GetFreeBuffer();
        (*it)->m_processingSamples.push_back(buffer);
        (*it)->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMBUFFER, &buffer, sizeof(CSampleBuffer*));
        (*it)->IncFreeBuffers();
        time += (float)buffer->pkt->max_nb_samples / buffer->pkt->config.sample_rate;
      }
    }
    else
    {
      if ((*it)->m_resampleBuffers->m_inputSamples.empty() &&
          (*it)->m_resampleBuffers->m_outputSamples.empty() &&
          (*it)->m_processingSamples.empty())
      {
        (*it)->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMDRAINED);
        (*it)->m_drain = false;
        (*it)->m_resampleBuffers->m_drain = false;
        (*it)->m_started = false;

        // set variables being polled via stream interface
        CSingleLock lock((*it)->m_streamLock);
        if ((*it)->m_streamSlave)
        {
          CActiveAEStream *slave = (CActiveAEStream*)((*it)->m_streamSlave);
          slave->m_paused = false;

          // TODO: find better solution for this
          // gapless bites audiophile
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
        if ((*it)->m_paused || !(*it)->m_started || !(*it)->m_resampleBuffers)
          continue;

        if ((*it)->m_resampleBuffers->m_outputSamples.empty())
          allStreamsReady = false;
      }

      bool needClamp = false;
      for (it = m_streams.begin(); it != m_streams.end() && allStreamsReady; ++it)
      {
        if ((*it)->m_paused || !(*it)->m_resampleBuffers)
          continue;

        if (!(*it)->m_resampleBuffers->m_outputSamples.empty())
        {
          (*it)->m_started = true;

          if (!out)
          {
            out = (*it)->m_resampleBuffers->m_outputSamples.front();
            (*it)->m_resampleBuffers->m_outputSamples.pop_front();

            int nb_floats = out->pkt->nb_samples * out->pkt->config.channels / out->pkt->planes;
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
            if ((*it)->m_amplify != 1.0 || !(*it)->m_resampleBuffers->m_normalize || (m_sinkFormat.m_dataFormat == AE_FMT_FLOAT))
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
#ifdef __SSE__
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
            mix = (*it)->m_resampleBuffers->m_outputSamples.front();
            (*it)->m_resampleBuffers->m_outputSamples.pop_front();

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
            if ((*it)->m_amplify != 1.0 || !(*it)->m_resampleBuffers->m_normalize)
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
#ifdef __SSE__
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
      if(out && needClamp)
      {
        int nb_floats = out->pkt->nb_samples * out->pkt->config.channels / out->pkt->planes;
        for(int i=0; i<out->pkt->planes; i++)
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
          if (m_audioCallback && m_vizBuffers && !m_streams.empty())
          {
            if (!m_vizInitialized)
            {
              m_audioCallback->OnInitialize(2, m_vizBuffers->m_format.m_sampleRate, 32);
              m_vizInitialized = true;
            }

            if (!m_vizBuffersInput->m_freeSamples.empty())
            {
              // copy the samples into the viz input buffer
              CSampleBuffer *viz = m_vizBuffersInput->GetFreeBuffer();
              int samples = std::min(512, out->pkt->nb_samples);
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
            unsigned int now = XbmcThreads::SystemClockMillis();
            unsigned int timestamp = now + m_stats.GetDelay() * 1000;
            busy |= m_vizBuffers->ResampleBuffers(timestamp);
            while(!m_vizBuffers->m_outputSamples.empty())
            {
              CSampleBuffer *buf = m_vizBuffers->m_outputSamples.front();
              if ((now - buf->timestamp) & 0x80000000)
                break;
              else
              {
                int samples;
                samples = std::min(512, buf->pkt->nb_samples);
                m_audioCallback->OnAudioData((float*)(buf->pkt->data[0]), samples);
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
          CSampleBuffer *buf = m_encoderBuffers->GetFreeBuffer();
          m_encoder->Encode(out->pkt->data[0], out->pkt->planes*out->pkt->linesize,
                            buf->pkt->data[0], buf->pkt->planes*buf->pkt->linesize);
          buf->pkt->nb_samples = buf->pkt->max_nb_samples;
          out->Return();
          out = buf;
        }
        busy = true;
      }

      // update stats
      if(out)
      {
        m_stats.AddSamples(out->pkt->nb_samples, m_streams);
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
        if (!(*it)->m_resampleBuffers->m_outputSamples.empty())
        {
          buffer =  (*it)->m_resampleBuffers->m_outputSamples.front();
          (*it)->m_resampleBuffers->m_outputSamples.pop_front();
          m_stats.AddSamples(buffer->pkt->nb_samples, m_streams);
          m_sinkBuffers->m_inputSamples.push_back(buffer);
        }
      }
    }

    // serve sink buffers
    busy = m_sinkBuffers->ResampleBuffers();
    while(!m_sinkBuffers->m_outputSamples.empty())
    {
      CSampleBuffer *out = NULL;
      out = m_sinkBuffers->m_outputSamples.front();
      m_sinkBuffers->m_outputSamples.pop_front();
      m_sink.m_dataPort.SendOutMessage(CSinkDataProtocol::SAMPLE,
          &out, sizeof(CSampleBuffer*));
      busy = true;
    }
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
    if (!(*it)->m_resampleBuffers->m_inputSamples.empty())
      return true;
    if (!(*it)->m_resampleBuffers->m_outputSamples.empty())
      return true;
    if (!(*it)->m_processingSamples.empty())
      return true;
  }

  return false;
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
                m_dllAvUtil.av_get_bytes_per_sample(it->sound->GetSound(false)->config.fmt) *
                it->sound->GetSound(false)->config.channels /
                it->sound->GetSound(false)->planes;

    for(int j=0; j<dstSample.planes; j++)
    {
      volume = it->sound->GetVolume();
      out = (float*)dstSample.data[j];
      sample_buffer = (float*)(it->sound->GetSound(false)->data[j]+start);
      int nb_floats = mix_samples * dstSample.config.channels / dstSample.planes;
#ifdef __SSE__
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
  if (m_volume < 1.0 || m_muted)
  {
    float *buffer;
    int nb_floats = dstSample.nb_samples * dstSample.config.channels / dstSample.planes;

    for(int j=0; j<dstSample.planes; j++)
    {
      buffer = (float*)dstSample.data[j];
#ifdef __SSE__
      CAEUtil::SSEMulArray(buffer, m_muted ? 0.0 : m_volume, nb_floats);
#else
      float *fbuffer = buffer;
      for (int i = 0; i < nb_floats; i++)
        *fbuffer++ *= m_volume;
#endif
    }
  }
}

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------

void CActiveAE::LoadSettings()
{
  m_settings.device = CSettings::Get().GetString("audiooutput.audiodevice");
  m_settings.passthoughdevice = CSettings::Get().GetString("audiooutput.passthroughdevice");

  m_settings.config = CSettings::Get().GetInt("audiooutput.config");
  m_settings.channels = (m_sink.GetDeviceType(m_settings.device) == AE_DEVTYPE_IEC958) ? AE_CH_LAYOUT_2_0 : CSettings::Get().GetInt("audiooutput.channels");
  m_settings.samplerate = CSettings::Get().GetInt("audiooutput.samplerate");

  m_settings.stereoupmix = IsSettingVisible("audiooutput.stereoupmix") ? CSettings::Get().GetBool("audiooutput.stereoupmix") : false;
  m_settings.normalizelevels = CSettings::Get().GetBool("audiooutput.normalizelevels");

  m_settings.passthrough = m_settings.config == AE_CONFIG_FIXED ? false : CSettings::Get().GetBool("audiooutput.passthrough");
  m_settings.ac3passthrough = CSettings::Get().GetBool("audiooutput.ac3passthrough");
  m_settings.eac3passthrough = CSettings::Get().GetBool("audiooutput.eac3passthrough");
  m_settings.truehdpassthrough = CSettings::Get().GetBool("audiooutput.truehdpassthrough");
  m_settings.dtspassthrough = CSettings::Get().GetBool("audiooutput.dtspassthrough");
  m_settings.dtshdpassthrough = CSettings::Get().GetBool("audiooutput.dtshdpassthrough");

  m_settings.resampleQuality = static_cast<AEQuality>(CSettings::Get().GetInt("audiooutput.processquality"));
}

bool CActiveAE::Initialize()
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())
  {
    CLog::Log(LOGERROR,"CActiveAE::Initialize - failed to load ffmpeg libraries");
    return false;
  }
  m_dllAvFormat.av_register_all();

  Create();
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::INIT,
                                                 &reply,
                                                 5000))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      Dispose();
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
    Dispose();
    return false;
  }

  // hook into windowing for receiving display reset events
#if defined(HAS_GLX) || defined(TARGET_DARWIN_OSX)
  g_Windowing.Register(this);
#endif

  m_inMsgEvent.Reset();
  return true;
}

void CActiveAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  m_sink.EnumerateOutputDevices(devices, passthrough);
}

std::string CActiveAE::GetDefaultDevice(bool passthrough)
{
  return m_sink.GetDefaultDevice(passthrough);
}

void CActiveAE::OnSettingsChange(const std::string& setting)
{
  if (setting == "audiooutput.passthroughdevice" ||
      setting == "audiooutput.audiodevice"       ||
      setting == "audiooutput.config"            ||
      setting == "audiooutput.ac3passthrough"    ||
      setting == "audiooutput.eac3passthrough"   ||
      setting == "audiooutput.dtspassthrough"    ||
      setting == "audiooutput.truehdpassthrough" ||
      setting == "audiooutput.dtshdpassthrough"  ||
      setting == "audiooutput.channels"          ||
      setting == "audiooutput.stereoupmix"       ||
      setting == "audiooutput.streamsilence"     ||
      setting == "audiooutput.processquality"    ||
      setting == "audiooutput.passthrough"       ||
      setting == "audiooutput.samplerate"        ||
      setting == "audiooutput.normalizelevels")
  {
    m_controlPort.SendOutMessage(CActiveAEControlProtocol::RECONFIGURE);
  }
}

bool CActiveAE::SupportsRaw(AEDataFormat format)
{
  if (!m_sink.HasPassthroughDevice())
    return false;

  // those formats require HDMI
  if (format == AE_FMT_DTSHD || format == AE_FMT_TRUEHD)
  {
    if(m_sink.GetDeviceType(CSettings::Get().GetString("audiooutput.passthroughdevice")) != AE_DEVTYPE_HDMI)
      return false;
  }

  // TODO: check ELD?
  return true;
}

bool CActiveAE::SupportsSilenceTimeout()
{
  return true;
}

bool CActiveAE::SupportsQualityLevel(enum AEQuality level)
{
  if (level == AE_QUALITY_LOW || level == AE_QUALITY_MID || level == AE_QUALITY_HIGH)
    return true;

  return false;
}

bool CActiveAE::IsSettingVisible(const std::string &settingId)
{
  if (settingId == "audiooutput.samplerate")
  {
    if (m_sink.GetDeviceType(CSettings::Get().GetString("audiooutput.audiodevice")) == AE_DEVTYPE_IEC958)
      return true;
    if (CSettings::Get().GetInt("audiooutput.config") == AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == "audiooutput.channels")
  {
    if (m_sink.GetDeviceType(CSettings::Get().GetString("audiooutput.audiodevice")) != AE_DEVTYPE_IEC958)
      return true;
  }
  else if (settingId == "audiooutput.passthrough")
  {
    if (m_sink.HasPassthroughDevice() && CSettings::Get().GetInt("audiooutput.config") != AE_CONFIG_FIXED)
      return true;
  }
  else if (settingId == "audiooutput.truehdpassthrough")
  {
    if (m_sink.HasPassthroughDevice() &&
        CSettings::Get().GetInt("audiooutput.config") != AE_CONFIG_FIXED &&
        m_sink.GetDeviceType(CSettings::Get().GetString("audiooutput.passthroughdevice")) == AE_DEVTYPE_HDMI)
      return true;
  }
  else if (settingId == "audiooutput.dtshdpassthrough")
  {
    if (m_sink.HasPassthroughDevice() &&
        CSettings::Get().GetInt("audiooutput.config") != AE_CONFIG_FIXED &&
        m_sink.GetDeviceType(CSettings::Get().GetString("audiooutput.passthroughdevice")) == AE_DEVTYPE_HDMI)
      return true;
  }
  else if (settingId == "audiooutput.stereoupmix")
  {
    if (m_sink.GetDeviceType(CSettings::Get().GetString("audiooutput.audiodevice")) != AE_DEVTYPE_IEC958)
    {
      if (CSettings::Get().GetInt("audiooutput.channels") > AE_CH_LAYOUT_2_0)
        return true;
    }
    else
    {
      if (m_sink.HasPassthroughDevice() &&
          CSettings::Get().GetBool("audiooutput.passthrough") &&
          CSettings::Get().GetBool("audiooutput.ac3passthrough"))
        return true;
    }
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
    bool success = reply->signal == CActiveAEControlProtocol::ACC ? true : false;
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

void CActiveAE::SetSoundMode(const int mode)
{
  int soundmode = mode;
  m_controlPort.SendOutMessage(CActiveAEControlProtocol::SOUNDMODE, &soundmode, sizeof(int));
}


void CActiveAE::OnLostDevice()
{
//  m_controlPort.SendOutMessage(CActiveAEControlProtocol::DISPLAYLOST);
}

void CActiveAE::OnResetDevice()
{
//  m_controlPort.SendOutMessage(CActiveAEControlProtocol::DISPLAYRESET);
}

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------

uint8_t **CActiveAE::AllocSoundSample(SampleConfig &config, int &samples, int &bytes_per_sample, int &planes, int &linesize)
{
  uint8_t **buffer;
  planes = m_dllAvUtil.av_sample_fmt_is_planar(config.fmt) ? config.channels : 1;
  buffer = new uint8_t*[planes];

  // align buffer to 16 in order to be compatible with sse in CAEConvert
  m_dllAvUtil.av_samples_alloc(buffer, &linesize, config.channels,
                                 samples, config.fmt, 16);
  bytes_per_sample = m_dllAvUtil.av_get_bytes_per_sample(config.fmt);
  return buffer;
}

void CActiveAE::FreeSoundSample(uint8_t **data)
{
  m_dllAvUtil.av_freep(data);
  delete [] data;
}

bool CActiveAE::CompareFormat(AEAudioFormat &lhs, AEAudioFormat &rhs)
{
  if (lhs.m_channelLayout != rhs.m_channelLayout ||
      lhs.m_dataFormat != rhs.m_dataFormat ||
      lhs.m_sampleRate != rhs.m_sampleRate ||
      lhs.m_frames != rhs.m_frames)
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

#define SOUNDBUFFER_SIZE 20480

IAESound *CActiveAE::MakeSound(const std::string& file)
{
  AVFormatContext *fmt_ctx = NULL;
  AVCodecContext *dec_ctx = NULL;
  AVIOContext *io_ctx;
  AVInputFormat *io_fmt;
  AVCodec *dec = NULL;
  CActiveAESound *sound = NULL;
  SampleConfig config;

  sound = new CActiveAESound(file);
  if (!sound->Prepare())
  {
    delete sound;
    return NULL;
  }
  int fileSize = sound->GetFileSize();

  fmt_ctx = m_dllAvFormat.avformat_alloc_context();
  unsigned char* buffer = (unsigned char*)m_dllAvUtil.av_malloc(SOUNDBUFFER_SIZE+FF_INPUT_BUFFER_PADDING_SIZE);
  io_ctx = m_dllAvFormat.avio_alloc_context(buffer, SOUNDBUFFER_SIZE, 0,
                                            sound, CActiveAESound::Read, NULL, CActiveAESound::Seek);
  io_ctx->max_packet_size = sound->GetChunkSize();
  if(io_ctx->max_packet_size)
    io_ctx->max_packet_size *= SOUNDBUFFER_SIZE / io_ctx->max_packet_size;

  if(!sound->IsSeekPosible())
    io_ctx->seekable = 0;

  fmt_ctx->pb = io_ctx;

  m_dllAvFormat.av_probe_input_buffer(io_ctx, &io_fmt, file.c_str(), NULL, 0, 0);
  if (!io_fmt)
  {
    m_dllAvFormat.avformat_close_input(&fmt_ctx);
    delete sound;
    return NULL;
  }

  // find decoder
  if (m_dllAvFormat.avformat_open_input(&fmt_ctx, file.c_str(), NULL, NULL) == 0)
  {
    fmt_ctx->flags |= AVFMT_FLAG_NOPARSE;
    if (m_dllAvFormat.avformat_find_stream_info(fmt_ctx, NULL) >= 0)
    {
      dec_ctx = fmt_ctx->streams[0]->codec;
      dec = m_dllAvCodec.avcodec_find_decoder(dec_ctx->codec_id);
      config.sample_rate = dec_ctx->sample_rate;
      config.channels = dec_ctx->channels;
      config.channel_layout = dec_ctx->channel_layout;
    }
  }
  if (dec == NULL)
  {
    m_dllAvFormat.avformat_close_input(&fmt_ctx);
    delete sound;
    return NULL;
  }

  dec_ctx = m_dllAvCodec.avcodec_alloc_context3(dec);
  dec_ctx->sample_rate = config.sample_rate;
  dec_ctx->channels = config.channels;
  if (!config.channel_layout)
    config.channel_layout = m_dllAvUtil.av_get_default_channel_layout(config.channels);
  dec_ctx->channel_layout = config.channel_layout;

  AVPacket avpkt;
  AVFrame *decoded_frame = NULL;
  decoded_frame = m_dllAvCodec.avcodec_alloc_frame();

  if (m_dllAvCodec.avcodec_open2(dec_ctx, dec, NULL) >= 0)
  {
    bool init = false;

    // decode until eof
    m_dllAvCodec.av_init_packet(&avpkt);
    int len;
    while (m_dllAvFormat.av_read_frame(fmt_ctx, &avpkt) >= 0)
    {
      int got_frame = 0;
      len = m_dllAvCodec.avcodec_decode_audio4(dec_ctx, decoded_frame, &got_frame, &avpkt);
      if (len < 0)
      {
        m_dllAvCodec.avcodec_close(dec_ctx);
        m_dllAvUtil.av_free(dec_ctx);
        m_dllAvUtil.av_free(&decoded_frame);
        m_dllAvFormat.avformat_close_input(&fmt_ctx);
        delete sound;
        return NULL;
      }
      if (got_frame)
      {
        if (!init)
        {
          int samples = fileSize / m_dllAvUtil.av_get_bytes_per_sample(dec_ctx->sample_fmt) / config.channels;
          config.fmt = dec_ctx->sample_fmt;
          config.bits_per_sample = dec_ctx->bits_per_coded_sample;
          sound->InitSound(true, config, samples);
          init = true;
        }
        sound->StoreSound(true, decoded_frame->extended_data,
                          decoded_frame->nb_samples, decoded_frame->linesize[0]);
      }
    }
    m_dllAvCodec.avcodec_close(dec_ctx);
  }

  m_dllAvUtil.av_free(dec_ctx);
  m_dllAvUtil.av_free(decoded_frame);
  m_dllAvFormat.avformat_close_input(&fmt_ctx);

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

  dst_config.channel_layout = CActiveAEResample::GetAVChannelLayout(m_internalFormat.m_channelLayout);
  dst_config.channels = m_internalFormat.m_channelLayout.Count();
  dst_config.sample_rate = m_internalFormat.m_sampleRate;
  dst_config.fmt = CActiveAEResample::GetAVSampleFormat(m_internalFormat.m_dataFormat);
  dst_config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_internalFormat.m_dataFormat);

  CActiveAEResample *resampler = new CActiveAEResample();
  resampler->Init(dst_config.channel_layout,
                  dst_config.channels,
                  dst_config.sample_rate,
                  dst_config.fmt,
                  dst_config.bits_per_sample,
                  orig_config.channel_layout,
                  orig_config.channels,
                  orig_config.sample_rate,
                  orig_config.fmt,
                  orig_config.bits_per_sample,
                  false,
                  true,
                  NULL,
                  AE_QUALITY_MID);

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

IAEStream *CActiveAE::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options)
{
  //TODO: pass number of samples in audio packet

  AEAudioFormat format;
  format.m_dataFormat = dataFormat;
  format.m_sampleRate = sampleRate;
  format.m_encodedRate = encodedSampleRate;
  format.m_channelLayout = channelLayout;
  format.m_frames = format.m_sampleRate / 10;
  format.m_frameSize = format.m_channelLayout.Count() *
                       (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);

  MsgStreamNew msg;
  msg.format = format;
  msg.options = options;

  Message *reply;
  if (m_dataPort.SendOutMessageSync(CActiveAEDataProtocol::NEWSTREAM,
                                    &reply,10000,
                                    &msg, sizeof(MsgStreamNew)))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC ? true : false;
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

IAEStream *CActiveAE::FreeStream(IAEStream *stream)
{
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::FREESTREAM, &stream, sizeof(IAEStream*));
  return NULL;
}

void CActiveAE::FlushStream(CActiveAEStream *stream)
{
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::FLUSHSTREAM,
                                       &reply,1000,
                                       &stream, sizeof(CActiveAEStream*)))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "CActiveAE::FlushStream - failed");
    }
  }
}

void CActiveAE::PauseStream(CActiveAEStream *stream, bool pause)
{
  // TODO pause sink, needs api change
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
  m_audioCallback = pCallback;
  m_vizInitialized = false;
}

void CActiveAE::UnregisterAudioCallback()
{
  CSingleLock lock(m_vizLock);
  m_audioCallback = NULL;
}
