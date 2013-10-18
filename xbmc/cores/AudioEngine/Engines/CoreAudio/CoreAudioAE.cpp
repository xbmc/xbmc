/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "CoreAudioAEStream.h"
#include "CoreAudioAESound.h"
#include "Application.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/MathUtils.h"
#include "threads/SystemClock.h"

#define DELAY_FRAME_TIME  20
#define BUFFERSIZE        16416

// on darwin when the devicelist changes
// reinit by calling opencoreaudio with the last engine parameters 
// (this will fallback to default
// device when our current output device vanishes
// and on the other hand will go back to that device
// if it re-appears).
#if defined(TARGET_DARWIN_OSX)
OSStatus deviceChangedCB( AudioObjectID                       inObjectID,
                          UInt32                              inNumberAddresses,
                          const AudioObjectPropertyAddress    inAddresses[],
                          void*                               inClientData)
{
  CCoreAudioAE *pEngine = (CCoreAudioAE *)inClientData;
  if (pEngine->GetHAL())
  {
    pEngine->AudioDevicesChanged();
    CLog::Log(LOGDEBUG, "CCoreAudioAE - audiodevicelist changed!");
  }
  return noErr;
}

void RegisterDeviceChangedCB(bool bRegister, void *ref)
{
  OSStatus ret = noErr;
  const AudioObjectPropertyAddress inAdr = 
  {  
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster 
  };
  
  if (bRegister)
    ret = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &inAdr, deviceChangedCB, ref);
  else
    ret = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &inAdr, deviceChangedCB, ref);

  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioAE::Deinitialize - error %s a listener callback for device changes!", bRegister?"attaching":"removing");   
}
#else//ios
void RegisterDeviceChangedCB(bool bRegister, void *ref){}
#endif

CCoreAudioAE::CCoreAudioAE() :
  m_Initialized        (false         ),
  m_callbackRunning    (false         ),
  m_lastStreamFormat   (AE_FMT_INVALID),
  m_lastChLayoutCount  (0             ),
  m_lastSampleRate     (0             ),
  m_chLayoutCount      (0             ),
  m_rawPassthrough     (false         ),
  m_volume             (1.0f          ),
  m_volumeBeforeMute   (1.0f          ),
  m_muted              (false         ),
  m_soundMode          (AE_SOUND_OFF  ),
  m_streamsPlaying     (false         ),
  m_isSuspended        (false         ),
  m_softSuspend        (false         ),
  m_softSuspendTimer   (0             )
{
  HAL = new CCoreAudioAEHAL;
  
  RegisterDeviceChangedCB(true, this);
}

CCoreAudioAE::~CCoreAudioAE()
{
  RegisterDeviceChangedCB(false, this);
  Shutdown();
}

