/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 Fix 7.1 channel order in CoreAudio (mac).
 
 To activate 7.1 audio (using either HDMI or DisplayPort); make sure to first launch the 
 Audio MIDI Setup in /Application/Utilities and configure HDMI audio as 8 channels-24 bits (the default is just stereo). 
 It is recommended to disable DTS and AC3 passthrough as it would reset the hdmi audio in two channels mode, 
 which would break future multi-channels playback.
 */

#include "system.h"

#include "CoreAudioAE.h"
#include "CoreAudioAEStream.h"
#include "CoreAudioAESound.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "AEUtil.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "MathUtils.h"
#include "utils/EndianSwap.h"

#define DELAY_FRAME_TIME  20
#define BUFFERSIZE        16416
#define SPEAKER_COUNT     9

#define INT16_MAX         0x7fff

static inline int safeRound(double f)
{
  /* if the value is larger then we can handle, then clamp it */
  if (f >= INT_MAX) return INT_MAX;
  if (f <= INT_MIN) return INT_MIN;
  
  /* if the value is out of the MathUtils::round_int range, then round it normally */
  if (f <= static_cast<double>(INT_MIN / 2) - 1.0 || f >= static_cast <double>(INT_MAX / 2) + 1.0)
    return floor(f+0.5);
  
  return MathUtils::round_int(f);
}

