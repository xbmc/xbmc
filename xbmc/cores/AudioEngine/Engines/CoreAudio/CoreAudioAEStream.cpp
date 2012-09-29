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
#include "CoreAudioAEStream.h"

#include "xbmc/cores/AudioEngine/Interfaces/AE.h"
#include "xbmc/cores/AudioEngine/AEFactory.h"
#include "xbmc/cores/AudioEngine/Utils/AEUtil.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "settings/AdvancedSettings.h"
#include "utils/MathUtils.h"
#include "utils/log.h"



// typecast AE to CCoreAudioAE
#define AE (*(CCoreAudioAE*)CAEFactory::GetEngine())

void CheckOutputBufferSize(void **buffer, int *oldSize, int newSize)
{
  if (newSize > *oldSize)
  {
    if (*buffer)
      _aligned_free(*buffer);
    *buffer = _aligned_malloc(newSize, 16);
    *oldSize = newSize;
  }
  memset(*buffer, 0x0, *oldSize);
}

using namespace std;

template <class AudioDataType>
static inline void _Upmix(AudioDataType *input,
  unsigned int channelsInput, AudioDataType *output,
  unsigned int channelsOutput, unsigned int frames)
{
  unsigned int unused = channelsOutput - channelsInput;
  AudioDataType *_input  = input;
  AudioDataType *_output = output;

  for (unsigned int i = 0; i < frames; i++)
  {
    // get input channels
    for(unsigned int j = 0; j < channelsInput; j++)
      *_output++ = *_input++;
    // set unused channels
    for(unsigned int j = 0; j < unused; j++)
      *_output++ = 0;
  }
}

void CCoreAudioAEStream::Upmix(void *input,
  unsigned int channelsInput,  void *output,
  unsigned int channelsOutput, unsigned int frames, AEDataFormat dataFormat)
{
  // input channels must be less than output channels
  if (channelsInput >= channelsOutput)
    return;

  switch (CAEUtil::DataFormatToBits(dataFormat))
  {
    case 8:  _Upmix ( (unsigned char *) input, channelsInput, (unsigned char *) output, channelsOutput, frames ); break;
    case 16: _Upmix ( (short         *) input, channelsInput, (short         *) output, channelsOutput, frames ); break;
    case 32: _Upmix ( (float         *) input, channelsInput, (float         *) output, channelsOutput, frames ); break;
    default: _Upmix ( (int           *) input, channelsInput, (int           *) output, channelsOutput, frames ); break;
  }
}

CCoreAudioAEStream::CCoreAudioAEStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSamplerate, CAEChannelInfo channelLayout, unsigned int options) :
  m_outputUnit      (NULL ),
  m_valid           (false),
  m_delete          (false),
  m_volume          (1.0f ),
  m_rgain           (1.0f ),
  m_slave           (NULL ),
  m_convertFn       (NULL ),
  m_Buffer          (NULL ),
  m_convertBuffer   (NULL ),
  m_ssrc            (NULL ),
  m_draining        (false),
  m_AvgBytesPerSec  (0    ),
  m_audioCallback   (NULL ),
  m_fadeRunning     (false),
  m_frameSize       (0    ),
  m_doRemap         (true ),
  m_firstInput      (true )
{
  m_ssrcData.data_out             = NULL;

  m_rawDataFormat                 = dataFormat;
  m_StreamFormat.m_dataFormat     = dataFormat;
  m_StreamFormat.m_sampleRate     = sampleRate;
  m_StreamFormat.m_encodedRate    = 0;  //we don't support this
  m_StreamFormat.m_channelLayout  = channelLayout;
  m_chLayoutCountStream           = m_StreamFormat.m_channelLayout.Count();
  m_StreamFormat.m_frameSize      = (CAEUtil::DataFormatToBits(dataFormat) >> 3) * m_chLayoutCountStream;

  m_OutputFormat                  = AE.GetAudioFormat();
  m_chLayoutCountOutput           = m_OutputFormat.m_channelLayout.Count();

  //m_forceResample                 = (options & AESTREAM_FORCE_RESAMPLE) != 0;
  m_paused                        = (options & AESTREAM_PAUSED) != 0;

  m_vizRemapBufferSize            = m_remapBufferSize = /*m_resampleBufferSize = */ m_upmixBufferSize = m_convertBufferSize = 16*1024;
  m_convertBuffer                 = (float   *)_aligned_malloc(m_convertBufferSize,16);
  //m_resampleBuffer                = (float   *)_aligned_malloc(m_resampleBufferSize,16);
  m_upmixBuffer                   = (uint8_t *)_aligned_malloc(m_upmixBufferSize,16);
  m_remapBuffer                   = (uint8_t *)_aligned_malloc(m_remapBufferSize,16);
  m_vizRemapBuffer                = (uint8_t *)_aligned_malloc(m_vizRemapBufferSize,16);

  m_isRaw                         = COREAUDIO_IS_RAW(dataFormat);
}