void CCoreAudioAE::Shutdown()
{
  CSingleLock engineLock(m_engineLock);

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

void CCoreAudioAE::AudioDevicesChanged()
{
  if (!m_Initialized)
    return;

  // give CA a bit time to realise that maybe the 
  // default device might have changed now - else
  // OpenCoreAudio might open the old default device
  // again (yeah that really is the case - duh)
  Sleep(500);
  CSingleLock engineLock(m_engineLock);

  // re-check initialized since it can have changed when we waited and grabbed the lock
  if (!m_Initialized)
    return;
  OpenCoreAudio(m_lastSampleRate, COREAUDIO_IS_RAW(m_lastStreamFormat), m_lastStreamFormat, m_transcode);
}

bool CCoreAudioAE::Initialize()
{
  CSingleLock engineLock(m_engineLock);

  Stop();

  Deinitialize();

  bool ret = OpenCoreAudio(44100, false, AE_FMT_FLOAT, false);
  m_lastSampleRate = 44100;
  m_lastStreamFormat = AE_FMT_FLOAT;

  Start();

  return ret;
}

bool CCoreAudioAE::OpenCoreAudio(unsigned int sampleRate, bool forceRaw,
  enum AEDataFormat rawDataFormat, bool forceTranscode)
{
  unsigned int maxChannelCountInStreams = 0;
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

    if (stream->GetChannelCount() > maxChannelCountInStreams)
        maxChannelCountInStreams = stream->GetChannelCount();

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

  m_transcode = forceTranscode;
  
  if (m_transcode)
    CLog::Log(LOGINFO, "CCoreAudioAE::OpenCoreAudio - transcode to ac3 enabled");
  
  std::string m_outputDevice =  CSettings::Get().GetString("audiooutput.audiodevice");

  // on iOS devices we set fixed to two channels.
  m_stdChLayout = AE_CH_LAYOUT_2_0;
#if defined(TARGET_DARWIN_OSX)
  switch (CSettings::Get().GetInt("audiooutput.channels"))
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

  // setup the desired format
  m_format.m_channelLayout = CAEChannelInfo(m_stdChLayout);

  // if there is an audio resample rate set, use it.
  if (CSettings::Get().GetInt("audiooutput.config") == AE_CONFIG_FIXED && !m_rawPassthrough)
  {
    sampleRate = CSettings::Get().GetInt("audiooutput.samplerate");
    CLog::Log(LOGINFO, "CCoreAudioAE::passthrough - Forcing samplerate to %d", sampleRate);
  }

  if (m_rawPassthrough && !m_transcode)
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
        // audio midi setup can be setup to 2.0 or 7.1
        // if we have the number of max channels from streams we use that for
        // selecting either 2.0 or 7.1 setup depending on that.
        // This allows DPII modes on amps for enhancing stereo sound
        // (when switching to 7.1 - all 8 channels will be pushed out preventing most amps
        // to switch to DPII mode)
        if (maxChannelCountInStreams == 1 || maxChannelCountInStreams == 2)
          m_format.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_2_0);
        else
          m_format.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_7_1);
        m_format.m_sampleRate   = sampleRate;
        m_format.m_dataFormat   = AE_FMT_FLOAT;
        break;
      default:
        break;
    }
  }
  else if (m_transcode)
  {
    // transcode is to ac3 only, do we copy the ac3 settings from above
    m_format.m_channelLayout    = CAEChannelInfo(AE_CH_LAYOUT_2_0);
    m_format.m_sampleRate       = 48000;
    m_format.m_dataFormat       = AE_FMT_S16NE;
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
  m_Initialized = HAL->Initialize(this, m_rawPassthrough || m_transcode, initformat, m_transcode ? AE_FMT_AC3 : rawDataFormat, m_outputDevice, m_volume);

  unsigned int bps         = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_chLayoutCount          = m_format.m_channelLayout.Count();
  m_format.m_frameSize     = (bps>>3) * m_chLayoutCount; //initformat.m_frameSize;
  //m_format.m_frames        = (unsigned int)(((float)m_format.m_sampleRate / 1000.0f) * (float)DELAY_FRAME_TIME);
  //m_format.m_frameSamples  = m_format.m_frames * m_format.m_channelLayout.Count();

  if ((initformat.m_channelLayout.Count() != m_chLayoutCount) && !m_rawPassthrough && !m_transcode)
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
  CLog::Log(LOGINFO, "  Transcode     : %d", m_transcode);

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
      setting == "audiooutput.ac3passthrough"    ||
      setting == "audiooutput.eac3passthrough"   ||
      setting == "audiooutput.dtspassthrough"    ||
      setting == "audiooutput.channels"          ||
      setting == "audiooutput.samplerate"        ||
      setting == "audiooutput.config"            ||
      setting == "audiooutput.passthrough"        )
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

bool CCoreAudioAE::SupportsRaw(AEDataFormat format)
{
  switch(format)
  {
    case AE_FMT_AC3:
    case AE_FMT_DTS:
    case AE_FMT_EAC3:
    case AE_FMT_LPCM:
      return true;
    default:
      return false;
  }
}

