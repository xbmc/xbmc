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

#include <string.h>

#include "system.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "DllAvCore.h"

#include "SoftAE.h"
#include "SoftAESound.h"
#include "SoftAEStream.h"
#include "AESinkFactory.h"
#include "AESink.h"
#include "AEUtil.h"
#include "Encoders/AEEncoderFFmpeg.h"

#ifdef __SSE__
#include <xmmintrin.h>
#endif

/*
  frame delay time in milliseconds when there is no sink
  must NOT be less then 4
*/
#define DELAY_FRAME_TIME 20

using namespace std;

CSoftAE::CSoftAE():
  m_thread             (NULL ),
  m_running            (false),
  m_reOpened           (false),
  m_sink               (NULL ),
  m_rawPassthrough     (false),
  m_passthrough        (false),
  m_buffer             (NULL ),
  m_encoder            (NULL ),
  m_encodedBuffer      (NULL ),
  m_encodedBufferSize  (0    ),
  m_encodedBufferPos   (0    ),
  m_encodedBufferFrames(0    ),
  m_encodedPending     (false),
  m_remapped           (NULL ),
  m_remappedSize       (0    ),
  m_converted          (NULL ),
  m_convertedSize      (0    )
{
}

CSoftAE::~CSoftAE()
{
  Deinitialize();

  /* free the streams */
  CSingleLock streamLock(m_streamLock);
  while(!m_streams.empty())
  {
    CSoftAEStream *s = m_streams.front();
    /* note: the stream will call RemoveStream via it's dtor */
    delete s;
  }

  /* free the sounds */
  CSingleLock soundLock(m_soundLock);
  while(!m_sounds.empty())
  {
    CSoftAESound *s = m_sounds.front();
    m_sounds.pop_front();
    delete s;
  }
}

IAESink *CSoftAE::GetSink(AEAudioFormat &newFormat, bool passthrough, CStdString &device)
{
  device = passthrough ? m_passthroughDevice : m_device;
  CStdString driver = passthrough ? m_passthroughDriver : m_driver;

  IAESink *sink = CAESinkFactory::Create(driver, device, newFormat, passthrough);

  if (sink)
  {
    if (passthrough)
      m_passthroughDriver = sink->GetName();
    else
      m_driver = sink->GetName();
  }

  return sink;
}

