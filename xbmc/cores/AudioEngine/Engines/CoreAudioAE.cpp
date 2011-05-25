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

#define DELAY_FRAME_TIME  20
#define BUFFERSIZE        16416
#define SPEAKER_COUNT     9

static enum AEChannel CoreAudioChannelMap[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

CCoreAudioAE::CCoreAudioAE() :
  m_Initialized(false),
  m_OutputBufferIndex(0),
  m_BytesPerSec(0),
  m_rawPassthrough(false),
  m_RemapChannelLayout(NULL),
  m_DeviceIsRunning(false),
  m_volume(1.0f),
  m_EngineLock(false)
{
  /* Allocate internal buffers */
  m_OutputBuffer      = (float *)_aligned_malloc(BUFFERSIZE, 16);
  m_StreamBuffer      = (uint8_t *)_aligned_malloc(BUFFERSIZE, 16);
  m_StreamBufferSize  = m_OutputBufferSize = BUFFERSIZE;
  m_Mutex             = SDL_CreateMutex();
  m_MutexLockEngine   = SDL_CreateMutex();
  m_callbackCond      = SDL_CreateCond();
  //m_reinitTrigger     = new CCoreAudioAEEventThread(this);
  
  m_volume            = g_settings.m_fVolumeLevel;
  
}

CCoreAudioAE::~CCoreAudioAE()
{
  Stop();
  
  //delete m_reinitTrigger;
  
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

  if(m_callbackCond)
    SDL_DestroyCond(m_callbackCond);
  
#ifndef __arm__
  CCoreAudioHardware::ResetAudioDevices();
#endif
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
    
  /* remove any deleted streams */
  std::list<CCoreAudioAEStream*>::iterator itt_streams;
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
    m_format.m_dataFormat       = AE_FMT_FLOAT;
  }

  if (m_outputDevice.IsEmpty())
    m_outputDevice = "default";
  
  AEAudioFormat initformat = m_format;
  m_Initialized = InitializeAudioDevice(initformat, m_outputDevice);
  
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
    
#ifndef __arm__
  if (m_rawPassthrough)
    m_AudioDevice.RemoveIOProc();
  else
    m_AUOutput.SetRenderProc(nil, nil);

  m_AUOutput.Close();
  m_OutputStream.Close();
#endif
  Sleep(10);
  m_AudioDevice.Close();
  Sleep(100);
  
  m_BytesPerSec = 0;
  
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
#ifdef __arm__

  return 0.0f;

#else
  float delay;
    
  delay += (float)(m_NumLatencyFrames * m_BytesPerFrame) / m_BytesPerSec;

  return delay;
#endif
}

float CCoreAudioAE::GetVolume()
{
  return m_volume;
}

void CCoreAudioAE::SetVolume(float volume)
{
  g_settings.m_fVolumeLevel = volume;
  m_volume = volume;
  
/*
#ifndef __arm__
  if(!m_rawPassthrough)
  {
    m_AUOutput.SetCurrentVolume(m_volume);
  }  
#endif
*/
}

bool CCoreAudioAE::SupportsRaw()
{
  /* if we are going to encode, we dont do raw */
  /*
  if (!m_streams.empty())
    return false;
  */
  
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

  SDL_CondWaitTimeout(m_callbackCond, m_Mutex, 1000);
  
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
		//printf("streams empty inititalize in analoge mode\n");
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

  delete (CCoreAudioAESound*)sound;
  SDL_mutexV(m_Mutex);
}

void CCoreAudioAE::MixSounds(float *buffer, unsigned int samples)
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

//#ifdef __arm__
    float volume = ss->owner->GetVolume() * m_volume;
//#endif

    
//#ifdef __SSE__
//    CAEUtil::SSEMulAddArray(buffer, ss->samples, 1, mixSamples);
//#else
    for(unsigned int i = 0; i < mixSamples; ++i)
//#ifdef __arm__
      buffer[i] = (buffer[i] + ss->samples[i]) * volume;