bool CCoreAudioAE::IsSettingVisible(const std::string &settingId)
{
  if (settingId == "audiooutput.samplerate")
  {
    if (CSettings::Get().GetInt("audiooutput.config") == AE_CONFIG_FIXED)
      return true;
    else
      return false;
  }

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
  if (m_isSuspended && !m_softSuspend)
#if defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV)
    Resume();
#else
    return NULL;
#endif

  CAEChannelInfo channelInfo(channelLayout);
  CLog::Log(LOGINFO, "CCoreAudioAE::MakeStream - %s, %u, %u, %s",
    CAEUtil::DataFormatToStr(dataFormat), sampleRate, encodedSamplerate, ((std::string)channelInfo).c_str());

  bool multichannelpcm = CSettings::Get().GetInt("audiooutput.channels") > AE_CH_LAYOUT_2_0; //if more then 2 channels are set - assume lpcm capability
#if defined(TARGET_DARWIN_IOS)
  multichannelpcm = false;
#endif
  // determine if we need to transcode this audio
  // when we're called, we'll either get the audio in an encoded form (COREAUDIO_IS_RAW==true)
  // that we can passthrough based on user options, or we'll get it unencoded
  // if it's unencoded, and is 5.1, we'll transcode it to AC3 if possible
  bool transcode = CSettings::Get().GetBool("audiooutput.passthrough") && CSettings::Get().GetBool("audiooutput.ac3passthrough") && !multichannelpcm &&
                   !COREAUDIO_IS_RAW(dataFormat) &&
                  (channelInfo.Count() == 6);
  
  CCoreAudioAEStream *stream = new CCoreAudioAEStream(dataFormat, sampleRate, encodedSamplerate, channelLayout, options, transcode);
  CSingleLock streamLock(m_streamLock);
  m_streams.push_back(stream);
  streamLock.Leave();

  if ((options & AESTREAM_PAUSED) == 0)
    Stop();

  // reinit the engine if pcm format changes or always on raw format or always on transcode
  if (m_Initialized && ( m_lastStreamFormat != dataFormat ||
                         m_lastChLayoutCount != channelLayout.Count() ||
                         m_lastSampleRate != sampleRate ||
                         COREAUDIO_IS_RAW(dataFormat) ||
                         transcode))
  {
    CSingleLock engineLock(m_engineLock);
    Stop();
    Deinitialize();
    m_Initialized = OpenCoreAudio(sampleRate, COREAUDIO_IS_RAW(dataFormat), dataFormat, transcode);
    m_lastStreamFormat = dataFormat;
    m_lastChLayoutCount = channelLayout.Count();
    m_lastSampleRate = sampleRate;
  }

  /* if the stream was not initialized, do it now */
  if (!stream->IsValid())
    stream->Initialize();

  if ((options & AESTREAM_PAUSED) == 0)  
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
  if (m_soundMode == AE_SOUND_OFF || (m_soundMode == AE_SOUND_IDLE && m_streamsPlaying) || (m_isSuspended && !m_softSuspend))
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
#if defined(TARGET_DARWIN_OSX)
  if (CSettings::Get().GetBool("audiooutput.streamsilence"))
    return;
  
  if (!m_streamsPlaying && m_playing_sounds.empty())
  {
    if (!m_softSuspend)
    {
      m_softSuspend = true;
      m_softSuspendTimer = XbmcThreads::SystemClockMillis() + 10000; //10.0 second delay for softSuspend
    }
  }
  else
  {
    if (m_isSuspended)
    {
      CSingleLock engineLock(m_engineLock);
      CLog::Log(LOGDEBUG, "CCoreAudioAE::GarbageCollect - Acquire CA HAL.");
      Start();
      m_isSuspended = false;
    }
    m_softSuspend = false;
  }
  
  unsigned int curSystemClock = XbmcThreads::SystemClockMillis();
  if (!m_isSuspended && m_softSuspend && curSystemClock > m_softSuspendTimer)
  {
    Suspend();// locks m_engineLock internally
    CLog::Log(LOGDEBUG, "CCoreAudioAE::GarbageCollect - Release CA HAL.");
  }
#endif // TARGET_DARWIN_OSX
}

void CCoreAudioAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  if (m_isSuspended && !m_softSuspend)
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
  CSingleLock engineLock(m_engineLock);
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