static enum AEChannel CoreAudioChannelMap[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

CCoreAudioAE::CCoreAudioAE() :
  m_Initialized(false),
  m_rawPassthrough(false),
  m_RemapChannelLayout(NULL),
  m_volume(1.0f),
  m_guiSoundWhilePlayback(true),
  m_EngineLock(false),
  m_convertFn(NULL)
{
  /* Allocate internal buffers */
  m_OutputBuffer      = (float *)_aligned_malloc(BUFFERSIZE, 16);
  m_StreamBuffer      = (uint8_t *)_aligned_malloc(BUFFERSIZE, 16);
  m_StreamBufferSize  = m_OutputBufferSize = BUFFERSIZE;
  
  m_Mutex             = SDL_CreateMutex();
  m_MutexLockEngine   = SDL_CreateMutex();
  
  m_volume            = g_settings.m_fVolumeLevel;
  
#ifdef __arm__
  HAL = new CCoreAudioAEHALIOS;
#else
  HAL = new CCoreAudioAEHALOSX;
#endif
 
  m_Use16BitAudio     = false; //g_sysinfo.IsAppleTV();
}

CCoreAudioAE::~CCoreAudioAE()
{
  Stop();
    
  Deinitialize();
  
  /* free the streams */
  while(!m_streams.empty())
  {
    CCoreAudioAEStream *s = m_streams.front();
    /* note: the stream will call RemoveStream via it's dtor */
    RemoveStream(s);
    delete s;
  }
  
  while(!m_sounds.empty())
  {
    CCoreAudioAESound *s = m_sounds.front();
    m_sounds.pop_front();
    delete s;
  }
    
  if(m_OutputBuffer)
    _aligned_free(m_OutputBuffer);
  if(m_StreamBuffer)
    _aligned_free(m_StreamBuffer);

  if (m_Mutex)
    SDL_DestroyMutex(m_Mutex);  

  if (m_MutexLockEngine)
    SDL_DestroyMutex(m_MutexLockEngine);  

#ifndef __arm__
  CCoreAudioHardware::ResetAudioDevices();
#endif
  
  delete HAL;
}

void CCoreAudioAE::LockEngine()
{
  SDL_mutexP(m_MutexLockEngine);
  m_EngineLock = true;
  SDL_mutexV(m_MutexLockEngine);
}

void CCoreAudioAE::UnlockEngine()
{
  SDL_mutexP(m_MutexLockEngine);
  m_EngineLock = false;
  SDL_mutexV(m_MutexLockEngine);
}

bool CCoreAudioAE::Initialize()
{
  Stop();
  
  SDL_mutexP(m_Mutex);

  Deinitialize();
  
  bool ret = OpenCoreAudio();
 
  SDL_mutexV(m_Mutex);
  
  Start();
  
  return ret;
}

bool CCoreAudioAE::OpenCoreAudio(unsigned int sampleRate, bool forceRaw, enum AEDataFormat rawFormat)
{
  m_Initialized = false;
  
  /* override the sample rate based on the oldest stream if there is one */
  if (!m_streams.empty())
    sampleRate = m_streams.front()->GetSampleRate();
    
  std::list<CCoreAudioAEStream*>::iterator itt_streams;
  /* remove any deleted streams */
  /*
  for(itt_streams = m_streams.begin(); itt_streams != m_streams.end();)
  {
    if ((*itt_streams)->IsDestroyed() && !(*itt_streams)->IsBusy())
    {
      CCoreAudioAEStream *stream = *itt_streams;
      itt_streams = m_streams.erase(itt_streams);
      RemoveStream(stream);
      delete stream;
      continue;
    }
    ++itt_streams;
  }
  */
  /* Remove all playing sounds */
  std::list<SoundState>::iterator itt_sound;
  for(itt_sound = m_playing_sounds.begin(); itt_sound != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt_sound);
    
    ss->owner->ReleaseSamples();
    itt_sound = m_playing_sounds.erase(itt_sound);
  }
  
  std::list<CCoreAudioAESound*>::iterator itt_sounds;
  for(itt_sounds = m_sounds.begin(); itt_sounds != m_sounds.end(); ++itt_sounds)
    (*itt_sounds)->Lock();
  
  if (forceRaw)
    m_rawPassthrough = true;
  else
    m_rawPassthrough = !m_streams.empty() && m_streams.front()->IsRaw();
      
  if (m_rawPassthrough) CLog::Log(LOGINFO, "CCoreAudioAE::OpenCoreAudio - RAW passthrough enabled");

  CStdString m_outputDevice =  g_guiSettings.GetString("audiooutput.audiodevice");
  
  m_guiSoundWhilePlayback = g_guiSettings.GetBool("audiooutput.guisoundwhileplayback");

  enum AEStdChLayout m_stdChLayout = AE_CH_LAYOUT_2_0;
  switch(g_guiSettings.GetInt("audiooutput.channellayout"))
  {
    default:
    case  0: m_stdChLayout = AE_CH_LAYOUT_2_0; break; /* dont alow 1_0 output */
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

  /* setup the desired format */
  m_format.m_channelLayout = CAEUtil::GetStdChLayout(m_stdChLayout);    
 
  /* if there is an audio resample rate set, use it. */
  if (g_advancedSettings.m_audioResample && !m_rawPassthrough)
  {
    sampleRate = g_advancedSettings.m_audioResample;
    CLog::Log(LOGINFO, "CCoreAudioAE::passthrough - Forcing samplerate to %d", sampleRate);
  }

  if(m_rawPassthrough)
  {
    switch(rawFormat)
    {
      case AE_FMT_AC3:
      case AE_FMT_DTS:
        m_format.m_channelCount = 2;
        m_format.m_sampleRate   = 48000;        
        break;
      case AE_FMT_EAC3:
        m_format.m_channelCount = 2;
        m_format.m_sampleRate   = 192000;
        break;
    }
    m_format.m_dataFormat   = AE_FMT_S16NE;
  }
  else 
  {
    m_format.m_sampleRate       = sampleRate;
    m_format.m_channelCount     = CAEUtil::GetChLayoutCount(m_format.m_channelLayout);

    if(m_Use16BitAudio)
    {
      m_format.m_dataFormat     = AE_FMT_S16NE;
      m_convertFn               = CAEConvert::FrFloat(m_format.m_dataFormat);
    }
    else 
    {      
      m_format.m_dataFormat     = AE_FMT_FLOAT;
    }


  }

  if (m_outputDevice.IsEmpty())
    m_outputDevice = "default";
  
  AEAudioFormat initformat = m_format;
  m_Initialized = HAL->Initialize(this, m_rawPassthrough, initformat, m_outputDevice);
  
  m_format.m_frameSize     = initformat.m_frameSize;
  m_format.m_frames        = (unsigned int)(((float)m_format.m_sampleRate / 1000.0f) * (float)DELAY_FRAME_TIME);
  m_format.m_frameSamples  = m_format.m_frames * m_format.m_channelCount;
  
  if((initformat.m_channelCount != m_format.m_channelCount) && !m_rawPassthrough)
  {
    /* readjust parameters. hardware didn't accept channel count*/
    CLog::Log(LOGINFO, "CCoreAudioAE::Initialize: Setup channels (%d) greater than possible hardware channels (%d).",
              m_format.m_channelCount, initformat.m_channelCount);
    
    m_format.m_channelCount  = initformat.m_channelCount;
    m_format.m_channelLayout = CAEUtil::GetStdChLayout((AEStdChLayout)m_format.m_channelCount);
    m_format.m_frameSamples  = m_format.m_frames * m_format.m_channelCount;
  }

  CLog::Log(LOGINFO, "CCoreAudioAE::Initialize:");
  CLog::Log(LOGINFO, "  Output Device : %s", m_outputDevice.c_str());
  CLog::Log(LOGINFO, "  Sample Rate   : %d", m_format.m_sampleRate);
  CLog::Log(LOGINFO, "  Sample Format : %s", CAEUtil::DataFormatToStr(m_format.m_dataFormat));
  CLog::Log(LOGINFO, "  Channel Count : %d", m_format.m_channelCount);
  CLog::Log(LOGINFO, "  Channel Layout: %s", CAEUtil::GetChLayoutStr(m_format.m_channelLayout).c_str());
  CLog::Log(LOGINFO, "  Frame Size    : %d", m_format.m_frameSize);
  CLog::Log(LOGINFO, "  Volume Level  : %f", m_volume);
  CLog::Log(LOGINFO, "  Passthrough   : %d", m_rawPassthrough);
  
  if(m_format.m_channelCount > SPEAKER_COUNT)
    m_format.m_channelCount = SPEAKER_COUNT;

  if(m_RemapChannelLayout)
    delete[] m_RemapChannelLayout;
  
  m_RemapChannelLayout = new enum AEChannel[m_format.m_channelCount + 1];
  memcpy(m_RemapChannelLayout, CoreAudioChannelMap, m_format.m_channelCount * sizeof(enum AEChannel));
  m_RemapChannelLayout[m_format.m_channelCount] = AE_CH_NULL;
    
  SetVolume(m_volume);    
  
  /* if we in raw passthrough, we are finished */
  if (m_rawPassthrough)
  {
    /* re-init sounds and unlock */
    for(itt_sounds = m_sounds.begin(); itt_sounds != m_sounds.end(); ++itt_sounds)
    {
      (*itt_sounds)->UnLock();
    }
  }
  else 
  {
    /* re-init streams */
    for(itt_streams = m_streams.begin(); itt_streams != m_streams.end(); ++itt_streams)
      (*itt_streams)->Initialize(m_format);  
    
    /* re-init sounds and unlock */
    for(itt_sounds = m_sounds.begin(); itt_sounds != m_sounds.end(); ++itt_sounds)
    {
      (*itt_sounds)->Initialize(m_format);
      (*itt_sounds)->UnLock();
    }
  }
  
  return m_Initialized;
}

void CCoreAudioAE::Deinitialize()
{
  if(!m_Initialized)
    return;
  
  HAL->Deinitialize();
    
  if(m_RemapChannelLayout)
    delete[] m_RemapChannelLayout;
  m_RemapChannelLayout = NULL;
 
  m_Initialized = false;
  
  CLog::Log(LOGINFO, "CCoreAudioAE::Deinitialize: Audio device has been closed.");
}

void CCoreAudioAE::OnSettingsChange(CStdString setting)
{
  if (setting == "audiooutput.dontnormalizelevels")
  {
    /* re-init streams reampper */
    SDL_mutexP(m_Mutex);
    
    std::list<CCoreAudioAEStream*>::iterator itt;
    for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->InitializeRemap();
    SDL_mutexV(m_Mutex);  
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
    Initialize();
  }

  if (setting =="audiooutput.guisoundwhileplayback")
    m_guiSoundWhilePlayback = g_guiSettings.GetBool("audiooutput.guisoundwhileplayback");
}

unsigned int CCoreAudioAE::GetSampleRate()
{
  return m_format.m_sampleRate;
}

AEChLayout CCoreAudioAE::GetChannelLayout()
{
  return m_format.m_channelLayout;
}

unsigned int CCoreAudioAE::GetChannelCount()
{
  return m_format.m_channelCount;
}

enum AEDataFormat CCoreAudioAE::GetDataFormat()
{
  return m_format.m_dataFormat;
}

AEAudioFormat CCoreAudioAE::GetAudioFormat()
{
  return m_format;
}

float CCoreAudioAE::GetDelay()
{  
  return HAL->GetDelay();
}

float CCoreAudioAE::GetVolume()
{
  return m_volume;
}

void CCoreAudioAE::SetVolume(float volume)
{
  g_settings.m_fVolumeLevel = volume;
  m_volume = volume;
}

bool CCoreAudioAE::SupportsRaw()
{
  return true;
}

IAEStream *CCoreAudioAE::GetStream(enum AEDataFormat dataFormat, 
                                   unsigned int sampleRate, 
                                   unsigned int channelCount, 
                                   AEChLayout channelLayout, 
                                   unsigned int options/* = 0 */)
{
  CLog::Log(LOGINFO, "CCoreAudioAE::GetStream - %s, %u, %u, %s",
            CAEUtil::DataFormatToStr(dataFormat),
            sampleRate,
            channelCount,
            CAEUtil::GetChLayoutStr(channelLayout).c_str()
            );

  SDL_mutexV(m_MutexLockEngine);

  m_EngineLock = true;
  
  Stop();
  
  SDL_mutexP(m_Mutex);
  
  bool wasEmpty = m_streams.empty();
  
  CCoreAudioAEStream *stream = new CCoreAudioAEStream(dataFormat, sampleRate, channelCount, channelLayout, options);
  
  m_streams.push_back(stream);
  
  if(COREAUDIO_IS_RAW(dataFormat))
  {
    Deinitialize();
    m_Initialized = OpenCoreAudio(sampleRate, true, dataFormat);
  }
  else if (wasEmpty || m_rawPassthrough)
  {
    Deinitialize();
    m_Initialized = OpenCoreAudio(sampleRate);
  } 
  
  /* if the stream was not initialized, do it now */
  if (!stream->IsValid())
    stream->Initialize(m_format);

  SDL_mutexV(m_Mutex);

  m_EngineLock = false;

  SDL_mutexV(m_MutexLockEngine);
  
  Start();

  return stream;
}

IAEStream *CCoreAudioAE::AlterStream(IAEStream *stream, 
                                     enum AEDataFormat dataFormat, 
                                     unsigned int sampleRate, 
                                     unsigned int channelCount, 
                                     AEChLayout channelLayout, 
                                     unsigned int options/* = 0 */)
{
  /* TODO: reconfigure the stream */
  ((CCoreAudioAEStream*)stream)->SetFreeOnDrain();

  SDL_mutexP(m_Mutex);
  stream->Drain();
  SDL_mutexV(m_Mutex);
  
  return GetStream(dataFormat, sampleRate, channelCount, channelLayout, options);
}

void CCoreAudioAE::RemoveStream(IAEStream *stream)
{
  std::list<CCoreAudioAEStream*>::iterator itt;
  
  m_streams.remove((CCoreAudioAEStream *)stream);
  
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    if (*itt == stream)
    {
      m_streams.erase(itt);
      return;
    }
  }
}

