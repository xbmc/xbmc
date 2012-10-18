/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "system.h"

#include "CoreAudioAE.h"

#include "MathUtils.h"
#include "CoreAudioAEStream.h"
#include "CoreAudioAESound.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#define DELAY_FRAME_TIME  20
#define BUFFERSIZE        16416

CCoreAudioAE::CCoreAudioAE() :
  m_Initialized        (false         ),
  m_callbackRunning    (false         ),
  m_chLayoutCount      (0             ),
  m_rawPassthrough     (false         ),
  m_volume             (1.0f          ),
  m_volumeBeforeMute   (1.0f          ),
  m_muted              (false         ),
  m_soundMode          (AE_SOUND_OFF  ),
  m_streamsPlaying     (false         ),
  m_isSuspended        (false         )
{
  HAL = new CCoreAudioAEHAL;
}

CCoreAudioAE::~CCoreAudioAE()
{
  Shutdown();
}

void CCoreAudioAE::Shutdown()
{
  Stop();

  Deinitialize();

  /* free the streams */
  CSingleLock streamLock(m_streamLock);
  while (!m_streams.empty())
  {
    CCoreAudioAEStream *s = m_streams.front();
    m_sounds.pop_front();
    delete s;
  }

  /* free the sounds */
  CSingleLock soundLock(m_soundLock);
  while (!m_sounds.empty())
  {
    CCoreAudioAESound *s = m_sounds.front();
    m_sounds.pop_front();
    delete s;
  }

  delete HAL;
  HAL = NULL;
}

bool CCoreAudioAE::Initialize()
{
  Stop();

  Deinitialize();

  bool ret = OpenCoreAudio(44100, false, AE_FMT_FLOAT);

  Start();

  return ret;
}

bool CCoreAudioAE::OpenCoreAudio(unsigned int sampleRate, bool forceRaw,
  enum AEDataFormat rawDataFormat)
{

  // remove any deleted streams
  CSingleLock streamLock(m_streamLock);
  for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end();)
  {
    CCoreAudioAEStream *stream = *itt;
    if (stream->IsDestroyed())
    {
      itt = m_streams.erase(itt);
      delete stream;
      continue;
    }
    else
    {
      // close all converter
      stream->CloseConverter();
    }
    ++itt;
  }

  /* override the sample rate based on the oldest stream if there is one */
  if (!m_streams.empty())
    sampleRate = m_streams.front()->GetSampleRate();

  if (forceRaw)
    m_rawPassthrough = true;
  else
    m_rawPassthrough = !m_streams.empty() && m_streams.front()->IsRaw();
  streamLock.Leave();
    
  if (m_rawPassthrough)
    CLog::Log(LOGINFO, "CCoreAudioAE::OpenCoreAudio - RAW passthrough enabled");

  std::string m_outputDevice =  g_guiSettings.GetString("audiooutput.audiodevice");

  // on iOS devices we set fixed to two channels.
  m_stdChLayout = AE_CH_LAYOUT_2_0;
#if defined(TARGET_DARWIN_OSX)
  switch (g_guiSettings.GetInt("audiooutput.channellayout"))
  {
    default:
    case  0: m_stdChLayout = AE_CH_LAYOUT_2_0; break; /* do not allow 1_0 output */
    case  1: m_stdChLayout = AE_CH_LAYOUT_2_0; break;
    case  2: m_stdChLayout = AE_CH_LAYOUT_2_1; break;
    case  3: m_stdChLayout = AE_CH_LAYOUT_3_0; break;
    case  4: m_stdChLayout = AE_CH_LAYOUT_3_1; break;
    case  5: m_stdChLayout = AE_CH_LAYOUT_4_0; break;
    case  6: m_stdChLayout = AE_CH_LAYOUT_4_1; break;
    case  7: m_stdChLayout = AE_CH_LAYOUT_5_0; break;
    case  8: m_stdChLayout = AE_CH_LAYOUT_5_1; break;
    case  9: m_stdChLayout = AE_CH_LAYOUT_7_0; break;
    case 10: m_stdChLayout = AE_CH_LAYOUT_7_1; break;
  }