CCoreAudioAEStream::~CCoreAudioAEStream()
{
  CloseConverter();

  m_delete = true;
  m_valid = false;

  InternalFlush();

  _aligned_free(m_convertBuffer); m_convertBuffer = NULL;
  //_aligned_free(m_resampleBuffer); m_resampleBuffer = NULL;
  _aligned_free(m_remapBuffer); m_remapBuffer = NULL;
  _aligned_free(m_vizRemapBuffer); m_vizRemapBuffer = NULL;

  delete m_Buffer; m_Buffer = NULL;

  /*
  if (m_resample)
  {
    _aligned_free(m_ssrcData.data_out);
    src_delete(m_ssrc);
    m_ssrc = NULL;
  }
  */

  CLog::Log(LOGDEBUG, "CCoreAudioAEStream::~CCoreAudioAEStream - Destructed");
}

void CCoreAudioAEStream::InitializeRemap()
{
  if (!m_isRaw)
  {
    if (m_OutputFormat.m_channelLayout != AE.GetChannelLayout())
    {
      m_OutputFormat            = AE.GetAudioFormat();
      m_chLayoutCountOutput     = m_OutputFormat.m_channelLayout.Count();
      m_OutputBytesPerSample    = (CAEUtil::DataFormatToBits(m_OutputFormat.m_dataFormat) >> 3);

      // re-init the remappers
      m_remap   .Initialize(m_StreamFormat.m_channelLayout, m_OutputFormat.m_channelLayout, false);
      m_vizRemap.Initialize(m_StreamFormat.m_channelLayout, CAEChannelInfo(AE_CH_LAYOUT_2_0), false, true);

      InternalFlush();
    }
  }
}

void CCoreAudioAEStream::ReinitConverter()
{
  CloseConverter();
  OpenConverter();
}

// The source logic is in the HAL. The only thing we have to do here
// is to allocate the convrter and set the direct input call.
void CCoreAudioAEStream::CloseConverter()
{
  // we have a converter, delete it
  if (m_outputUnit)
    m_outputUnit = (CAUOutputDevice *) AE.GetHAL()->DestroyUnit(m_outputUnit);

  // it is save to unregister any direct input. the HAL takes care about it.
  AE.GetHAL()->SetDirectInput(NULL, m_OutputFormat);
}

void CCoreAudioAEStream::OpenConverter()
{
  // we always allocate a converter
  // the HAL decides if we get converter.
  // if there is already a converter delete it.
  if (m_outputUnit)
    m_outputUnit = (CAUOutputDevice *) AE.GetHAL()->DestroyUnit(m_outputUnit);

  AEAudioFormat format = m_OutputFormat;

  format.m_sampleRate = m_StreamFormat.m_sampleRate;
  m_outputUnit = (CAUOutputDevice *) AE.GetHAL()->CreateUnit(this, format);

  // it is safe to register any direct input. the HAL takes care about it.
  AE.GetHAL()->SetDirectInput(this, m_OutputFormat);
}