IAEStream *CCoreAudioAE::FreeStream(IAEStream *stream)
{
  SDL_mutexV(m_MutexLockEngine);  
  m_EngineLock = true;
  SDL_mutexP(m_Mutex);

  RemoveStream(stream);

  CCoreAudioAEStream *istream = (CCoreAudioAEStream *)stream;

  delete istream;

  SDL_mutexV(m_Mutex);

  m_EngineLock = false;

  SDL_mutexV(m_MutexLockEngine);

  /* When we have been in passthrough mode, reinit the hardware to come back to anlog out */
  if(m_streams.empty()/* && m_rawPassthrough*/)
  {
    Initialize();
    CLog::Log(LOGINFO, "CCoreAudioAE::FreeStream Reinit, no streams left" );
  }

  return NULL;
}

void CCoreAudioAE::PlaySound(IAESound *sound)
{
  float *samples = ((CCoreAudioAESound*)sound)->GetSamples();
  if (!samples && !m_Initialized)
    return;
 
  /* add the sound to the play list */
  SoundState ss = {
    ((CCoreAudioAESound*)sound),
    samples,
    ((CCoreAudioAESound*)sound)->GetSampleCount()
  };

  SDL_mutexP(m_Mutex);
  m_playing_sounds.push_back(ss);
  SDL_mutexV(m_Mutex);
}