bool CSoftAE::OpenSink(unsigned int sampleRate/* = 44100*/, bool forceRaw/* = false */)
{
  /* lock the sounds before we take the sink lock */
  CSingleLock soundLock(m_soundLock);
  list<CSoftAESound*>::iterator sitt;
  for(sitt = m_sounds.begin(); sitt != m_sounds.end(); ++sitt)
    (*sitt)->Lock();

  /* lock the sink so the thread gets held up */
  m_sinkLock.EnterExclusive();
  LoadSettings();

  /* load the configuration */
  enum AEStdChLayout stdChLayout = AE_CH_LAYOUT_2_0;
  switch(g_guiSettings.GetInt("audiooutput.channellayout"))
  {
    default:
    case  0: stdChLayout = AE_CH_LAYOUT_2_0; break; /* dont alow 1_0 output */
    case  1: stdChLayout = AE_CH_LAYOUT_2_0; break;
    case  2: stdChLayout = AE_CH_LAYOUT_2_1; break;
    case  3: stdChLayout = AE_CH_LAYOUT_3_0; break;
    case  4: stdChLayout = AE_CH_LAYOUT_3_1; break;
    case  5: stdChLayout = AE_CH_LAYOUT_4_0; break;
    case  6: stdChLayout = AE_CH_LAYOUT_4_1; break;
    case  7: stdChLayout = AE_CH_LAYOUT_5_0; break;
    case  8: stdChLayout = AE_CH_LAYOUT_5_1; break;
    case  9: stdChLayout = AE_CH_LAYOUT_7_0; break;
    case 10: stdChLayout = AE_CH_LAYOUT_7_1; break;
  }

  /* remove any deleted streams */
  CSingleLock streamLock(m_streamLock);
  list<CSoftAEStream*>::iterator itt;
  for(itt = m_streams.begin(); itt != m_streams.end();)
  {
    if ((*itt)->IsDestroyed() && !(*itt)->IsBusy())
    {
      CSoftAEStream *stream = *itt;
      itt = m_streams.erase(itt);
      delete stream;
      continue;
    }
    ++itt;
  }

  if (forceRaw)
    m_rawPassthrough = true;
  else
    m_rawPassthrough = !m_streams.empty() && m_streams.front()->IsRaw();

  /* override the sample rate based on the oldest stream if there is one */
  if (!m_streams.empty())
    sampleRate = m_streams.front()->GetSampleRate();

  streamLock.Leave();

  CStdString device, driver;
  if (m_passthrough || m_rawPassthrough)
  {
    device = m_passthroughDevice;
    driver = m_passthroughDriver;
  }
  else
  {
    device = m_device;
    driver = m_driver;
  }

  /*
    try to use 48000hz if we are going to transcode, this prevents the sink
    from being re-opened repeatedly when switching sources, which locks up
    some receivers & crappy integrated sound drivers.
  */
  if (m_passthrough && !m_rawPassthrough)
    sampleRate = 48000;

  /*
    if there is an audio resample rate set, use it, this MAY NOT be honoured as
    the audio sink may not support the requested format, and may change it.
  */
  if (g_advancedSettings.m_audioResample)
  {
    sampleRate = g_advancedSettings.m_audioResample;
    CLog::Log(LOGINFO, "CSoftAE::OpenSink - Forcing samplerate to %d", sampleRate);
  }

  /* setup the desired format */
  AEAudioFormat newFormat;
  newFormat.m_channelLayout = CAEUtil::GetStdChLayout  (stdChLayout);
  newFormat.m_channelCount  = CAEUtil::GetChLayoutCount(newFormat.m_channelLayout);
  newFormat.m_sampleRate    = sampleRate;
  newFormat.m_dataFormat    = (m_rawPassthrough || m_passthrough) ? AE_FMT_RAW : AE_FMT_FLOAT;

  /* if the sink is already open and it is compatible we dont need to do anything */
  if (m_sink && m_sink->IsCompatible(newFormat, device))
  {
    if (driver.IsEmpty() || m_sink->GetName() == driver)
    {
      /* unlock the sounds */
      for(sitt = m_sounds.begin(); sitt != m_sounds.end(); ++sitt)
        (*sitt)->UnLock();

      m_sinkLock.LeaveExclusive();
      return true;
    }
  }

  /* let the thread know we have re-opened the sink */
  m_reOpened = true;

  /* we are going to open, so close the old sink if it was open */
  if (m_sink)
  {
    m_sink->Drain();
    m_sink->Deinitialize();
    delete m_sink;
    m_sink = NULL;

    _aligned_free(m_buffer);
    m_buffer = NULL;
  }

  /* setup the standard output layout & format */
  m_chLayout     = CAEUtil::GetStdChLayout  (stdChLayout);
  m_channelCount = CAEUtil::GetChLayoutCount(m_chLayout );

  /* create the new sink */
  m_sink = GetSink(newFormat, m_passthrough || m_rawPassthrough, device);
  if (!m_sink)
  {
    /* we failed, set the data format to defaults so the thread does not block */
    newFormat.m_dataFormat    = (m_rawPassthrough || m_passthrough) ? AE_FMT_S16NE : AE_FMT_FLOAT;
    newFormat.m_channelLayout = CAEUtil::GetStdChLayout(stdChLayout);
    newFormat.m_channelCount  = (m_rawPassthrough || m_passthrough) ? 2 : m_channelCount;
    newFormat.m_sampleRate    = sampleRate;
    newFormat.m_frames        = (unsigned int)(((float)sampleRate / 1000.0f) * (float)DELAY_FRAME_TIME);
    newFormat.m_frameSamples  = newFormat.m_frames * newFormat.m_channelCount;
    newFormat.m_frameSize     = (CAEUtil::DataFormatToBits(newFormat.m_dataFormat) >> 3) * newFormat.m_channelCount;
  }

  /* if raw passthrough correct the layout and channel count and ensure there is no encoder */
  if (m_rawPassthrough)
  {
    m_chLayout     = newFormat.m_channelLayout;
    m_channelCount = newFormat.m_channelCount;

    delete m_encoder;
    m_encoder = NULL;
  }

  m_sinkFormat = newFormat;

  CLog::Log(LOGINFO, "CSoftAE::Initialize - %s Initialized:", m_sink ? m_sink->GetName() : "NULL");
  CLog::Log(LOGINFO, "  Output Device : %s", m_sink ? device.c_str() : "NULL");
  CLog::Log(LOGINFO, "  Sample Rate   : %d", newFormat.m_sampleRate);
  CLog::Log(LOGINFO, "  Sample Format : %s", CAEUtil::DataFormatToStr(newFormat.m_dataFormat));
  CLog::Log(LOGINFO, "  Channel Count : %d", newFormat.m_channelCount);
  CLog::Log(LOGINFO, "  Channel Layout: %s", CAEUtil::GetChLayoutStr(newFormat.m_channelLayout).c_str());
  CLog::Log(LOGINFO, "  Frames        : %d", newFormat.m_frames);
  CLog::Log(LOGINFO, "  Frame Samples : %d", newFormat.m_frameSamples);
  CLog::Log(LOGINFO, "  Frame Size    : %d", newFormat.m_frameSize);

  m_bufferSamples = 0;
  if (m_rawPassthrough)
  {
    m_convertFn      = NULL;
    m_buffer         = _aligned_malloc(newFormat.m_frames * newFormat.m_frameSize, 16);
    m_bytesPerSample = CAEUtil::DataFormatToBits(newFormat.m_dataFormat) >> 3;  
    m_frameSize      = newFormat.m_frameSize;
  }
  else
  {
    /* if we are transcoding */
    if (m_passthrough)
    {
      /* configure the encoder */
      m_encoderFormat.m_sampleRate    = newFormat.m_sampleRate;
      m_encoderFormat.m_dataFormat    = AE_FMT_FLOAT;
      m_encoderFormat.m_channelLayout = m_chLayout;
      m_encoderFormat.m_channelCount  = m_channelCount;
      SetupEncoder(m_encoderFormat);
      
      /* remap directly to the format we need for encode */
      m_chLayout     = m_encoderFormat.m_channelLayout;
      m_channelCount = m_encoderFormat.m_channelCount;
      m_convertFn    = CAEConvert::FrFloat(m_encoderFormat.m_dataFormat);
      m_bufferSize   = m_encoderFormat.m_frames * sizeof(float) * m_channelCount;
      m_buffer       = _aligned_malloc(m_bufferSize, 16);
      
      CLog::Log(LOGDEBUG, "CSoftAE::Initialize - Encoding using layout: %s", CAEUtil::GetChLayoutStr(m_chLayout).c_str());
    }
    else
    {
      m_convertFn  = CAEConvert::FrFloat(newFormat.m_dataFormat);
      m_bufferSize = newFormat.m_frames * sizeof(float) * m_channelCount;
      m_buffer     = _aligned_malloc(m_bufferSize, 16);
      
      CLog::Log(LOGDEBUG, "CSoftAE::Initialize - Using speaker layout: %s", CAEUtil::GetStdChLayoutName(stdChLayout));
    }

    m_bytesPerSample = CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3;
    m_frameSize      = m_bytesPerSample * m_channelCount;
  }
  
  /* initialize the final stage remapper */
  m_remap.Initialize(m_chLayout, newFormat.m_channelLayout, true);

  /* if we in raw passthrough, we are finished */
  if (m_rawPassthrough)
  {
    /* unlock the sounds */
    for(sitt = m_sounds.begin(); sitt != m_sounds.end(); ++sitt)
      (*sitt)->UnLock();

    m_sinkLock.LeaveExclusive();
    return true;
  }

  /* re-init sounds and unlock */
  for(sitt = m_sounds.begin(); sitt != m_sounds.end(); ++sitt)
  {
    (*sitt)->Initialize();
    (*sitt)->UnLock();
  }
  soundLock.Leave();

  /* re-init streams */
  streamLock.Enter();
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->Initialize();  
  streamLock.Leave();

  bool valid = m_sink != NULL;
  m_sinkLock.LeaveExclusive();

  return valid;
}