//#else
//      buffer[i] = buffer[i] + ss->samples[i];
//#endif
//#endif
    
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
#ifdef __arm__
	IOSCoreAudioDeviceList deviceList;
	CIOSCoreAudioHardware::GetOutputDevices(&deviceList);
	
	/* Add default output device if GetOutputDevices return nothing */
	//if (CCoreAudioHardware::GetDefaultOutputDevice() && deviceList.empty())
	devices.push_back(AEDevice("Default", "IOSCoreAudio:default"));
	
	CStdString deviceName;
	for (int i = 0; !deviceList.empty(); i++)
	{
		CIOSCoreAudioDevice device(deviceList.front());
		device.GetName(deviceName);
		
		CStdString deviceName_Internal = CStdString("IOSCoreAudio:") + deviceName;
		devices.push_back(AEDevice(deviceName, deviceName_Internal));
		
		deviceList.pop_front();
		
	}
#else  
  CoreAudioDeviceList deviceList;
  CCoreAudioHardware::GetOutputDevices(&deviceList);
  
  /* Add default output device if GetOutputDevices return nothing */
  //if (CCoreAudioHardware::GetDefaultOutputDevice() && deviceList.empty())
  //devices.push_back(AEDevice("Default", "CoreAudio:default"));
  CStdString defaultDeviceName;
  CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);
  
  CStdString deviceName;
  for (int i = 0; !deviceList.empty(); i++)
  {
    CCoreAudioDevice device(deviceList.front());
    device.GetName(deviceName);
    
    CStdString deviceName_Internal = CStdString("CoreAudio:") + deviceName;
    devices.push_back(AEDevice(deviceName, deviceName_Internal));
    
    printf("deviceName_Internal %s\n", deviceName_Internal.c_str());

    deviceList.pop_front();
  }
#endif  
}