#endif
  // force optical/coax to 2.0 output channels
  if (!m_rawPassthrough && g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958)
    m_stdChLayout = AE_CH_LAYOUT_2_0;

  // setup the desired format
  m_format.m_channelLayout = CAEChannelInfo(m_stdChLayout);

  // if there is an audio resample rate set, use it.
  if (g_advancedSettings.m_audioResample && !m_rawPassthrough)
  {
    sampleRate = g_advancedSettings.m_audioResample;
    CLog::Log(LOGINFO, "CCoreAudioAE::passthrough - Forcing samplerate to %d", sampleRate);
  }

  if (m_rawPassthrough)
  {
    switch (rawDataFormat)
    {
      case AE_FMT_AC3:
      case AE_FMT_DTS:
        m_format.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_2_0);
        m_format.m_sampleRate   = 48000;
        m_format.m_dataFormat   = AE_FMT_S16NE;
        break;
      case AE_FMT_EAC3:
        m_format.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_2_0);
        m_format.m_sampleRate   = 192000;
        m_format.m_dataFormat   = AE_FMT_S16NE;
        break;
      case AE_FMT_DTSHD:
      case AE_FMT_TRUEHD:
        m_format.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_7_1);
        m_format.m_sampleRate   = 192000;
        m_format.m_dataFormat   = AE_FMT_S16NE;
        break;
      case AE_FMT_LPCM:
        m_format.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_7_1);
        m_format.m_sampleRate   = sampleRate;
        m_format.m_dataFormat   = AE_FMT_FLOAT;
        break;
      default:
        break;
    }
  }
  else
  {
    m_format.m_sampleRate       = sampleRate;
    m_format.m_channelLayout    = CAEChannelInfo(m_stdChLayout);
    m_format.m_dataFormat       = AE_FMT_FLOAT;
  }

  m_format.m_encodedRate = 0;

  if (m_outputDevice.empty())
    m_outputDevice = "default";

  AEAudioFormat initformat = m_format;

  // initialize audio hardware
  m_Initialized = HAL->Initialize(this, m_rawPassthrough, initformat, rawDataFormat, m_outputDevice, m_volume);

  unsigned int bps         = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_chLayoutCount          = m_format.m_channelLayout.Count();
  m_format.m_frameSize     = (bps>>3) * m_chLayoutCount; //initformat.m_frameSize;
  //m_format.m_frames        = (unsigned int)(((float)m_format.m_sampleRate / 1000.0f) * (float)DELAY_FRAME_TIME);
  //m_format.m_frameSamples  = m_format.m_frames * m_format.m_channelLayout.Count();

  if ((initformat.m_channelLayout.Count() != m_chLayoutCount) && !m_rawPassthrough)
  {
    /* readjust parameters. hardware didn't accept channel count*/
    CLog::Log(LOGINFO, "CCoreAudioAE::Initialize: Setup channels (%d) greater than possible hardware channels (%d).",
              m_chLayoutCount, initformat.m_channelLayout.Count());

    m_format.m_channelLayout = CAEChannelInfo(initformat.m_channelLayout);
    m_chLayoutCount          = m_format.m_channelLayout.Count();
    m_format.m_frameSize     = (bps>>3) * m_chLayoutCount; //initformat.m_frameSize;
    //m_format.m_frameSamples  = m_format.m_frames * m_format.m_channelLayout.Count();
  }

  CLog::Log(LOGINFO, "CCoreAudioAE::Initialize:");
  CLog::Log(LOGINFO, "  Output Device : %s", m_outputDevice.c_str());
  CLog::Log(LOGINFO, "  Sample Rate   : %d", m_format.m_sampleRate);
  CLog::Log(LOGINFO, "  Sample Format : %s", CAEUtil::DataFormatToStr(m_format.m_dataFormat));
  CLog::Log(LOGINFO, "  Channel Count : %d", m_chLayoutCount);
  CLog::Log(LOGINFO, "  Channel Layout: %s", ((std::string)m_format.m_channelLayout).c_str());
  CLog::Log(LOGINFO, "  Frame Size    : %d", m_format.m_frameSize);
  CLog::Log(LOGINFO, "  Volume Level  : %f", m_volume);
  CLog::Log(LOGINFO, "  Passthrough   : %d", m_rawPassthrough);

  CSingleLock soundLock(m_soundLock);
  StopAllSounds();

  // re-init sounds and unlock
  for (SoundList::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    (*itt)->Initialize();

  soundLock.Leave();

  // if we are not in m_rawPassthrough reinit the streams
  if (!m_rawPassthrough)
  {
    /* re-init streams */
    streamLock.Enter();
    for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->Initialize();
    streamLock.Leave();
  }

  return m_Initialized;
}