void CCoreAudioAE::StopSound(IAESound *sound)
{
  SDL_mutexP(m_Mutex);
  std::list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound)
    {
      (*itt).owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
    }
    else ++itt;
  }
  SDL_mutexV(m_Mutex);
}

IAESound *CCoreAudioAE::GetSound(CStdString file)
{
  CCoreAudioAESound *sound = new CCoreAudioAESound(file);
  if (!sound->Initialize(m_format))
  {
    delete sound;
    return NULL;
  }

  SDL_mutexP(m_Mutex);  
  m_sounds.push_back(sound);
  SDL_mutexV(m_Mutex);
  
  return sound;
}

void CCoreAudioAE::RemovePlayingSound(IAESound *sound)
{
  std::list<SoundState>::iterator itt;

  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt);
    
    if(ss->owner == sound)
    {
      itt = m_playing_sounds.erase(itt);
      return;
    }
    ++itt;
  }
  
}

void CCoreAudioAE::FreeSound(IAESound *sound)
{
  if (!sound) return;

  SDL_mutexP(m_Mutex);
  sound->Stop();
  for(std::list<CCoreAudioAESound*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }

  RemovePlayingSound(sound);
  
  delete (CCoreAudioAESound*)sound;
  SDL_mutexV(m_Mutex);
}