void CCoreAudioAEStream::Initialize()
{
  if (m_valid)
    InternalFlush();

  m_OutputFormat = AE.GetAudioFormat();
  m_chLayoutCountOutput = m_OutputFormat.m_channelLayout.Count();

  if (m_rawDataFormat == AE_FMT_LPCM)
    m_OutputBytesPerSample = (CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3);
  else
    m_OutputBytesPerSample = (CAEUtil::DataFormatToBits(m_OutputFormat.m_dataFormat) >> 3);

  if (m_isRaw)
  {
    // we are raw, which means we need to work in the output format
    if (m_rawDataFormat != AE_FMT_LPCM)
    {
      m_StreamFormat = AE.GetAudioFormat();
      m_chLayoutCountStream = m_StreamFormat.m_channelLayout.Count();
    }
    m_StreamBytesPerSample = (CAEUtil::DataFormatToBits(m_StreamFormat.m_dataFormat) >> 3);
    m_doRemap = false;
  }
  else
  {
    if (!m_chLayoutCountStream)
    {
      m_valid = false;
      return;
    }
    // Work around a bug in TrueHD and DTSHD deliver
    if (m_StreamFormat.m_dataFormat == AE_FMT_TRUEHD || m_StreamFormat.m_dataFormat == AE_FMT_DTSHD)
      m_StreamBytesPerSample = (CAEUtil::DataFormatToBits(AE_FMT_S16NE) >> 3);
    else
      m_StreamBytesPerSample = (CAEUtil::DataFormatToBits(m_StreamFormat.m_dataFormat) >> 3);
    m_StreamFormat.m_frameSize = m_StreamBytesPerSample * m_chLayoutCountStream;
  }

  if (!m_isRaw)
  {
    if (!m_remap.Initialize(m_StreamFormat.m_channelLayout, m_OutputFormat.m_channelLayout, false))
    {
      m_valid = false;
      return;
    }

    m_doRemap  = m_chLayoutCountStream != 2;
  }

  if (!m_isRaw)
  {
    if (!m_vizRemap.Initialize(m_OutputFormat.m_channelLayout, CAEChannelInfo(AE_CH_LAYOUT_2_0), false, true))
    {
      m_valid = false;
      return;
    }
  }

  m_convert = m_StreamFormat.m_dataFormat != AE_FMT_FLOAT && !m_isRaw;
  //m_resample = false; //(m_StreamFormat.m_sampleRate != m_OutputFormat.m_sampleRate) && !m_isRaw;

  // if we need to convert, set it up
  if (m_convert)
  {
    // get the conversion function and allocate a buffer for the data
    CLog::Log(LOGDEBUG, "CCoreAudioAEStream::CCoreAudioAEStream - Converting from %s to AE_FMT_FLOAT", CAEUtil::DataFormatToStr(m_StreamFormat.m_dataFormat));
    m_convertFn = CAEConvert::ToFloat(m_StreamFormat.m_dataFormat);

    if (!m_convertFn)
      m_valid = false;
  }

  // if we need to resample, set it up
  /*
  if (m_resample)
  {
    int err;
    m_ssrc                   = src_new(SRC_SINC_MEDIUM_QUALITY, m_chLayoutCountStream, &err);
    m_ssrcData.src_ratio     = (double)m_OutputFormat.m_sampleRate / (double)m_StreamFormat.m_sampleRate;
    m_ssrcData.data_in       = m_convertBuffer;
    m_ssrcData.end_of_input  = 0;
  }
  */

  // m_AvgBytesPerSec is calculated based on the output format.
  // we have to keep in mind that we convert our data to the output format
  m_AvgBytesPerSec = m_OutputFormat.m_frameSize * m_OutputFormat.m_sampleRate;

  delete m_Buffer;
  m_Buffer = new CoreAudioRingBuffer(m_AvgBytesPerSec);

  m_fadeRunning = false;

  OpenConverter();

  m_valid = true;
}

void CCoreAudioAEStream::Destroy()
{
  m_valid  = false;
  m_delete = true;
  InternalFlush();
}