bool CSoftAE::SetupEncoder(AEAudioFormat &format)
{  
  delete m_encoder;
  m_encoder = NULL;

  delete[] m_encodedBuffer;
  m_encodedBuffer       = NULL;
  m_encodedBufferSize   = 0;
  m_encodedBufferPos    = 0;
  m_encodedBufferFrames = 0;
  m_encodedPending      = false;

  if (!m_passthrough)
    return false;

  m_encoder = new CAEEncoderFFmpeg();
  if (m_encoder->Initialize(format))
    return true;

  delete m_encoder;
  m_encoder = NULL;
  return false;
}

bool CSoftAE::Initialize()
{
  /* get the current volume level */
  m_volume = g_settings.m_fVolumeLevel;

  /* we start even if we failed to open a sink */
  OpenSink();
  m_running = true;
  m_thread  = new CThread(this);
  m_thread->Create();

  return true;
}

void CSoftAE::OnSettingsChange(CStdString setting)
{
  if (setting == "audiooutput.dontnormalizelevels")
  {
    /* re-init streams reampper */
    CSingleLock streamLock(m_streamLock);
    list<CSoftAEStream*>::iterator itt;
    for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->InitializeRemap();
  }

  if (setting == "audiooutput.passthroughdevice" ||
      setting == "audiooutput.custompassthrough" ||
      setting == "audiooutput.audiodevice"       ||
      setting == "audiooutput.customdevice"      ||
      setting == "audiooutput.mode"              ||
      setting == "audiooutput.ac3passthrough"    ||
      setting == "audiooutput.channellayout")
  {
    OpenSink();
  }
}