#ifndef __arm__
bool CCoreAudioAE::InitializePCM(AEAudioFormat &format, CStdString &device, unsigned int bps)
{ 
  // Create the MatrixMixer AudioUnit Component
  if (!m_MixerUnit.Open(kAudioUnitType_Mixer, kAudioUnitSubType_MatrixMixer, kAudioUnitManufacturer_Apple))
    return false;
  
  // Set the input stream format for the AudioUnit (this is what is being sent to us)
  AudioStreamBasicDescription inputFormat;
  inputFormat.mFormatID = kAudioFormatLinearPCM;                  //  Data encoding format
  inputFormat.mFormatFlags = /*kAudioFormatFlagsNativeEndian | */ kLinearPCMFormatFlagIsPacked;
  
  switch(format.m_dataFormat) {
    case AE_FMT_FLOAT:
      inputFormat.mFormatFlags |= kAudioFormatFlagIsFloat;
      break;
    default:
      inputFormat.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
  }
  
#ifdef __BIG_ENDIAN__
  inputFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
  
  inputFormat.mChannelsPerFrame = format.m_channelCount;          // Number of interleaved audiochannels
  inputFormat.mSampleRate = (Float64)format.m_sampleRate;         //  the sample rate of the audio stream
  inputFormat.mBitsPerChannel =  bps;                             // Number of bits per sample, per channel
  inputFormat.mBytesPerFrame = (bps>>3) * format.m_channelCount;    // Size of a frame == 1 sample per channel    
  inputFormat.mFramesPerPacket = 1;                               // The smallest amount of indivisible data. Always 1 for uncompressed audio   
  inputFormat.mBytesPerPacket = inputFormat.mBytesPerFrame * inputFormat.mFramesPerPacket;
  inputFormat.mReserved = 0;
  
  AudioStreamBasicDescription inputDesc_end;
  CStdString formatString;

  m_AUOutput.GetFormat(&inputDesc_end, kAudioUnitScope_Output, kInputBus);
 
  if (!m_AUOutput.SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
    return false;

  if (!m_AUOutput.SetFormat(&inputFormat, kAudioUnitScope_Output, kInputBus))
    return false;
    
  m_BytesPerFrame = inputFormat.mBytesPerFrame;
  m_BytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame;      // 1 sample per channel per frame
  
  return true;
}

bool CCoreAudioAE::InitializePCMEncoded(AEAudioFormat &format, CStdString &device, unsigned int bps)
{
  m_AudioDevice.SetHogStatus(true); // Prevent any other application from using this device.
  m_AudioDevice.SetMixingSupport(false); // Try to disable mixing support. Effectiveness depends on the device.
  
  // Set the Sample Rate as defined by the spec.
  m_AudioDevice.SetNominalSampleRate((float)format.m_sampleRate);
  
  if (!InitializePCM(format, device, bps))
    return false;
  
  return true;  
}

bool CCoreAudioAE::InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format, unsigned int bps)
{
  //return false; // un-comment to force PCM Spoofing (DD-Wav). For testing use only.
  
  CStdString formatString;
  AudioStreamBasicDescription outputFormat = {0};
  AudioStreamID outputStream = 0;
  bool bFound = false;
  
  // Fetch a list of the streams defined by the output device
  AudioStreamIdList streams;
  UInt32  streamIndex = 0;
  m_AudioDevice.GetStreams(&streams);
  
  m_OutputBufferIndex = 0;
  
  while (!streams.empty())
  {
    // Get the next stream
    CCoreAudioStream stream;
    stream.Open(streams.front());
    streams.pop_front(); // We copied it, now we are done with it
    
    CLog::Log(LOGDEBUG, "CCoreAudioAE::InitializeEncoded: Found %s stream - id: 0x%04X, Terminal Type: 0x%04lX",
              stream.GetDirection() ? "Input" : "Output",
              stream.GetId(),
              stream.GetTerminalType());
    
    // Probe physical formats
    StreamFormatList physicalFormats;
    stream.GetAvailablePhysicalFormats(&physicalFormats);
    while (!physicalFormats.empty())
    {
      AudioStreamRangedDescription& desc = physicalFormats.front();
      CLog::Log(LOGDEBUG, "CCoreAudioAE::InitializeEncoded:    Considering Physical Format: %s", StreamDescriptionToString(desc.mFormat, formatString));
      if (desc.mFormat.mFormatID == kAudioFormat60958AC3 || desc.mFormat.mFormatID == 'IAC3')
      {
        outputFormat = desc.mFormat; // Select this format
        m_OutputBufferIndex = streamIndex;
        outputStream = stream.GetId();
        
        /* Adjust samplerate */
        outputFormat.mChannelsPerFrame = format.m_channelCount;
        outputFormat.mSampleRate = (Float64)format.m_sampleRate;
        stream.SetPhysicalFormat(&outputFormat);

        m_AUOutput.SetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus);
        
        bFound = true;
        
        break;
      }
      physicalFormats.pop_front();
    }
    
    if (bFound)
      break; // We found a suitable format. No need to continue.

    streamIndex++;
  }
  
  if (!bFound) // No match found
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAE::InitializeEncoded: Unable to identify suitable output format.");
    return false;
  }
  
  m_BytesPerSec = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3) * outputFormat.mSampleRate; // mBytesPerFrame is 0 for a cac3 stream  
  m_BytesPerFrame = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3);
  
  CLog::Log(LOGDEBUG, "CCoreAudioAE::InitializeEncoded: Selected stream[%lu] - id: 0x%04lX, Physical Format: %s (%lu Bytes/sec.)", m_OutputBufferIndex, outputStream, StreamDescriptionToString(outputFormat, formatString), m_BytesPerSec);
  
  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  
  m_AudioDevice.SetHogStatus(true); // Hog the device if it is not set to be done automatically
  m_AudioDevice.SetMixingSupport(false); // Try to disable mixing. If we cannot, it may not be a problem
  
  m_NumLatencyFrames = m_AudioDevice.GetNumLatencyFrames();
  
  // Configure the output stream object
  m_OutputStream.Open(outputStream); // This is the one we will keep
  AudioStreamBasicDescription previousFormat;
  m_OutputStream.GetPhysicalFormat(&previousFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAE::InitializeEncoded: Previous Physical Format: %s (%lu Bytes/sec.)", StreamDescriptionToString(previousFormat, formatString), m_BytesPerSec);
  m_OutputStream.SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  m_NumLatencyFrames += m_OutputStream.GetNumLatencyFrames();
  
  // Register for data request callbacks from the driver
  m_AudioDevice.AddIOProc(RenderCallbackDirect, this);
  
  return true;
}
#endif

