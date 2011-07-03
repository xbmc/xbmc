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

#include "system.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "DllAvCore.h"

#include "AEFactory.h"
#include "AEUtil.h"

#include "CoreAudioAE.h"
#include "CoreAudioAEStream.h"

#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "MathUtils.h"

/* typecast AE to CCoreAudioAE */
#define AE (*(CCoreAudioAE*)AE.GetEngine())

using namespace std;

CCoreAudioAEStream::CCoreAudioAEStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options) :
  m_convertBuffer   (NULL ),
  m_valid           (false),
  m_delete          (false),
  m_volume          (1.0f ),
  m_rgain           (1.0f ),
  m_convertFn       (NULL ),
  m_ssrc            (NULL ),
  m_draining        (false),
  m_disableCallbacks(false),
  m_cbDataFunc      (NULL ),
  m_cbDrainFunc     (NULL ),
  m_cbDataArg       (NULL ),
  m_cbDrainArg      (NULL ),
  m_inDataFunc      (false),
  m_inDrainFunc     (false),
  m_audioCallback   (NULL ),
  m_cbFreeFunc      (NULL ),
  m_AvgBytesPerSec  (0    ),
  m_Buffer          (NULL)
{
  m_ssrcData.data_out             = NULL;

  m_StreamFormat.m_dataFormat     = dataFormat;
  m_StreamFormat.m_sampleRate     = sampleRate;
  m_StreamFormat.m_channelCount   = channelCount;
  m_StreamFormat.m_channelLayout  = channelLayout;
  
  m_OutputFormat                  = m_StreamFormat;
    
  m_freeOnDrain                   = (options & AESTREAM_FREE_ON_DRAIN) != 0;
  m_forceResample                 = (options & AESTREAM_FORCE_RESAMPLE) != 0;
  m_paused                        = (options & AESTREAM_PAUSED) != 0;

  m_vizRemapBufferSize            = m_remapBufferSize = m_resampleBufferSize = m_convertBufferSize = 16*1024;
  m_convertBuffer                 = (float*)_aligned_malloc(m_convertBufferSize,16);
  m_resampleBuffer                = (float*)_aligned_malloc(m_resampleBufferSize,16);
  m_remapBuffer                   = (uint8_t *)_aligned_malloc(m_remapBufferSize,16); 
  m_vizRemapBuffer                = (uint8_t*)_aligned_malloc(m_vizRemapBufferSize,16); 
}

CCoreAudioAEStream::~CCoreAudioAEStream()
{
  CSingleLock StreamLock(m_MutexStream);

  InternalFlush();
  
  _aligned_free(m_convertBuffer);
  _aligned_free(m_resampleBuffer);
  _aligned_free(m_remapBuffer);
  _aligned_free(m_vizRemapBuffer);
  
  if (m_cbFreeFunc)
    m_cbFreeFunc(this, m_cbFreeArg, 0);
  
  if(m_Buffer)
    delete m_Buffer;
  
  StreamLock.Leave();
    
  CLog::Log(LOGDEBUG, "CCoreAudioAEStream::~CCoreAudioAEStream - Destructed");
}

void CCoreAudioAEStream::InitializeRemap()
{
  CSingleLock StreamLock(m_MutexStream);
  
  m_OutputFormat = AE.GetAudioFormat();
  
  if (!COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat))
  {
    /* re-init the remappers */
    m_remap   .Initialize(m_StreamFormat.m_channelLayout, m_OutputFormat.m_channelLayout, false);
    m_vizRemap.Initialize(m_StreamFormat.m_channelLayout, CAEUtil::GetStdChLayout(AE_CH_LAYOUT_2_0), false, true);

    InternalFlush();
  }
  
  StreamLock.Leave();
}