void CSoftAE::LoadSettings()
{
  int pos;

  m_passthroughDevice = g_guiSettings.GetString("audiooutput.passthroughdevice");
  if (m_passthroughDevice == "custom")
    m_passthroughDevice = g_guiSettings.GetString("audiooutput.custompassthrough");

  if (m_passthroughDevice.IsEmpty())
    m_passthroughDevice = g_guiSettings.GetString("audiooutput.audiodevice");

  pos = m_passthroughDevice.find_first_of(':');
  if (pos > 0)
  {
    m_passthroughDriver = m_passthroughDevice.substr(0, pos);
    m_passthroughDriver = m_passthroughDriver.ToUpper();
    m_passthroughDevice = m_passthroughDevice.substr(pos + 1, m_passthroughDevice.length() - pos - 1);
  }
  else
    m_passthroughDriver.Empty();

  if (m_passthroughDevice.IsEmpty())
    m_passthroughDevice = "default";

  m_device = g_guiSettings.GetString("audiooutput.audiodevice");
  if (m_device == "custom")
    m_device = g_guiSettings.GetString("audiooutput.customdevice");

  pos = m_device.find_first_of(':');
  if (pos > 0)
  {
    m_driver = m_device.substr(0, pos);
    m_driver = m_driver.ToUpper();
    m_device = m_device.substr(pos + 1, m_device.length() - pos - 1);
  }
  else
    m_driver.Empty();

  if (m_device.IsEmpty())
    m_device = "default";

  m_passthrough =
    g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958 &&
    g_guiSettings.GetBool("audiooutput.ac3passthrough");

  if (m_passthrough)
	CLog::Log(LOGINFO, "CSoftAE::OnSettingsChange - Transcode Enabled");
}

void CSoftAE::Deinitialize()
{
  if (m_thread)
  {
    Stop();
    m_thread->StopThread(true);
    delete m_thread;
    m_thread = NULL;
  }

  if (m_sink)
  {
    /* shutdown the sink */
    m_sink->Deinitialize();
    delete m_sink;
    m_sink = NULL;
  }

  delete m_encoder;
  m_encoder = NULL;

  delete[] m_encodedBuffer;
  m_encodedBuffer       = NULL;
  m_encodedBufferSize   = 0;
  m_encodedBufferPos    = 0;
  m_encodedBufferFrames = 0;  
  m_encodedPending      = false;

  _aligned_free(m_buffer);
  m_buffer = NULL;

  _aligned_free(m_converted);
  m_converted = NULL;
  m_convertedSize = 0;

  _aligned_free(m_remapped);
  m_remapped = NULL;
  m_remappedSize = 0;
}

void CSoftAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  CAESinkFactory::Enumerate(devices, passthrough);
}

bool CSoftAE::SupportsRaw()
{
  /* if we are going to encode, we dont do raw */
  if (m_passthrough && !m_streams.empty())
    return false;

  return true;
}

/* this is used when there is no sink to prevent us running too fast */
void CSoftAE::DelayFrames()
{
  /*
    since there is no audio, this does not need to be exact
    but less then the actual by a little to cope with system
    overheads otherwise the video output is jerky
  */
  Sleep(DELAY_FRAME_TIME - 4);
}

void CSoftAE::Stop()
{
  m_running = false;

  /* wait for the thread to stop */
  CSingleLock lock(m_runningLock);
}