unsigned int CCoreAudioAEStream::AddData(void *data, unsigned int size)
{
  unsigned int frames   = size / m_StreamFormat.m_frameSize;
  unsigned int samples  = size / m_StreamBytesPerSample;
  uint8_t     *adddata  = (uint8_t *)data;
  unsigned int addsize  = size;
  unsigned int channelsInBuffer = m_chLayoutCountStream;

  if (!m_valid || size == 0 || data == NULL || !m_Buffer)
    return 0;

  // if the stream is draining
  if (m_draining)
  {
    // if the stream has finished draining, cork it
    if (m_Buffer && m_Buffer->GetReadSize() == 0)
      m_draining = false;
    else
      return 0;
  }

  // convert the data if we need to
  if (m_convert)
  {
    CheckOutputBufferSize((void **)&m_convertBuffer, &m_convertBufferSize, frames * channelsInBuffer  * m_OutputBytesPerSample);

    samples = m_convertFn(adddata, size / m_StreamBytesPerSample, m_convertBuffer);
    frames  = samples / channelsInBuffer;
    addsize = frames * channelsInBuffer * m_OutputBytesPerSample;
    adddata = (uint8_t *)m_convertBuffer;
  }
  else
  {
    samples = size / m_StreamBytesPerSample;
    adddata = (uint8_t *)data;
    addsize = size;
  }

  if (samples == 0)
    return 0;

  // resample it if we need to
  /*
  if (m_resample)
  {
    unsigned int resample_frames = samples / m_chLayoutCountStream;

    CheckOutputBufferSize((void **)&m_resampleBuffer, &m_resampleBufferSize,
                          resample_frames * std::ceil(m_ssrcData.src_ratio) * sizeof(float) * 2);

    m_ssrcData.input_frames   = resample_frames;
    m_ssrcData.output_frames  = resample_frames * std::ceil(m_ssrcData.src_ratio);
    m_ssrcData.data_in        = (float *)adddata;
    m_ssrcData.data_out       = m_resampleBuffer;

    if (src_process(m_ssrc, &m_ssrcData) != 0)
      return 0;

    frames    = m_ssrcData.output_frames_gen;
    samples   = frames * m_chLayoutCountStream;
    adddata   = (uint8_t *)m_ssrcData.data_out;
  }
  else
  {
    frames    = samples / m_chLayoutCountStream;
    samples   = frames * m_chLayoutCountStream;
  }
  */

  if (m_doRemap)
  {
    addsize = frames * m_OutputBytesPerSample * m_chLayoutCountOutput;
    CheckOutputBufferSize((void **)&m_remapBuffer, &m_remapBufferSize, addsize);

    // downmix/remap the data
    m_remap.Remap((float *)adddata, (float *)m_remapBuffer, frames);
    adddata = (uint8_t *)m_remapBuffer;
    channelsInBuffer = m_OutputFormat.m_channelLayout.Count();
  }

  // upmix the ouput to output channels
  if ( (!m_isRaw || m_rawDataFormat == AE_FMT_LPCM) && (m_chLayoutCountOutput > channelsInBuffer) )
  {
    frames = addsize / m_StreamFormat.m_frameSize;

    CheckOutputBufferSize((void **)&m_upmixBuffer, &m_upmixBufferSize, frames * m_chLayoutCountOutput  * sizeof(float));
    Upmix(adddata, channelsInBuffer, m_upmixBuffer, m_chLayoutCountOutput, frames, m_OutputFormat.m_dataFormat);
    adddata = m_upmixBuffer;
    addsize = frames * m_chLayoutCountOutput *  sizeof(float);
  }

  unsigned int total_ms_sleep = 0;
  unsigned int room = m_Buffer->GetWriteSize();
  while (addsize > room && !m_paused && total_ms_sleep < 100)
  {
    // we got deleted
    if (!m_valid || !m_Buffer || m_draining )
      return 0;

    unsigned int ms_sleep_time = (1000 * room) / m_AvgBytesPerSec;
    if (ms_sleep_time == 0)
      ms_sleep_time++;

    // sleep until we have space (estimated) or 1ms min
    Sleep(ms_sleep_time);
    total_ms_sleep += ms_sleep_time;

    room = m_Buffer->GetWriteSize();
  }

  if (addsize > room)
    size = 0;
  else
    m_Buffer->Write(adddata, addsize);

  return size;
}