void CCoreAudioAEStream::Initialize(AEAudioFormat &outputFormat)
{
  CSingleLock StreamLock(m_MutexStream);
  if (m_valid)
  {
    InternalFlush();
  }
  
  m_OutputFormat = outputFormat;

  m_OutputBytesPerSample          = (CAEUtil::DataFormatToBits(m_OutputFormat.m_dataFormat) >> 3);

  if(COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat))
  {
    m_StreamBytesPerSample        = (CAEUtil::DataFormatToBits(m_OutputFormat.m_dataFormat) >> 3);
    m_StreamFormat.m_frameSize    = m_OutputFormat.m_frameSize;
  }
  else
  {
    // no channel layout provided, so guess
    if (!m_StreamFormat.m_channelLayout)
    {
      m_StreamFormat.m_channelLayout = CAEUtil::GuessChLayout(m_StreamFormat.m_channelCount);
      if (!m_StreamFormat.m_channelLayout)
      {
        m_valid = false;
        StreamLock.Leave();
        return;
      }
    }
    /* Work around a bug in TrueHD and DTSHD deliver */
    if(m_StreamFormat.m_dataFormat == AE_FMT_TRUEHD || m_StreamFormat.m_dataFormat == AE_FMT_DTSHD)
    {
       m_StreamBytesPerSample        = (CAEUtil::DataFormatToBits(AE_FMT_S16NE) >> 3);
    }
    else
    {
      m_StreamBytesPerSample        = (CAEUtil::DataFormatToBits(m_StreamFormat.m_dataFormat) >> 3);
    }
    m_StreamFormat.m_frameSize    = m_StreamBytesPerSample * m_StreamFormat.m_channelCount;
  }

  if (!COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat))
  {
    if (
      !m_remap.Initialize(m_StreamFormat.m_channelLayout, m_OutputFormat.m_channelLayout, false) ||
      !m_vizRemap.Initialize(m_OutputFormat.m_channelLayout, CAEUtil::GetStdChLayout(AE_CH_LAYOUT_2_0), false, true))
    {
      m_valid = false;
      StreamLock.Leave();
      return;
    }
  }

  m_resample      = (/*m_forceResample || */ m_StreamFormat.m_sampleRate != m_OutputFormat.m_sampleRate) && !COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat);
  m_convert       = m_StreamFormat.m_dataFormat != AE_FMT_FLOAT && !COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat);

  /* if we need to convert, set it up */
  if (m_convert)
  {
    /* get the conversion function and allocate a buffer for the data */
    CLog::Log(LOGDEBUG, "CCoreAudioAEStream::CCoreAudioAEStream - Converting from %s to AE_FMT_FLOAT", CAEUtil::DataFormatToStr(m_StreamFormat.m_dataFormat));
    m_convertFn = CAEConvert::ToFloat(m_StreamFormat.m_dataFormat);
    
    if (!m_convertFn)
      m_valid         = false;
  }
  
  /* if we need to resample, set it up */
  if (m_resample)
  {
    int err;
    m_ssrc                   = src_new(SRC_SINC_MEDIUM_QUALITY, m_StreamFormat.m_channelCount, &err);
    m_ssrcData.src_ratio     = (double)m_OutputFormat.m_sampleRate / (double)m_StreamFormat.m_sampleRate;
    m_ssrcData.data_in       = m_convertBuffer;
    m_ssrcData.end_of_input  = 0;
  }

  m_AvgBytesPerSec =  m_OutputFormat.m_frameSize * m_OutputFormat.m_sampleRate;

  if(m_Buffer)
    delete m_Buffer;
  
  m_Buffer = new CoreAudioRingBuffer(m_AvgBytesPerSec);

  /* print input output channels */
  CLog::Log(LOGDEBUG, "==[Stream input channels]==");
  CStdString s;
  for(int i = 0; m_StreamFormat.m_channelLayout[i] != AE_CH_NULL; i++)
  {
    s.append(CAEUtil::GetChName( m_StreamFormat.m_channelLayout[i]) + CStdString(" ,"));
  }
  CLog::Log(LOGDEBUG, "%s", s.substr(0, s.length() - 1).c_str());
  CLog::Log(LOGDEBUG, "====================\n");
  
  CLog::Log(LOGDEBUG, "==[Stream output channels]==");
  s = "";
  for(int i = 0; m_OutputFormat.m_channelLayout[i] != AE_CH_NULL; i++)
  {
    s.append(CAEUtil::GetChName( m_OutputFormat.m_channelLayout[i]) + CStdString(" ,"));
  }
  CLog::Log(LOGDEBUG, "%s", s.substr(0, s.length() - 1).c_str());
  CLog::Log(LOGDEBUG, "====================\n");

  StreamLock.Leave();
  m_valid = true;
}