IAEStream *CSoftAE::GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options/* = 0 */)
{
  CLog::Log(LOGINFO, "CSoftAE::GetStream - %s, %u, %u, %s",
    CAEUtil::DataFormatToStr(dataFormat),
    sampleRate,
    channelCount,
    CAEUtil::GetChLayoutStr(channelLayout).c_str()
  );

  CSingleLock streamLock(m_streamLock);
  bool wasEmpty = m_streams.empty();
  CSoftAEStream *stream = new CSoftAEStream(dataFormat, sampleRate, channelCount, channelLayout, options);
  m_streams.push_back(stream);
  streamLock.Leave();

  if (dataFormat == AE_FMT_RAW)
    OpenSink(sampleRate, true);
  else if (wasEmpty)
    OpenSink(sampleRate);

  /* if the stream was not initialized, do it now */
  if (!stream->IsValid())
    stream->Initialize();

  return stream;
}

IAEStream *CSoftAE::AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options/* = 0 */)
{
  /* TODO: reconfigure the stream */
  ((CSoftAEStream*)stream)->SetFreeOnDrain();
  stream->Drain();

  return GetStream(dataFormat, sampleRate, channelCount, channelLayout, options);
}

IAESound *CSoftAE::GetSound(CStdString file)
{
  CSingleLock soundLock(m_soundLock);

  CSoftAESound *sound = new CSoftAESound(file);
  if (!sound->Initialize())
  {
    delete sound;
    return NULL;
  }

  m_sounds.push_back(sound);
  return sound;
}

void CSoftAE::PlaySound(IAESound *sound)
{
   float *samples = ((CSoftAESound*)sound)->GetSamples();
   if (!samples)
     return;

   /* add the sound to the play list */
   CSingleLock soundSampleLock(m_soundSampleLock);
   SoundState ss = {
      ((CSoftAESound*)sound),
      samples,
      ((CSoftAESound*)sound)->GetSampleCount()
   };
   m_playing_sounds.push_back(ss);
}

void CSoftAE::FreeSound(IAESound *sound)
{
  if (!sound) return;

  sound->Stop();
  CSingleLock soundLock(m_soundLock);
  for(list<CSoftAESound*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }

  delete (CSoftAESound*)sound;
}

void CSoftAE::GarbageCollect()
{
}

unsigned int CSoftAE::GetSampleRate()
{
  return m_passthrough && m_encoder ? m_encoderFormat.m_sampleRate : m_sinkFormat.m_sampleRate;
}

void CSoftAE::StopSound(IAESound *sound)
{
  CSingleLock lock(m_soundSampleLock);
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound)
    {
      (*itt).owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
    }
    else ++itt;
  }
}

void CSoftAE::RemoveStream(IAEStream *stream)
{
  list<CSoftAEStream*>::iterator itt;
  CSingleLock lock(m_streamLock);

  /* ensure the stream still exists */
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    if (*itt == stream)
    {
      m_streams.erase(itt);
      lock.Leave();
      OpenSink();

      return;
    }
}

float CSoftAE::GetDelay()
{
  if (!m_running)
    return 0.0f;

  m_sinkLock.EnterShared();

  float delay = 0.0f;
  if (m_sink)
    delay = m_sink->GetDelay();

  if (m_passthrough && m_encoder)
    delay += m_encoder->GetDelay(m_encodedBufferFrames - m_encodedBufferPos);

  unsigned int buffered = m_bufferSamples / m_channelCount;
  delay += (float)buffered / (float)m_sinkFormat.m_sampleRate;

  m_sinkLock.LeaveShared();

  return delay;
}

float CSoftAE::GetVolume()
{
  return m_volume;
}

void CSoftAE::SetVolume(float volume)
{
  g_settings.m_fVolumeLevel = volume;
  m_volume = volume;
}

#ifdef __SSE__
inline void CSoftAE::SSEMulAddArray(float *data, float *add, const float mul, uint32_t count)
{
  const __m128 m = _mm_set_ps1(mul);

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)add & 0xF)) && count > 0)
  {
    data[0] += add[0] * mul;
    ++add;
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i+=4, data+=4, add+=4)
  {
    __m128 ad      = _mm_load_ps(add );
    __m128 to      = _mm_load_ps(data);
    *(__m128*)data = _mm_add_ps (to, _mm_mul_ps(ad, m));
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] += add[0] * mul;
    else
    {
      __m128 ad;
      __m128 to;
      if (odd == 2)
      {
        ad = _mm_setr_ps(add [0], add [1], 0, 0);
        to = _mm_setr_ps(data[0], data[1], 0, 0);
        __m128 ou = _mm_add_ps(to, _mm_mul_ps(ad, m));
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
      }
      else
      {
        ad = _mm_setr_ps(add [0], add [1], add [2], 0);
        to = _mm_setr_ps(data[0], data[1], data[2], 0);
        __m128 ou = _mm_add_ps(to, _mm_mul_ps(ad, m));
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
        data[2] = ((float*)&ou)[2];
      }
    }
  }
}