void CCoreAudioAE::Deinitialize()
{
  if (!m_Initialized)
    return;

  // close all open converters
  CSingleLock streamLock(m_streamLock);
  for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end();++itt)
    (*itt)->CloseConverter();
  streamLock.Leave();

  m_Initialized = false;

  CSingleLock callbackLock(m_callbackLock);

  /*
  while(m_callbackRunning)
    Sleep(100);
  */

  HAL->Deinitialize();
}

void CCoreAudioAE::OnSettingsChange(const std::string& setting)
{
  if (setting == "audiooutput.dontnormalizelevels")
  {
    // re-init streams remapper
    CSingleLock streamLock(m_streamLock);
    for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->InitializeRemap();
    streamLock.Leave();
  }

  if (setting == "audiooutput.passthroughdevice" ||
      setting == "audiooutput.custompassthrough" ||
      setting == "audiooutput.audiodevice"       ||
      setting == "audiooutput.customdevice"      ||
      setting == "audiooutput.mode"              ||
      setting == "audiooutput.ac3passthrough"    ||
      setting == "audiooutput.dtspassthrough"    ||
      setting == "audiooutput.channellayout"     ||
      setting == "audiooutput.multichannellpcm")
  {
    // only reinit the engine if we not
    // suspended (resume will initialize
    // us again in that case)
    if (!m_isSuspended)
      Initialize();
  }
}

unsigned int CCoreAudioAE::GetSampleRate()
{
  return m_format.m_sampleRate;
}

unsigned int CCoreAudioAE::GetEncodedSampleRate()
{
  return m_format.m_encodedRate;
}

CAEChannelInfo CCoreAudioAE::GetChannelLayout()
{
  return m_format.m_channelLayout;
}

unsigned int CCoreAudioAE::GetChannelCount()
{
  return m_chLayoutCount;
}

enum AEDataFormat CCoreAudioAE::GetDataFormat()
{
  return m_format.m_dataFormat;
}

AEAudioFormat CCoreAudioAE::GetAudioFormat()
{
  return m_format;
}

double CCoreAudioAE::GetDelay()
{
  return HAL->GetDelay();
}

float CCoreAudioAE::GetVolume()
{
  return m_volume;
}

void CCoreAudioAE::SetVolume(float volume)
{
  if (m_rawPassthrough)
    return;

  m_volume = volume;
  // track volume if we are not muted
  // we need this because m_volume is init'ed via
  // SetVolume and need to also init m_volumeBeforeMute.
  if (!m_muted)
    m_volumeBeforeMute = volume;

  HAL->SetVolume(m_volume);
}

void CCoreAudioAE::SetMute(const bool enabled)
{
  m_muted = enabled;
  if(m_muted)
  {
    m_volumeBeforeMute = m_volume;
    SetVolume(VOLUME_MINIMUM);
  }
  else
  {
    SetVolume(m_volumeBeforeMute);
  }
}

bool CCoreAudioAE::IsMuted()
{
  return m_muted;
}

bool CCoreAudioAE::IsSuspended()
{
  return m_isSuspended;
}

void CCoreAudioAE::SetSoundMode(const int mode)
{
  m_soundMode = mode;

  /* stop all currently playing sounds if they are being turned off */
  if (mode == AE_SOUND_OFF || (mode == AE_SOUND_IDLE && m_streamsPlaying))
    StopAllSounds();
}