void CCoreAudioAE::MixSounds32(float *buffer, unsigned int samples)
{
  if(!m_Initialized)
    return;
  
  std::list<SoundState>::iterator itt;
  
  //SingleLock lock(m_lock);
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt);
    
    /* no more frames, so remove it from the list */
    if (ss->sampleCount == 0)
    {
      ss->owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
      continue;
    }
    
    unsigned int mixSamples = std::min(ss->sampleCount, samples);

    float volume = ss->owner->GetVolume() * m_volume;
    
    for(unsigned int i = 0; i < mixSamples; ++i)
      buffer[i] = (buffer[i] + ss->samples[i]) * volume;
    
    ss->sampleCount -= mixSamples;
    ss->samples     += mixSamples;
    
    ++itt;
  }
}

void CCoreAudioAE::MixSounds16(int16_t *buffer, unsigned int samples)
{
  if(!m_Initialized)
    return;
  
  std::list<SoundState>::iterator itt;
  
  //SingleLock lock(m_lock);
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt);
    
    /* no more frames, so remove it from the list */
    if (ss->sampleCount == 0)
    {
      ss->owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
      continue;
    }
    
    unsigned int mixSamples = std::min(ss->sampleCount, samples);
    
    float volume = ss->owner->GetVolume() * m_volume;
    
    for(unsigned int i = 0; i < mixSamples; ++i)
    {
      float f_sound = ss->samples[i] * volume;
      
      buffer[i] = buffer[i] + safeRound(f_sound * ((float)INT16_MAX+.5f));
#ifdef __BIG_ENDIAN__
      buffer[i] = Endian_Swap16(buffer[i]);
#endif
    }
    
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
  HAL->EnumerateOutputDevices(devices, passthrough);
}

void CCoreAudioAE::Start()
{
  if(!m_Initialized)
    return;
  
  HAL->Start();
  
}

void CCoreAudioAE::Stop()
{
  if(!m_Initialized)
    return;

  HAL->Stop();

}