bool CCoreAudioAE::InitializeAudioDevice(AEAudioFormat &format, CStdString &device)
{ 
#ifndef __arm__
  int bPassthrough = m_rawPassthrough;
#endif
  unsigned int bps = CAEUtil::DataFormatToBits(format.m_dataFormat);;
  
  if (format.m_channelCount == 0)
  {
    CLog::Log(LOGERROR, "CCoreAudioAE::Initialize - Unable to open the requested channel layout");
    return false;
  }

#ifdef __arm__
  // Set the input stream format for the AudioUnit
  // We use the default DefaultOuput AudioUnit, so we only can set the input stream format.
  // The autput format is automaticaly set to the input format.
  AudioStreamBasicDescription audioFormat;
  audioFormat.mFormatID = kAudioFormatLinearPCM;						//  Data encoding format
  audioFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;
	switch(format.m_dataFormat) {
    case AE_FMT_FLOAT:
      audioFormat.mFormatFlags |= kAudioFormatFlagIsFloat;
      break;
    default:
      audioFormat.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
	}
  audioFormat.mChannelsPerFrame = format.m_channelCount;		// Number of interleaved audiochannels
  audioFormat.mSampleRate = (Float64)format.m_sampleRate;		//  the sample rate of the audio stream
  audioFormat.mBitsPerChannel = bps;						// Number of bits per sample, per channel
  audioFormat.mBytesPerFrame = (bps>>3) * format.m_channelCount; // Size of a frame == 1 sample per channel   
  audioFormat.mFramesPerPacket = 1;													// The smallest amount of indivisible data. Always 1 for uncompressed audio   
  audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame * audioFormat.mFramesPerPacket;
  audioFormat.mReserved = 0;
	
  // Attach our output object to the device
  if(!m_AudioDevice.Init(/*m_Passthrough*/ true, &audioFormat, RenderCallback, this))
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAE::Init failed");
    return false;
  }
	
	UInt32 m_PacketSize = 64;
	m_AudioDevice.FramesPerSlice(m_PacketSize);
		
  // set the format parameters
  m_BytesPerFrame = audioFormat.mBytesPerFrame;
	
	if (!m_AudioDevice.Open())
		return false;
  