bool CCoreAudioAE::SupportsRaw()
{
  return true;
}

CCoreAudioAEHAL* CCoreAudioAE::GetHAL()
{
  return HAL;
}

IAEStream* CCoreAudioAE::MakeStream(enum AEDataFormat dataFormat,
  unsigned int sampleRate, unsigned int encodedSamplerate, CAEChannelInfo channelLayout, unsigned int options)
{
  // if we are suspended we don't
  // want anyone to mess with us
  if (m_isSuspended)
    return NULL;

  CAEChannelInfo channelInfo(channelLayout);
  CLog::Log(LOGINFO, "CCoreAudioAE::MakeStream - %s, %u, %u, %s",
    CAEUtil::DataFormatToStr(dataFormat), sampleRate, encodedSamplerate, ((std::string)channelInfo).c_str());

  CSingleLock streamLock(m_streamLock);
  //bool wasEmpty = m_streams.empty();
  CCoreAudioAEStream *stream = new CCoreAudioAEStream(dataFormat, sampleRate, encodedSamplerate, channelLayout, options);
  m_streams.push_back(stream);
  streamLock.Leave();

  Stop();

  if (m_Initialized)
  {
    Deinitialize();
    m_Initialized = OpenCoreAudio(sampleRate, COREAUDIO_IS_RAW(dataFormat), dataFormat);
  }

  /* if the stream was not initialized, do it now */
  if (!stream->IsValid())
    stream->Initialize();

  Start();

  m_streamsPlaying = true;

  return stream;
}

IAEStream* CCoreAudioAE::FreeStream(IAEStream *stream)
{
  CSingleLock streamLock(m_streamLock);
  /* ensure the stream still exists */
  for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); )
  {
    CCoreAudioAEStream *del = *itt;
    if (*itt == stream)
    {
      itt = m_streams.erase(itt);
      delete (CCoreAudioAEStream *)stream;
    }
    else if (del->IsDestroyed())
    {
      itt = m_streams.erase(itt);
      delete del;
    }
    else
    {
      ++itt;
    }

  }
  m_streamsPlaying = !m_streams.empty();

  streamLock.Leave();

  // When we have been in passthrough mode and are not suspended,
  // reinit the hardware to come back to anlog out
  if (/*m_streams.empty() || */ m_rawPassthrough && !m_isSuspended)
  {
    CLog::Log(LOGINFO, "CCoreAudioAE::FreeStream Reinit, no streams left" );
    Initialize();
  }

  return NULL;
}

void CCoreAudioAE::PlaySound(IAESound *sound)
{
  if (m_soundMode == AE_SOUND_OFF || (m_soundMode == AE_SOUND_IDLE && m_streamsPlaying) || m_isSuspended)
    return;

  float *samples = ((CCoreAudioAESound*)sound)->GetSamples();
  if (!samples && !m_Initialized)
    return;

  /* add the sound to the play list */
  CSingleLock soundSampleLock(m_soundSampleLock);
  SoundState ss = {
    ((CCoreAudioAESound*)sound),
    samples,
    ((CCoreAudioAESound*)sound)->GetSampleCount()
  };
  m_playing_sounds.push_back(ss);
}

void CCoreAudioAE::StopSound(IAESound *sound)
{
  CSingleLock lock(m_soundSampleLock);
  for (SoundStateList::iterator itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound)
    {
      (*itt).owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
    }
    else
      ++itt;
  }
}