template <class AudioDataType>
static inline void _ReorderSmpteToCA(AudioDataType *buf, uint frames)
{
  AudioDataType tmpLS, tmpRS, tmpRLs, tmpRRs, *buf2;
  for (uint i = 0; i < frames; i++)
  {
    buf = buf2 = buf + 4;
    tmpRLs = *buf++;
    tmpRRs = *buf++;
    tmpLS = *buf++;
    tmpRS = *buf++;
    
    *buf2++ = tmpLS;
    *buf2++ = tmpRS;
    *buf2++ = tmpRLs;
    *buf2++ = tmpRRs;
  }
}

void CCoreAudioAE::ReorderSmpteToCA(void *buf, uint frames, AEDataFormat dataFormat)
{
  switch((CAEUtil::DataFormatToBits(dataFormat) >> 3))
  {
    case 8: _ReorderSmpteToCA((unsigned char *)buf, frames); break;
    case 16: _ReorderSmpteToCA((short *)buf, frames); break;
    default: _ReorderSmpteToCA((int *)buf, frames); break;
  }
}

//***********************************************************************************************
// Rendering Methods
//***********************************************************************************************
OSStatus CCoreAudioAE::OnRenderCallback(AudioUnitRenderActionFlags *ioActionFlags, 
                                            const AudioTimeStamp *inTimeStamp, 
                                            UInt32 inBusNumber, 
                                            UInt32 inNumberFrames, 
                                            AudioBufferList *ioData)
{
  SDL_mutexP(m_Mutex);  

  UInt32 frames = inNumberFrames;

  unsigned int rSamples = frames * m_format.m_channelCount;
  int size = frames * HAL->m_BytesPerFrame;
  unsigned int readframes = frames;
  
  if(!m_Initialized || m_EngineLock)
  {
    ioData->mBuffers[0].mDataByteSize = 0;
    goto out;
  }
    
  CheckOutputBufferSize((void **)&m_OutputBuffer, &m_OutputBufferSize, size);
  CheckOutputBufferSize((void **)&m_StreamBuffer, &m_StreamBufferSize, size);
  
  if(!m_OutputBuffer)
  {
    ioData->mBuffers[0].mDataByteSize = 0;
    goto out;
  }
  
  memset(m_OutputBuffer, 0x0, size);
  memset(m_StreamBuffer, 0x0, size);
    
  if(m_rawPassthrough)
  {
    if (m_streams.empty())
    {
      ioData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = 0;
      goto out;
    }
    
    CCoreAudioAEStream *stream = m_streams.front();
    if (!stream->IsRaw())
    {
      ioData->mBuffers[0].mDataByteSize = 0;
      
      RemoveStream(stream);
      delete stream;
      
      goto out;
    }
    
    size = stream->GetFrames((uint8_t *)m_OutputBuffer, size);
    
  }
  else
  {

    bool bPlayGuiSound = false;
    
    if(m_streams.empty())
    {
      bPlayGuiSound = true;
    }
    else
    {
      std::list<CCoreAudioAEStream*>::iterator itt;
      for(itt = m_streams.begin(); itt != m_streams.end();)
      {
        CCoreAudioAEStream *stream = *itt;
        
        /* On first paused stream we enable the playback of GUI sound and asume player is in pause mode. */
        if(stream->IsPaused())
        {
          bPlayGuiSound = true;
          break;
        }
        
        ++itt;
      }
    }

    
    if(m_guiSoundWhilePlayback)
    {
      if(m_format.m_dataFormat == AE_FMT_S16NE)
        MixSounds16((int16_t *)m_OutputBuffer, rSamples);
      else
        MixSounds32(m_OutputBuffer, rSamples);
    }
    else if(bPlayGuiSound) 
    {
      if(m_format.m_dataFormat == AE_FMT_S16NE)
        MixSounds16((int16_t *)m_OutputBuffer, rSamples);
      else
        MixSounds32(m_OutputBuffer, rSamples);      
    }

    if (!m_streams.empty()) 
    {
      std::list<CCoreAudioAEStream*>::iterator itt;
      
      /* mix in any running streams */
      for(itt = m_streams.begin(); itt != m_streams.end();)
      {
        CCoreAudioAEStream *stream = *itt;
        
        /* dont process streams that are paused */
        if (stream->IsPaused())
        {
          ++itt;
          continue;
        }
        
        /* AE stream is internal float. */
        if(m_format.m_dataFormat == AE_FMT_S16NE)
        {
          size = frames * m_format.m_channelCount * (CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3);
          CheckOutputBufferSize((void **)&m_StreamBuffer, &m_StreamBufferSize, size);
          CheckOutputBufferSize((void **)&m_OutputBuffer, &m_OutputBufferSize, size);
          size = stream->GetFrames((uint8_t *)m_StreamBuffer, size);
          readframes = size / (m_format.m_channelCount * (CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3));
        }
        else
        {
          size = stream->GetFrames((uint8_t *)m_StreamBuffer, size);
          readframes = size / HAL->m_BytesPerFrame;
        }
        
        if (!readframes)
        {
          /* if the stream is drained and is set to free on drain */
          if (stream->IsDraining() && stream->IsFreeOnDrain())
          {
            itt = m_streams.erase(itt);
            delete stream;
            continue;
          }
          
          ++itt;
          continue;
        }
        
        UInt32 j;
        
        float volume = stream->GetVolume() * stream->GetReplayGain() * m_volume;
        
        if(m_format.m_dataFormat == AE_FMT_S16NE)
        {
          float *src    = (float *)m_StreamBuffer;
          int16_t *dst    = (int16_t *)m_OutputBuffer;
          
          for(j = 0; j < readframes; j++)
          {          
            unsigned int i;
            for(i = 0; i < m_format.m_channelCount; i++)
            {
              float f_sound = src[i] * volume;
              
              dst[i] = dst[i] + safeRound(f_sound * ((float)INT16_MAX+.5f));
#ifdef __BIG_ENDIAN__
              dst[i] = Endian_Swap16(dst[i]);
#endif
            }
            src += m_format.m_channelCount;
            dst += m_format.m_channelCount;
          }
          size = readframes * m_format.m_channelCount * (CAEUtil::DataFormatToBits(AE_FMT_S16NE) >> 3);
        }
        else 
        {          
          float *src    = (float *)m_StreamBuffer;
          float *dst    = m_OutputBuffer;
          
          for(j = 0; j < readframes; j++)
          {          
            unsigned int i;
            for(i = 0; i < m_format.m_channelCount; i++)
            {
              dst[i] += src[i] * volume;
            }
            src += m_format.m_channelCount;
            dst += m_format.m_channelCount;
          }
        }

        ++itt;
        
      }
    }
  }

  if(!m_rawPassthrough && m_format.m_channelCount == 8)
    ReorderSmpteToCA(m_OutputBuffer, size / HAL->m_BytesPerFrame, m_format.m_dataFormat);

  memcpy((unsigned char *)ioData->mBuffers[0].mData, (uint8_t *)m_OutputBuffer, size);
  ioData->mBuffers[0].mDataByteSize = size;

out:
  SDL_mutexV(m_Mutex);

  return noErr;
}