void CCoreAudioAEStream::Destroy()
{
  CSingleLock StreamLock(m_MutexStream);

  m_valid       = false;
  m_delete      = true;
  InternalFlush();
  
  StreamLock.Leave();
}

void CCoreAudioAEStream::DisableCallbacks(bool free /* = true */)
{
  m_disableCallbacks = true;
  while(IsBusy())
    Sleep(100);

  CSingleLock StreamLock(m_MutexStream);

  m_cbDataFunc  = NULL;
  m_cbDrainFunc = NULL;
  if (free)
    m_cbFreeFunc = NULL;
  
  StreamLock.Leave();
}

void CCoreAudioAEStream::SetDataCallback(AECBFunc *cbFunc, void *arg)
{
  CSingleLock StreamLock(m_MutexStream);

  m_cbDataFunc = cbFunc;
  m_cbDataArg  = arg;
  
  StreamLock.Leave();
}

void CCoreAudioAEStream::SetDrainCallback(AECBFunc *cbFunc, void *arg)
{
  CSingleLock StreamLock(m_MutexStream);

  m_cbDrainFunc = cbFunc;
  m_cbDrainArg  = arg;

  StreamLock.Leave();
}

void CCoreAudioAEStream::SetFreeCallback(AECBFunc *cbFunc, void *arg)
{
  CSingleLock StreamLock(m_MutexStream);

  m_cbFreeFunc = cbFunc;
  m_cbFreeArg  = arg;
  
  StreamLock.Leave();
}

unsigned int CCoreAudioAEStream::AddData(void *data, unsigned int size)
{
  unsigned int frames   = size / m_StreamFormat.m_frameSize;
  unsigned int samples  = size / m_StreamBytesPerSample;
  uint8_t     *adddata  = (uint8_t *)data;
  unsigned int addsize  = size;

  if (!m_valid || size == 0 || data == NULL || m_draining || m_delete || !m_Buffer)
  {
    return 0; 
  }

  //CSingleLock StreamLock(m_MutexStream);
  unsigned int room = m_Buffer->GetWriteSize();
     
  /* convert the data if we need to */
  if (m_convert)
  {
    CheckOutputBufferSize((void **)&m_convertBuffer, &m_convertBufferSize, frames * m_StreamFormat.m_channelCount * sizeof(float) * 2);

    samples     = m_convertFn(adddata, size / m_StreamBytesPerSample, m_convertBuffer);
    adddata     = (uint8_t *)m_convertBuffer;
  }
  else
  {
    samples     = size / m_StreamBytesPerSample;
    adddata     = (uint8_t *)data;
  }

  if (samples == 0)
  {
    //StreamLock.Leave();
    return 0;
  }
  
  /* resample it if we need to */
  if (m_resample)
  {
    unsigned int resample_frames = samples / m_StreamFormat.m_channelCount;
    
    CheckOutputBufferSize((void **)&m_resampleBuffer, &m_resampleBufferSize, 
                          resample_frames * MathUtils::ceil_int(m_ssrcData.src_ratio) * sizeof(float) * 2);
    
    m_ssrcData.input_frames   = resample_frames;
    m_ssrcData.output_frames  = resample_frames * MathUtils::ceil_int(m_ssrcData.src_ratio);
    m_ssrcData.data_in        = (float *)adddata;
    m_ssrcData.data_out       = m_resampleBuffer;
        
    if (src_process(m_ssrc, &m_ssrcData) != 0) 
    {
      return 0;
    }
    
    frames    = m_ssrcData.output_frames_gen;    
    samples   = frames * m_StreamFormat.m_channelCount;
    adddata   = (uint8_t *)m_ssrcData.data_out;
  }
  else
  {
    frames    = samples / m_StreamFormat.m_channelCount;
    samples   = frames * m_StreamFormat.m_channelCount;
  }

  if (!COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat))
  {
    addsize = frames * m_OutputBytesPerSample * m_OutputFormat.m_channelCount;
    
    CheckOutputBufferSize((void **)&m_remapBuffer, &m_remapBufferSize, addsize * 2);
        
    // downmix/remap the data
    m_remap.Remap((float *)adddata, (float *)m_remapBuffer, frames);
    adddata   = (uint8_t *)m_remapBuffer;
    //addsize   = frames * m_OutputFormat.m_frameSize;
  }

  //unsigned int copy = std::min(addsize, room);
  room = m_Buffer->GetWriteSize();
      
  if(addsize > room)
  {
    //CLog::Log(LOGDEBUG, "CCoreAudioAEStream::AddData failed : free size %d add size %d", room, addsize);
    size = 0;
  }
  else 
  {
    m_Buffer->Write(adddata, addsize);
  }
  //StreamLock.Leave();

  return size;    
}