unsigned int CCoreAudioAEStream::GetFrames(uint8_t *buffer, unsigned int size)
{
  // if we have been deleted
  if (!m_valid || m_delete || !m_Buffer || m_paused)
    return 0;

  unsigned int readsize = std::min(m_Buffer->GetReadSize(), size);
  m_Buffer->Read(buffer, readsize);

  if (!m_isRaw)
  {
    float *floatBuffer   = (float *)buffer;
    unsigned int samples = readsize / m_OutputBytesPerSample;

    // we have a frame, if we have a viz we need to hand the data to it.
    // Keep in mind that our buffer is already in output format.
    // So we remap output format to viz format !!!
    if (m_OutputFormat.m_dataFormat == AE_FMT_FLOAT)
    {
      // TODO : Why the hell is vizdata limited ?
      unsigned int frames         = samples / m_chLayoutCountOutput;
      unsigned int samplesClamped = (samples > 512) ? 512 : samples;
      if (samplesClamped)
      {
        // Viz channel count is 2
        CheckOutputBufferSize((void **)&m_vizRemapBuffer, &m_vizRemapBufferSize, frames * 2 * sizeof(float));

        m_vizRemap.Remap(floatBuffer, (float*)m_vizRemapBuffer, frames);
        if (m_audioCallback)
          m_audioCallback->OnAudioData((float *)m_vizRemapBuffer, samplesClamped);
      }
    }

    // if we are fading
    if (m_fadeRunning)
    {
      // TODO: check if we correctly respect the amount of our blockoperation
      m_volume += (m_fadeStep * ((float)readsize / (float)m_OutputFormat.m_frameSize));
      m_volume  = std::min(1.0f, std::max(0.0f, m_volume));
      if (m_fadeDirUp)
      {
        if (m_volume >= m_fadeTarget)
          m_fadeRunning = false;
      }
      else
      {
        if (m_volume <= m_fadeTarget)
          m_fadeRunning = false;
      }

#ifdef __SSE__
      CAEUtil::SSEMulArray(floatBuffer, m_volume, samples);
#else
      for(unsigned int i = 0; i < samples; i++)
        floatBuffer[i] *= m_volume;
#endif
      CAEUtil::ClampArray(floatBuffer, samples);
    }
  }

  return readsize;
}

const unsigned int CCoreAudioAEStream::GetFrameSize() const
{
  return m_OutputFormat.m_frameSize;
}

unsigned int CCoreAudioAEStream::GetSpace()
{
  if (!m_valid || m_draining)
    return 0;

  return m_Buffer->GetWriteSize();
}

double CCoreAudioAEStream::GetDelay()
{
  if (m_delete || !m_Buffer)
    return 0.0f;

  double delay = (double)(m_Buffer->GetReadSize()) / (double)m_AvgBytesPerSec;
  delay += AE.GetDelay();

  return delay;
}

bool CCoreAudioAEStream::IsBuffering()
{
  return m_Buffer->GetReadSize() == 0;
}

double CCoreAudioAEStream::GetCacheTime()
{
  if (m_delete || !m_Buffer)
    return 0.0f;

  return (double)(m_Buffer->GetReadSize()) / (double)m_AvgBytesPerSec;
}

double CCoreAudioAEStream::GetCacheTotal()
{
  if (m_delete || !m_Buffer)
    return 0.0f;

  return (double)m_Buffer->GetMaxSize() / (double)m_AvgBytesPerSec;
}


bool CCoreAudioAEStream::IsPaused()
{
  return m_paused;
}