#else
  
  // Reset all the devices to a default 'non-hog' and mixable format.
  // If we don't do this we may be unable to find the Default Output device.
  // (e.g. if we crashed last time leaving it stuck in AC-3 mode)
  CCoreAudioHardware::ResetAudioDevices();
  
  device.Replace("CoreAudio:", "");
  
  AudioDeviceID outputDevice = CCoreAudioHardware::FindAudioDevice(device);
  
  if (!outputDevice) // Fall back to the default device if no match is found
  {
    CLog::Log(LOGWARNING, "CCoreAudioAE::Initialize: Unable to locate configured device, falling-back to the system default.");
    outputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
    if (!outputDevice) // Not a lot to be done with no device. TODO: Should we just grab the first existing device?
      return false;
  }
  
  // Attach our output object to the device
  m_AudioDevice.Open(outputDevice);
  
  // If this is a passthrough (AC3/DTS) stream, attempt to handle it natively
  if (bPassthrough)
  {
    m_rawPassthrough = InitializeEncoded(outputDevice, format, bps);
    Sleep(100);
  }
  
  // If this is a PCM stream, or we failed to handle a passthrough stream natively,
  // prepare the standard interleaved PCM interface
  if (!m_rawPassthrough)
  {    
    // Create the Output AudioUnit Component
    if (!m_AUOutput.Open(kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple))
      return false;
    
    // Hook the Ouput AudioUnit to the selected device
    if (!m_AUOutput.SetCurrentDevice(outputDevice))
      return false;
    
    // If we are here and this is a passthrough stream, native handling failed.
    // Try to handle it as IEC61937 data over straight PCM (DD-Wav)
    bool configured = false;
    if (bPassthrough)
    {
      CLog::Log(LOGDEBUG, "CCoreAudioAE::Initialize: No suitable AC3 output format found. Attempting DD-Wav.");
      configured = InitializePCMEncoded(format, device, bps);
      // TODO: wait for audio device startup
      Sleep(100);
    }
    else
    {
      // Standard PCM data
      configured = InitializePCM(format, device, bps);
      // TODO: wait for audio device startup
      Sleep(100);
    }
    
    if (!configured) // No suitable output format was able to be configured
      return false;
    
    // Configure the maximum number of frames that the AudioUnit will ask to process at one time.
    // If this is not called, there is no guarantee that the callback will ever be called.
    m_AUOutput.SetBufferFrameSize(512);
    
    // Setup the callback function that the AudioUnit will use to request data  
    if (!m_AUOutput.SetRenderProc(RenderCallback, this))
      return false;

    // Initialize the Output AudioUnit
    if (!m_AUOutput.Initialize())
      return false;
    
    // Initialize the Output AudioUnit
    //if (!m_MixerUnit.Initialize())
    //  return false;
    
    // Log some information about the stream
    AudioStreamBasicDescription inputDesc_end, outputDesc_end;
    CStdString formatString;
    m_AUOutput.GetFormat(&inputDesc_end, kAudioUnitScope_Output, kInputBus);

    if(inputDesc_end.mChannelsPerFrame != format.m_channelCount) {
      CLog::Log(LOGERROR, "CCoreAudioAE::Initialize: Output channel count does not match input channel count. in %d : out %d. Please Correct your speaker layout setting.", 
                format.m_channelCount, inputDesc_end.mChannelsPerFrame);      
      format.m_channelCount = inputDesc_end.mChannelsPerFrame;
      if(!InitializePCM(format, device, bps))
      {
        CLog::Log(LOGERROR, "CCoreAudioAE::Initialize: Reinit with right channel count failed"); 
        return false;
      }
      Sleep(100);
    }

    m_AUOutput.GetFormat(&inputDesc_end, kAudioUnitScope_Output, kInputBus);
    m_AUOutput.GetFormat(&outputDesc_end, kAudioUnitScope_Input, kOutputBus);
    CLog::Log(LOGDEBUG, "CCoreAudioAE::Initialize: Input Stream Format %s", StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CCoreAudioAE::Initialize: Output Stream Format % s", StreamDescriptionToString(outputDesc_end, formatString));    
    
  }

  m_NumLatencyFrames = m_AudioDevice.GetNumLatencyFrames();
#endif
  
  // set the format parameters
  format.m_frameSize    = m_BytesPerFrame;
  
  return true;
}

void CCoreAudioAE::Start()
{
  if(!m_Initialized)
    return;
  
#ifndef __arm__
  if (m_rawPassthrough)
#endif
    m_AudioDevice.Start();
#ifndef __arm__
  else
    m_AUOutput.Start();
#endif

}

void CCoreAudioAE::Stop()
{
  if(!m_Initialized)
    return;

#ifndef __arm__
  if (m_rawPassthrough)
#endif
    m_AudioDevice.Stop();
#ifndef __arm__
  else
    m_AUOutput.Stop();
#endif

}