IAESound *CCoreAudioAE::MakeSound(const std::string& file)
{
  // we don't make sounds
  // when suspended
  if (m_isSuspended)
    return NULL;

  CSingleLock soundLock(m_soundLock);

  // first check if we have the file cached
  for (SoundList::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
  {
    if ((*itt)->GetFileName() == file)
      return *itt;
  }

  CCoreAudioAESound *sound = new CCoreAudioAESound(file);
  if (!sound->Initialize())
  {
    delete sound;
    return NULL;
  }

  m_sounds.push_back(sound);
  return sound;
}

void CCoreAudioAE::FreeSound(IAESound *sound)
{
  if (!sound)
    return;

  sound->Stop();
  CSingleLock soundLock(m_soundLock);
  for (SoundList::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }

  delete (CCoreAudioAESound*)sound;
}

void CCoreAudioAE::StopAllSounds()
{
  CSingleLock lock(m_soundSampleLock);
  while (!m_playing_sounds.empty())
  {
    SoundState *ss = &(*m_playing_sounds.begin());
    m_playing_sounds.pop_front();
    ss->owner->ReleaseSamples();
  }
}

void CCoreAudioAE::MixSounds(float *buffer, unsigned int samples)
{
  if (!m_Initialized)
    return;

  SoundStateList::iterator itt;

  CSingleLock lock(m_soundSampleLock);
  for (itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt);

    // no more frames, so remove it from the list
    if (ss->sampleCount == 0)
    {
      ss->owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
      continue;
    }

    unsigned int mixSamples = std::min(ss->sampleCount, samples);
    float volume = ss->owner->GetVolume();

    for (unsigned int i = 0; i < mixSamples; ++i)
      buffer[i] = (buffer[i] + (ss->samples[i] * volume));

    ss->sampleCount -= mixSamples;
    ss->samples     += mixSamples;

    ++itt;
  }
}

void CCoreAudioAE::GarbageCollect()
{
}

void CCoreAudioAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  if (m_isSuspended)
    return;

  HAL->EnumerateOutputDevices(devices, passthrough);
}

void CCoreAudioAE::Start()
{
  if (!m_Initialized)
    return;

  HAL->Start();
}

void CCoreAudioAE::Stop()
{
  if (!m_Initialized)
    return;

  HAL->Stop();
}

bool CCoreAudioAE::Suspend()
{
  CLog::Log(LOGDEBUG, "CCoreAudioAE::Suspend - Suspending AE processing");
  m_isSuspended = true;
  // stop all gui sounds
  StopAllSounds();
  // stop the CA thread
  Stop();

  return true;
}

bool CCoreAudioAE::Resume()
{
  // fire up the engine again
  bool ret = Initialize();
  CLog::Log(LOGDEBUG, "CCoreAudioAE::Resume - Resuming AE processing");
  m_isSuspended = false;

  return ret;
}

//***********************************************************************************************
// Rendering Methods
//***********************************************************************************************
OSStatus CCoreAudioAE::OnRender(AudioUnitRenderActionFlags *actionFlags,
  const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  UInt32 frames = inNumberFrames;

  unsigned int rSamples = frames * m_chLayoutCount;
  int size = frames * m_format.m_frameSize;
  //int size = frames * HAL->m_BytesPerFrame;

  for (UInt32 i = 0; i < ioData->mNumberBuffers; i++)
    bzero(ioData->mBuffers[i].mData, ioData->mBuffers[i].mDataByteSize);

  if (!m_Initialized)
  {
    m_callbackRunning = false;
    return noErr;
  }

  CSingleLock callbackLock(m_callbackLock);

  m_callbackRunning = true;

  /*
  CSingleLock streamLock(m_streamLock);
  // Remove any destroyed stream
  if (!m_streams.empty())
  {
    for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end();)
    {
      CCoreAudioAEStream *stream = *itt;

      if (stream->IsDestroyed())
      {
        itt = m_streams.erase(itt);
        delete stream;
        continue;
      }
      ++itt;
    }
  }
  streamLock.Leave();
  */

  // when not in passthrough output mix sounds
  if (!m_rawPassthrough && m_soundMode != AE_SOUND_OFF)
  {
    MixSounds((float *)ioData->mBuffers[0].mData, rSamples);
    ioData->mBuffers[0].mDataByteSize = size;
    if (!size && actionFlags)
      *actionFlags |=  kAudioUnitRenderAction_OutputIsSilence;
  }

  m_callbackRunning = false;

  return noErr;
}

// Static Callback from AudioUnit
OSStatus CCoreAudioAE::Render(AudioUnitRenderActionFlags* actionFlags,
  const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList)
{
  OSStatus ret = OnRender(actionFlags, pTimeStamp, busNumber, frameCount, pBufList);

  return ret;
}