bool CCoreAudioAEStream::IsDraining()
{
  return m_draining;
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

bool CCoreAudioAEStream::IsDrained()
{
  return m_Buffer->GetReadSize() <= 0;
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
  // reset the resampler
  /*
  if (m_resample) {
    m_ssrcData.end_of_input = 0;
    src_reset(m_ssrc);
  }
  */

  // Read the buffer empty to avoid Reset
  // Reset is not lock free.
  if (m_Buffer)
  {
    unsigned int readsize = m_Buffer->GetReadSize();
    if (readsize)
    {
      uint8_t *buffer = (uint8_t *)_aligned_malloc(readsize, 16);
      m_Buffer->Read(buffer, readsize);
      _aligned_free(buffer);
    }

    // if we are draining and are out of packets, tell the slave to resume
    if (m_draining && m_slave)
    {
      m_slave->Resume();
      m_slave = NULL;
    }
  }

  //if (m_Buffer)
  //  m_Buffer->Reset();
}

const unsigned int CCoreAudioAEStream::GetChannelCount() const
{
  return m_chLayoutCountStream;
}

const unsigned int CCoreAudioAEStream::GetSampleRate() const
{
  return m_StreamFormat.m_sampleRate;
}

const unsigned int CCoreAudioAEStream::GetEncodedSampleRate() const
{
  return m_StreamFormat.m_encodedRate;
}

const enum AEDataFormat CCoreAudioAEStream::GetDataFormat() const
{
  return m_StreamFormat.m_dataFormat;
}

const bool CCoreAudioAEStream::IsRaw() const
{
  return m_isRaw;
}

double CCoreAudioAEStream::GetResampleRatio()
{
  /*
  if (!m_resample)
   return 1.0f;

  double ret = m_ssrcData.src_ratio;
  return ret;
  */

  return 1.0f;
}

bool CCoreAudioAEStream::SetResampleRatio(double ratio)
{
  return false;
  /*
  if (!m_resample)
    return;

  src_set_ratio(m_ssrc, ratio);
  m_ssrcData.src_ratio = ratio;
  */
}

void CCoreAudioAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_audioCallback = pCallback;
  if (m_audioCallback)
    m_audioCallback->OnInitialize(2, m_StreamFormat.m_sampleRate, 32);
}

void CCoreAudioAEStream::UnRegisterAudioCallback()
{
  m_audioCallback = NULL;
}

void CCoreAudioAEStream::FadeVolume(float from, float target, unsigned int time)
{
  if (m_isRaw)
  {
    m_fadeRunning = false;
  }
  else
  {
    float delta   = target - from;
    m_fadeDirUp   = target > from;
    m_fadeTarget  = target;
    m_fadeStep    = delta / (((float)m_OutputFormat.m_sampleRate / 1000.0f) * (float)time);
    m_fadeRunning = true;
  }
}

bool CCoreAudioAEStream::IsFading()
{
  return m_fadeRunning;
}

void CCoreAudioAEStream::RegisterSlave(IAEStream *stream)
{
  m_slave = stream;
}

OSStatus CCoreAudioAEStream::Render(AudioUnitRenderActionFlags* actionFlags,
  const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList)
{
  OSStatus ret = OnRender(actionFlags, pTimeStamp, busNumber, frameCount, pBufList);
  return ret;
}

OSStatus CCoreAudioAEStream::OnRender(AudioUnitRenderActionFlags *ioActionFlags,
  const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  // if we have no valid data output silence
  if (!m_valid || m_delete || !m_Buffer || m_firstInput || m_paused)
  {
  	for (UInt32 i = 0; i < ioData->mNumberBuffers; i++)
      bzero(ioData->mBuffers[i].mData, ioData->mBuffers[i].mDataByteSize);
    if (ioActionFlags)
      *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
    m_firstInput = false;
    return noErr;
  }

  unsigned int size = inNumberFrames * m_OutputFormat.m_frameSize;
  //unsigned int size = inNumberFrames * m_StreamFormat.m_frameSize;

  // the index is important if we run encoded
  unsigned int outputBufferIndex = AE.GetHAL()->GetBufferIndex();

  ioData->mBuffers[outputBufferIndex].mDataByteSize  = GetFrames(
    (uint8_t*)ioData->mBuffers[outputBufferIndex].mData, size);
  if (!ioData->mBuffers[outputBufferIndex].mDataByteSize && ioActionFlags)
    *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;

  return noErr;
}