/* Check buffersize and allocate buffer new if desired */
void CCoreAudioAE::CheckOutputBufferSize(void **buffer, int *oldSize, int newSize)
{
  if(newSize > *oldSize) 
  {
    if(*buffer)
      _aligned_free(*buffer);
    *buffer = _aligned_malloc(newSize, 16);
    *oldSize = newSize;
  }
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
  
  unsigned int rSamples = inNumberFrames * m_format.m_channelCount;
  int size = inNumberFrames * m_BytesPerFrame;
  unsigned int readframes = inNumberFrames;
  //bool  reinit = false;
  
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
      ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
      
      //reinit = true;

      goto out;
    }
    
    CCoreAudioAEStream *stream = m_streams.front();
    if (stream->IsDestroyed() || !stream->IsRaw())
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
    MixSounds(m_OutputBuffer, rSamples);
  
    if (!m_streams.empty()) 
    {
      std::list<CCoreAudioAEStream*>::iterator itt;
      
      /* mix in any running streams */
      for(itt = m_streams.begin(); itt != m_streams.end();)
      {
        CCoreAudioAEStream *stream = *itt;
        
        /* skip streams that are flagged for deletion */
        if (stream->IsDestroyed())
        {
          if (!stream->IsBusy())
          {
            itt = m_streams.erase(itt);
            RemoveStream(stream);
            delete stream;
          }
          else
            ++itt;
          
          continue;
        }
        
        /* dont process streams that are paused */
        if (stream->IsPaused())
        {
          ++itt;
          continue;
        }
        
        size = stream->GetFrames((uint8_t *)m_StreamBuffer, size);
        
        readframes = size / m_BytesPerFrame;
        
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
        
        float *src    = (float *)m_StreamBuffer;
        float *dst    = m_OutputBuffer;
  

//#ifdef __arm__
        float volume = stream->GetVolume() * stream->GetReplayGain() * m_volume;
//#endif

        for(j = 0; j < readframes; j++)
        {          
//#ifdef __SSE__
//          CAEUtil::SSEMulAddArray(dst, src, 1, m_format.m_channelCount);
//#else      
          unsigned int i;
          for(i = 0; i < m_format.m_channelCount; i++)
          {
//#ifdef __arm__
            dst[i] += src[i] * volume;
//#else
//            dst[i] += src[i];
//#endif
          }
//#endif
          src += m_format.m_channelCount;
          dst += m_format.m_channelCount;
        }
        
        ++itt;
        
      }
    }
  }

  if(!m_rawPassthrough && m_format.m_channelCount == 8)
    ReorderSmpteToCA(m_OutputBuffer, size / m_BytesPerFrame, m_format.m_dataFormat);

  memcpy((unsigned char *)ioData->mBuffers[0].mData, (uint8_t *)m_OutputBuffer, size);
  ioData->mBuffers[0].mDataByteSize = size;

out:
  SDL_CondBroadcast(m_callbackCond);
  SDL_mutexV(m_Mutex);

  //if(reinit)
  //  m_reinitTrigger->Trigger();
  
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
  //bool reinit = false;
  
  SDL_mutexP(m_Mutex);

  if(!m_Initialized || m_EngineLock)
  {
    outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
    goto out;
  }
  
  if (m_streams.empty())
  {
    outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
    
    //reinit = true;
    
    goto out;
  }

  /* if the stream is to be deleted, or is not raw */
  CCoreAudioAEStream *stream = m_streams.front();
  if(stream->IsDestroyed() || !stream->IsRaw())
  {
    outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
   
    RemoveStream(stream);
    delete stream;

    goto out;
  }
  
  int size = outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize;
  int readsize;
  
  CheckOutputBufferSize((void **)&m_OutputBuffer, &m_OutputBufferSize, size);
  
  if(!m_OutputBuffer)
  {
    outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
    goto out;
  }
  
  memset(m_OutputBuffer, 0x0, size);
  
  readsize = stream->GetFrames((uint8_t *)m_OutputBuffer, size);
  
  if(readsize == size)
  {    
    memcpy((unsigned char *)outOutputData->mBuffers[m_OutputBufferIndex].mData, m_OutputBuffer, size);
    outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize = size;
    
  }
  else 
  {
    outOutputData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
  }
  
out:
  SDL_CondBroadcast(m_callbackCond);
  SDL_mutexV(m_Mutex);

  //if(reinit)
  //  m_reinitTrigger->Trigger();
  
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