inline void CSoftAE::SSEMulArray(float *data, const float mul, uint32_t count)
{
  const __m128 m = _mm_set_ps1(mul);

  /* work around invalid alignment */
  while(((uintptr_t)data & 0xF) && count > 0)
  {
    data[0] *= mul;
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i+=4, data+=4)
  {
    __m128 to      = _mm_load_ps(data);
    *(__m128*)data = _mm_mul_ps (to, m);
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] *= mul;
    else
    {     
      __m128 to;
      if (odd == 2)
      {
        to = _mm_setr_ps(data[0], data[1], 0, 0);
        __m128 ou = _mm_mul_ps(to, m);
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
      }
      else
      {
        to = _mm_setr_ps(data[0], data[1], data[2], 0);
        __m128 ou = _mm_mul_ps(to, m);
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
        data[2] = ((float*)&ou)[2];
      }
    }
  }
}
#endif

void CSoftAE::Run()
{
  /* we release this when we exit the thread unblocking anyone waiting on "Stop" */
  CSingleLock runningLock(m_runningLock);

  uint8_t *out = NULL;
  size_t   outSize = 0;

  CLog::Log(LOGINFO, "CSoftAE::Run - Thread Started");

  /* copy these value so we can use them outside of the sink lock */
  unsigned int channelCount = m_channelCount;
  size_t size               = m_frameSize;

  m_sinkLock.EnterShared();
  while(m_running)
  {
    m_reOpened = false;

    /* output the buffer to the sink */
    if (m_passthrough && m_encoder)
      RunTranscodeStage();
    else
      RunOutputStage();

    /* unlock the sink, we don't need it anymore */
    m_sinkLock.LeaveShared();

    /* make sure we have enough room to fetch a frame */
    if(size > outSize)
    {
      /* allocate space for the samples */
      _aligned_free(out);
      out = (uint8_t *)_aligned_malloc(size, 16);
      outSize = size;
    }
    memset(out, 0, size);

    /* run the stream stage */
    bool restart = false;
    unsigned int mixed = RunStreamStage(channelCount, out, restart);

    /* if we are told to restart */
    if (restart)
    {
      /* stop all playing sounds so that we can re-open */
      CSingleLock lock(m_soundSampleLock);
      while(!m_playing_sounds.empty())
      {
        SoundState *ss = &(*m_playing_sounds.begin());
        m_playing_sounds.pop_front();
        ss->owner->ReleaseSamples();
      }
      lock.Leave();

      /* re-open the sink, and drop the frame */
      OpenSink();
    }
    else
    {
      /* otherwise process the frame */
      if (!m_rawPassthrough && mixed)
        RunNormalizeStage(channelCount, out, mixed);
    }

    /* re-lock the sink for the next loop */
    m_sinkLock.EnterShared();

    /* update the save values */
    channelCount = m_channelCount;
    size         = m_frameSize;

    /* buffer the samples into the output buffer */
    if (!m_reOpened)
      RunBufferStage(out);
  }

  /* free the frame storage */
  if(out)
    _aligned_free(out);
}

void CSoftAE::MixSounds(float *buffer, unsigned int samples)
{
  list<SoundState>::iterator itt;

  CSingleLock lock(m_soundSampleLock);
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

    float volume = ss->owner->GetVolume();// * 0.5f; /* 0.5 to normalize */
    unsigned int mixSamples = std::min(ss->sampleCount, samples);

    #ifdef __SSE__
      CSoftAE::SSEMulAddArray(buffer, ss->samples, volume, mixSamples);
   //   CSoftAE::SSEMulArray   (buffer, 0.5f, mixSamples);
    #else
      for(unsigned int i = 0; i < mixSamples; ++i)
        buffer[i] = (buffer[i] + (ss->samples[i] * volume)) * 0.5f;
    #endif

    ss->sampleCount -= mixSamples;
    ss->samples     += mixSamples;

    ++itt;
  }
}