// Static Callback from AudioUnit
OSStatus CCoreAudioAE::RenderCallback(void *inRefCon, 
                                      AudioUnitRenderActionFlags *ioActionFlags, 
                                      const AudioTimeStamp *inTimeStamp, 
                                      UInt32 inBusNumber, 
                                      UInt32 inNumberFrames, 
                                      AudioBufferList *ioData)
{
  return ((CCoreAudioAE*)inRefCon)->OnRenderCallback(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

#ifndef __arm__
OSStatus CCoreAudioAE::OnRenderCallbackDirect( AudioDeviceID inDevice,
                                                  const AudioTimeStamp * inNow,
                                                  const void * inInputData,
                                                  const AudioTimeStamp * inInputTime,
                                                  AudioBufferList * outOutputData,
                                                  const AudioTimeStamp * inOutputTime,
                                                  void * threadGlobals )
{
  SDL_mutexP(m_Mutex);

  if(!m_Initialized || m_EngineLock)
  {
    outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = 0;
    goto out;
  }
  
  if (m_streams.empty())
  {
    outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = 0;
    goto out;
  }

  /* if the stream is to be deleted, or is not raw */
  CCoreAudioAEStream *stream = m_streams.front();
  if(!stream->IsRaw())
  {
    outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = 0;
   
    RemoveStream(stream);
    delete stream;

    goto out;
  }
  
  int size = outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize;
  int readsize;
  
  CheckOutputBufferSize((void **)&m_OutputBuffer, &m_OutputBufferSize, size);
  
  if(!m_OutputBuffer)
  {
    outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = 0;
    goto out;
  }
  
  memset(m_OutputBuffer, 0x0, size);
  
  readsize = stream->GetFrames((uint8_t *)m_OutputBuffer, size);
  
  if(readsize == size)
  {    
    memcpy((unsigned char *)outOutputData->mBuffers[HAL->m_OutputBufferIndex].mData, m_OutputBuffer, size);
    outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = size;
    
  }
  else 
  {
    outOutputData->mBuffers[HAL->m_OutputBufferIndex].mDataByteSize = 0;
  }
  
out:
  SDL_mutexV(m_Mutex);

  return noErr;    
}

// Static Callback from AudioDevice
OSStatus CCoreAudioAE::RenderCallbackDirect(AudioDeviceID inDevice, 
                                                const AudioTimeStamp* inNow, 
                                                const AudioBufferList* inInputData, 
                                                const AudioTimeStamp* inInputTime, 
                                                AudioBufferList* outOutputData, 
                                                const AudioTimeStamp* inOutputTime, 
                                                void* inClientData)
{
  return ((CCoreAudioAE*)inClientData)->OnRenderCallbackDirect(inDevice, inNow, inInputData, inInputTime, outOutputData, inOutputTime, inClientData);
}
#endif

// Helper Functions
char* UInt32ToFourCC(UInt32* pVal) // NOT NULL TERMINATED! Modifies input value.
{
  UInt32 inVal = *pVal;
  char* pIn = (char*)&inVal;
  char* fourCC = (char*)pVal;
  fourCC[3] = pIn[0];
  fourCC[2] = pIn[1];
  fourCC[1] = pIn[2];
  fourCC[0] = pIn[3];
  return fourCC;
}

const char* StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str)
{
  UInt32 formatId = desc.mFormatID;
  char* fourCC = UInt32ToFourCC(&formatId);
  
  switch (desc.mFormatID)
  {
    case kAudioFormatLinearPCM:
      str.Format("[%4.4s] %s%u Channel %u-bit %s %s (%uHz)",
                 fourCC,
                 (desc.mFormatFlags & kAudioFormatFlagIsNonMixable) ? "" : "Mixable ",
                 desc.mChannelsPerFrame,
                 desc.mBitsPerChannel,
                 (desc.mFormatFlags & kAudioFormatFlagIsFloat) ? "Floating Point" : "Signed Integer",
                 (desc.mFormatFlags & kAudioFormatFlagIsBigEndian) ? "BE" : "LE",
                 (UInt32)desc.mSampleRate);
      break;
    case kAudioFormatAC3:
      str.Format("[%4.4s] AC-3/DTS (%uHz)", fourCC, (UInt32)desc.mSampleRate);
      break;
    case kAudioFormat60958AC3:
      str.Format("[%4.4s] AC-3/DTS for S/PDIF %s (%uHz)",
                 fourCC,
                 (desc.mFormatFlags & kAudioFormatFlagIsBigEndian) ? "BE" : "LE",
                 (UInt32)desc.mSampleRate);
      break;
    default:
      str.Format("[%4.4s]", fourCC);
      break;
  }
  return str.c_str();
}

/* Check buffersize and allocate buffer new if desired */
void CheckOutputBufferSize(void **buffer, int *oldSize, int newSize)
{
  if(newSize > *oldSize) 
  {
    if(*buffer)
      _aligned_free(*buffer);
    *buffer = _aligned_malloc(newSize, 16);
    *oldSize = newSize;
  }
  memset(*buffer, 0x0, *oldSize); 
}