unsigned int CCoreAudioAEStream::GetFrames(uint8_t *buffer, unsigned int size)
{  
  /* if we have been deleted */
  if (!m_valid || m_delete || !m_Buffer)
  {
    return 0;
  }
    
  /* we are draining */
  if (m_draining)
  {
    /* if we are draining trigger the callback function */
    if (m_cbDrainFunc && !m_disableCallbacks)
    {
      m_inDrainFunc = true;
      m_cbDrainFunc(this, m_cbDrainArg, 0);
      m_cbDrainFunc = NULL;
      m_inDrainFunc = false;
    }
    return 0;
  }
  
  unsigned int readsize = m_Buffer->GetReadSize();
  if(readsize < size)
  {
    /* otherwise ask for more data */
    if (m_cbDataFunc && !m_disableCallbacks)
    {
      m_inDataFunc = true;
      unsigned int samples = m_Buffer->GetWriteSize() / m_StreamBytesPerSample / 2;
      m_cbDataFunc(this, m_cbDataArg, samples);
      m_inDataFunc = false;
    }
  }
  
  readsize = std::min(m_Buffer->GetReadSize(), size);  
  
  m_Buffer->Read(buffer, readsize);
 
#if 0
  if(!m_draining && (readsize < size))
  {
    /* otherwise ask for more data */
    if (m_cbDataFunc && !m_disableCallbacks)
    {
      m_inDataFunc = true;
      unsigned int samples = m_Buffer->GetWriteSize() / m_StreamBytesPerSample / 2;
      m_cbDataFunc(this, m_cbDataArg, samples);
      m_inDataFunc = false;
    }
  }
#endif

  /* we have a frame, if we have a viz we need to hand the data to it.
     On iOS we do not have vizualisation. Keep in mind that our buffer
     is already in output format. So we remap output format to viz format !!!*/
#ifndef __arm__
  if(!COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat) && (m_OutputFormat.m_dataFormat == AE_FMT_FLOAT))
  {
    // TODO : Why the hell is vizdata limited ?
    unsigned int samples   = readsize / m_OutputBytesPerSample;
    unsigned int frames    = samples / m_OutputFormat.m_channelCount;

    if(samples) {
      // Viz channel count is 2
      CheckOutputBufferSize((void **)&m_vizRemapBuffer, &m_vizRemapBufferSize, frames * 2 * sizeof(float));
      
      samples  = (samples > 512) ? 512 : samples;
      
      m_vizRemap.Remap((float*)buffer, (float*)m_vizRemapBuffer, frames);
      if (m_audioCallback)
      {
        m_audioCallback->OnAudioData((float *)m_vizRemapBuffer, samples);
      }
    }
  }
#endif
  
  return readsize;  
}

unsigned int CCoreAudioAEStream::GetFrameSize()
{
  return m_OutputFormat.m_frameSize;
}