void CSoftAE::FinalizeSamples(float *buffer, unsigned int samples)
{
  MixSounds(buffer, samples);

  #ifdef __SSE__
    CSoftAE::SSEMulArray(buffer, m_volume, samples);
    for(unsigned int i = 0; i < samples; ++i)
      buffer[i] = CAEUtil::SoftClamp(buffer[i]);
  #else
    for(unsigned int i = 0; i < samples; ++i)
      buffer[i] = CAEUtil::SoftClamp(buffer[i] * m_volume);
  #endif
}

inline void CSoftAE::RunOutputStage()
{
  unsigned int rSamples = m_sinkFormat.m_frames * m_sinkFormat.m_channelCount;
  unsigned int samples  = m_rawPassthrough ? rSamples : m_sinkFormat.m_frames * m_channelCount;

  /* this normally only loops once */
  while(m_bufferSamples >= samples)
  {
    int wroteFrames;

    /* if we are in raw passthrough we dont touch the samples */
    if (!m_rawPassthrough)
    {
      if(m_remappedSize < rSamples)
      {
        _aligned_free(m_remapped);
        m_remapped = (float *)_aligned_malloc(rSamples * sizeof(float), 16);
        m_remappedSize = rSamples;
      }

      m_remap.Remap((float *)m_buffer, m_remapped, m_sinkFormat.m_frames);
      FinalizeSamples(m_remapped, rSamples);

      if (m_convertFn)
      {
        unsigned int newSize = m_sinkFormat.m_frames * m_sinkFormat.m_frameSize;
        if(m_convertedSize < newSize)
        {
          _aligned_free(m_converted);
          m_converted = (uint8_t *)_aligned_malloc(newSize, 16);
          m_convertedSize = newSize;
        }
        m_convertFn(m_remapped, rSamples, m_converted);
        if (m_sink)
          wroteFrames = m_sink->AddPackets(m_converted, m_sinkFormat.m_frames);
        else
        {
          wroteFrames = m_sinkFormat.m_frames;
          DelayFrames();
        }
      }
      else
      {
        if (m_sink)
          wroteFrames = m_sink->AddPackets((uint8_t*)m_remapped, m_sinkFormat.m_frames);
        else
        {
          wroteFrames = m_sinkFormat.m_frames;
          DelayFrames();
        }
      }

      int wroteSamples = wroteFrames * m_channelCount;
      int bytesLeft    = (m_bufferSamples - wroteSamples) * m_bytesPerSample;
      memmove((float*)m_buffer, (float*)m_buffer + wroteSamples, bytesLeft);
      m_bufferSamples -= wroteSamples;
    }
    else
    {
      /* RAW output */
      unsigned int wroteFrames;
      uint8_t *rawBuffer = (uint8_t*)m_buffer;
      if (m_sink)
        wroteFrames = m_sink->AddPackets(rawBuffer, m_sinkFormat.m_frames);
      else
        wroteFrames = m_sinkFormat.m_frames;

      int wroteSamples = wroteFrames * m_channelCount;
      int bytesLeft    = (m_bufferSamples - wroteSamples) * m_bytesPerSample;
      memmove(rawBuffer, rawBuffer + (wroteSamples * m_bytesPerSample), bytesLeft);
      m_bufferSamples -= wroteSamples;
    }
  }
}