float CCoreAudioAEStream::GetDelay()
{
  if (m_delete || !m_Buffer) return 0.0f;
  
  float delay = (float)m_Buffer->GetReadSize() / (float)m_AvgBytesPerSec;
  
  return AE.GetDelay() + delay;
}

float CCoreAudioAEStream::GetCacheTime()
{
  if (m_delete || !m_Buffer) return 0.0f;
  
  return (float)m_Buffer->GetReadSize() / (float)m_AvgBytesPerSec;
}

float CCoreAudioAEStream::GetCacheTotal()
{
  if (m_delete || !m_Buffer) return 0.0f;
  
  return (float)m_Buffer->GetMaxSize() / (float)m_AvgBytesPerSec;
}

bool CCoreAudioAEStream::IsPaused()
{
  return m_paused;
}

bool CCoreAudioAEStream::IsDraining () 
{
  return m_draining;
}

bool CCoreAudioAEStream::IsFreeOnDrain()
{ 
  return m_freeOnDrain;
}

bool CCoreAudioAEStream::IsDestroyed()
{ 
  return m_delete;
}

bool CCoreAudioAEStream::IsValid() 
{ 
  return m_valid;
}

void CCoreAudioAEStream::Pause()
{
  m_paused = true;
}

void CCoreAudioAEStream::Resume()
{
  m_paused = false;
}

void CCoreAudioAEStream::Drain()
{
  m_draining = true;
}

void CCoreAudioAEStream::Flush()
{
  InternalFlush();
}

float CCoreAudioAEStream::GetVolume()
{
  return m_volume;
}

float CCoreAudioAEStream::GetReplayGain()
{ 
  return m_rgain;
}

void  CCoreAudioAEStream::SetVolume(float volume)
{
  m_volume = std::max( 0.0f, std::min(1.0f, volume));
}

void  CCoreAudioAEStream::SetReplayGain(float factor)
{
  m_rgain  = std::max(-1.0f, std::max(1.0f, factor));
}

void CCoreAudioAEStream::InternalFlush()
{
  /* reset the resampler */
  if (m_resample) {
    m_ssrcData.end_of_input = 0;
    src_reset(m_ssrc);
  }

  if(m_Buffer)
    m_Buffer->Reset();
}

void CCoreAudioAEStream::AppendPostProc(IAEPostProc *pp)
{
}

void CCoreAudioAEStream::PrependPostProc(IAEPostProc *pp)
{
}

void CCoreAudioAEStream::RemovePostProc(IAEPostProc *pp)
{
}

unsigned int CCoreAudioAEStream::GetChannelCount()
{
  return m_StreamFormat.m_channelCount;
}

unsigned int CCoreAudioAEStream::GetSampleRate()
{
  return m_StreamFormat.m_sampleRate;
}

enum AEDataFormat CCoreAudioAEStream::GetDataFormat()
{
  return m_StreamFormat.m_dataFormat;
}

bool CCoreAudioAEStream::IsRaw()
{
  return COREAUDIO_IS_RAW(m_StreamFormat.m_dataFormat);
}

double CCoreAudioAEStream::GetResampleRatio()
{
  if (!m_resample)
    return 1.0f;

  double ret = m_ssrcData.src_ratio;
  return ret;
}

void CCoreAudioAEStream::SetResampleRatio(double ratio)
{
  if (!m_resample)
    return;

  src_set_ratio(m_ssrc, ratio);
  m_ssrcData.src_ratio = ratio;
}

void CCoreAudioAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_audioCallback = pCallback;
  if (m_audioCallback)
    m_audioCallback->OnInitialize(2, m_StreamFormat.m_sampleRate, 32);
}

void CCoreAudioAEStream::UnRegisterAudioCallback()
{
  CSingleLock StreamLock(m_MutexStream);

  m_audioCallback = NULL;
  
  StreamLock.Leave();
}

void CCoreAudioAEStream::SetFreeOnDrain()
{
  m_freeOnDrain = true;
}

bool CCoreAudioAEStream::IsBusy()
{
  return (m_inDataFunc || m_inDrainFunc);
}