/*
  encodedPending is a flag to signify that the encoder has a packet ready for us
  we dont pick it up however until we have finished with our current encoded
  buffer, this allows our encoder to have a frame ready in advance.
*/
void CSoftAE::RunTranscodeStage()
{ 
  /* if there is not a pending block to pick up and we have enough samples to make one */
  if (!m_encodedPending && m_bufferSamples >= m_encoderFormat.m_frameSamples)
  {
    FinalizeSamples((float*)m_buffer, m_encoderFormat.m_frameSamples);    

    void *buffer;
    if (m_convertFn)
    {
      unsigned int newsize = m_encoderFormat.m_frames * m_encoderFormat.m_frameSize;
      if(m_convertedSize < newsize)
      {
        _aligned_free(m_converted);
        m_converted     = (uint8_t *)_aligned_malloc(newsize, 16);
        m_convertedSize = newsize;
      }
      m_convertFn((float*)m_buffer, m_encoderFormat.m_frames * m_encoderFormat.m_channelCount, m_converted);
      buffer = m_converted;
    }
    else
      buffer = m_buffer;

    int encodedFrames  = m_encoder->Encode((float*)buffer, m_encoderFormat.m_frames);
    int encodedSamples = encodedFrames * m_encoderFormat.m_channelCount;
    int bytesLeft      = (m_bufferSamples - encodedSamples) * m_bytesPerSample;

    memmove((float*)m_buffer, ((float*)m_buffer) + encodedSamples, bytesLeft);
    m_bufferSamples -= encodedSamples;
    m_encodedPending = true;
  }

  /* if there is a pending block, and we need to fetch it, then fetch it */
  if (m_encodedPending && m_encodedBufferPos == m_encodedBufferFrames)
  {
    uint8_t *packet;
    unsigned int size = m_encoder->GetData(&packet);
    if (m_encodedBufferSize < size)
    {
      delete[] m_encodedBuffer;
      m_encodedBuffer     = new uint8_t[size];
      m_encodedBufferSize = size;
    }

    memcpy(m_encodedBuffer, packet, size);
    m_encodedBufferPos    = 0;
    m_encodedBufferFrames = size / m_sinkFormat.m_frameSize;
    m_encodedPending      = false;
  }

  /* if we have data to write */
  if(m_encodedBufferPos < m_encodedBufferFrames)
  {
    unsigned int frames = m_encodedBufferFrames - m_encodedBufferPos;
    unsigned int write  = std::min(m_sinkFormat.m_frames, frames);
    int wrote;
    
    if (m_sink)
      wrote = m_sink->AddPackets(m_encodedBuffer + (m_encodedBufferPos * m_sinkFormat.m_frameSize), write);
    else
    {
      wrote = write;
      DelayFrames();
    }

    m_encodedBufferPos += wrote;
  }
}

unsigned int CSoftAE::RunStreamStage(unsigned int channelCount, void *out, bool &restart)
{
  CSingleLock streamLock(m_streamLock);

  if (m_rawPassthrough)
  {
    if (m_streams.empty()) return 0;
    CSoftAEStream *stream = m_streams.front();
    /* if the stream is to be deleted, or is not raw */
    if (stream->IsDestroyed() || !stream->IsRaw())
    {
      /* flag to have the sink re-started */
      restart = true;
      return 0;
    }

    uint8_t *frame = stream->GetFrame();
    if (!frame) return 0;

    memcpy(out, frame, m_sinkFormat.m_frameSize);
    return 1;
  }

  float *dst = (float*)out;
  unsigned int mixed = 0;
  list<CSoftAEStream*>::iterator itt;

  /* mix in any running streams */
  for(itt = m_streams.begin(); itt != m_streams.end();)
  {
    CSoftAEStream *stream = *itt;

    /* skip streams that are flagged for deletion */
    if (stream->IsDestroyed())
    {
      if (!stream->IsBusy())
      {
        itt = m_streams.erase(itt);
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

    float *frame = (float*)stream->GetFrame();
    if (!frame)
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

    float volume = stream->GetVolume() * stream->GetReplayGain();
    #ifdef __SSE__
    if (channelCount > 1)
      CSoftAE::SSEMulAddArray(dst, frame, volume, channelCount);
    else
    #endif
    {
      for(unsigned int i = 0; i < channelCount; ++i)
        dst[i] += frame[i] * volume;
    }

    ++mixed;
    ++itt;
  }

  return mixed;
}

inline void CSoftAE::RunNormalizeStage(unsigned int channelCount, void *out, unsigned int mixed)
{
  return;
  if (mixed <= 0) return;

  float *dst = (float*)out;
  float mul = 1.0f / mixed;
  #ifdef __SSE__
  if (channelCount > 1)
    CSoftAE::SSEMulArray(dst, mul, channelCount);
  else
  #endif
  {
    for(unsigned int i = 0; i < channelCount; ++i)
      dst[i] *= mul;
  }
}

inline void CSoftAE::RunBufferStage(void *out)
{
  if (m_rawPassthrough)
  {
    uint8_t *rawBuffer = (uint8_t*)m_buffer;
    memcpy(rawBuffer + (m_bufferSamples * m_bytesPerSample), out, m_sinkFormat.m_frameSize);
  }
  else
  {
    float *floatBuffer = (float*)m_buffer;
    memcpy(floatBuffer + m_bufferSamples, out, m_frameSize);
  }
  m_bufferSamples += m_channelCount;
}

