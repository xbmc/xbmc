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

#include "AEWAVLoader.h"


#include "system.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"
#include "filesystem/File.h"
#include "URL.h"
#include <samplerate.h>

#include "AEConvert.h"
#include "AEUtil.h"
#include "AERemap.h"

#ifdef TARGET_WINDOWS
#pragma comment(lib, "libsamplerate-0.lib")
#endif 

typedef struct
{
  char     chunk_id[4];
  uint32_t chunksize;
} WAVE_CHUNK;

CAEWAVLoader::CAEWAVLoader() :
  m_valid             (false),
  m_sampleRate        (0    ),
  m_outputSampleRate  (0    ),
  m_frameCount        (0    ),
  m_outputFrameCount  (0    ),
  m_sampleCount       (0    ),
  m_outputSampleCount (0    ),
  m_samples           (NULL ),
  m_outputSamples     (NULL )
{
}

CAEWAVLoader::~CAEWAVLoader()
{
  UnLoad();
}

bool CAEWAVLoader::Load(const std::string &filename)
{
  UnLoad();

  m_filename = filename;

  XFILE::CFile file;
  if (!file.Open(m_filename))
  {
    CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - Failed to create loader: %s", m_filename.c_str());
    return false;
  }

  struct __stat64 st;
  if (file.Stat(&st) < 0)
  {
    CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - Failed to stat file: %s", m_filename.c_str());
    return false;
  }

  bool isRIFF = false;
  bool isWAVE = false;
  bool isFMT  = false;
  bool isPCM  = false;
  bool isDATA = false;

  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;

  WAVE_CHUNK chunk;
  while (file.Read(&chunk, sizeof(chunk)) == sizeof(chunk))
  {
    chunk.chunksize = Endian_SwapLE32(chunk.chunksize);

    /* if its the RIFF header */
    if (!isRIFF && memcmp(chunk.chunk_id, "RIFF", 4) == 0)
    {
      isRIFF = true;

      /* work around invalid chunksize, I have seen this in one file so far (shutter.wav) */
      if (chunk.chunksize == st.st_size)
        chunk.chunksize -= 8;

      /* sanity check on the chunksize */
      if (chunk.chunksize > st.st_size - 8)
      {
        CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - Corrupt WAV header: %s", m_filename.c_str());
        file.Close();
        return false;
      }

      /* we only support WAVE files */
      char format[4];
      if (file.Read(&format, 4) != 4)
        break;
      isWAVE = memcmp(format, "WAVE", 4) == 0;
      if (!isWAVE)
        break;
    }
    /* if its the fmt section */
    else if (!isFMT && memcmp(chunk.chunk_id, "fmt ", 4) == 0)
    {
      isFMT = true;
      if (chunk.chunksize < 16)
        break;
      uint16_t format;
      if (file.Read(&format, sizeof(format)) != sizeof(format))
        break;
      format = Endian_SwapLE16(format);
      if (format != WAVE_FORMAT_PCM)
        break;

      uint16_t channelCount;
      if (file.Read(&channelCount , 2) != 2)
        break;
      if (file.Read(&sampleRate   , 4) != 4)
        break;
      if (file.Read(&byteRate     , 4) != 4)
        break;
      if (file.Read(&blockAlign   , 2) != 2)
        break;
      if (file.Read(&bitsPerSample, 2) != 2)
        break;

      channelCount = Endian_SwapLE16(channelCount);
      /* TODO: support > 2 channel count */
      if (channelCount > 2)
        break;

      static AEChannel layouts[][3] = {
        {AE_CH_FC, AE_CH_NULL},
        {AE_CH_FL, AE_CH_FR, AE_CH_NULL}
      };

      m_channels     = layouts[channelCount - 1];
      m_sampleRate   = Endian_SwapLE32(sampleRate   );
      byteRate       = Endian_SwapLE32(byteRate     );
      blockAlign     = Endian_SwapLE16(blockAlign   );
      bitsPerSample  = Endian_SwapLE16(bitsPerSample);
      isPCM          = true;

      if (chunk.chunksize > 16)
        file.Seek(chunk.chunksize - 16, SEEK_CUR);
    }
    /* if we have the PCM info and its the DATA section */
    else if (isPCM && !isDATA && memcmp(chunk.chunk_id, "data", 4) == 0)
    {
       unsigned int bytesPerSample = bitsPerSample >> 3;
       m_sampleCount = chunk.chunksize / bytesPerSample;
       m_frameCount  = m_sampleCount / m_channels.Count();
       isDATA        = m_frameCount > 0;

       /* get the conversion function */
       CAEConvert::AEConvertToFn convertFn;
       switch (bitsPerSample)
       {
         case 8 : convertFn = CAEConvert::ToFloat(AE_FMT_U8   ); break;
         case 16: convertFn = CAEConvert::ToFloat(AE_FMT_S16LE); break;
         case 32: convertFn = CAEConvert::ToFloat(AE_FMT_S32LE); break;
         default:
           CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - Unsupported data format in wav: %s", m_filename.c_str());
           file.Close();
           return false;
       }

       /* read in each sample */
       unsigned int size = bytesPerSample * m_sampleCount;
       m_samples    = (float*   )_aligned_malloc(sizeof(float) * m_sampleCount, 16);
       uint8_t *raw = (uint8_t *)_aligned_malloc(size, 16);
       if (file.Read(raw, size) != size)
       {
         CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - WAV data shorter then expected: %s", m_filename.c_str());
         _aligned_free(m_samples);
         _aligned_free(raw);
         m_samples = NULL;
         file.Close();
         return false;
       }

       /* convert the samples to float */
       convertFn(raw, m_sampleCount, m_samples);
       _aligned_free(raw);
    }
    else
    {
      /* skip any unknown sections */
      file.Seek(chunk.chunksize, SEEK_CUR);
    }
  }

  if (!isRIFF || !isWAVE || !isFMT || !isPCM || !isDATA || m_sampleCount == 0)
  {
    CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - Invalid, or un-supported WAV file: %s", m_filename.c_str());
    file.Close();
    return false;
  }

  /* close the file as we have the samples now */
  file.Close();

  m_outputChannels     = m_channels;
  m_outputSampleRate   = m_sampleRate;
  m_outputSamples      = m_samples;
  m_outputSampleCount  = m_sampleCount;
  m_outputFrameCount   = m_frameCount;

  CLog::Log(LOGINFO, "CAEWAVLoader::Initialize - Sound Loaded: %s", m_filename.c_str());
  m_valid = true;
  return true;
}

bool CAEWAVLoader::Initialize(unsigned int resampleRate, CAEChannelInfo channelLayout, enum AEStdChLayout stdChLayout/* = AE_CH_LAYOUT_INVALID */)
{
  if (!m_valid)
  {
    CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - File has not been loaded");
    return false;
  }

  DeInitialize();

  /* if the sample rates do not match */
  if (m_sampleRate != resampleRate)
  {
    unsigned int space = (unsigned int)((((float)m_sampleCount / (float)m_sampleRate) * (float)resampleRate) * 2.0f);
    SRC_DATA data;
    data.data_in       = m_samples;
    data.input_frames  = m_frameCount;
    data.data_out      = (float*)_aligned_malloc(sizeof(float) * space, 16);
    data.output_frames = space / m_channels.Count();
    data.src_ratio     = (double)resampleRate / (double)m_sampleRate;
#ifdef TARGET_DARWIN_IOS
    if (src_simple(&data, SRC_SINC_FASTEST, m_channels.Count()) != 0)
#else
    if (src_simple(&data, SRC_SINC_MEDIUM_QUALITY, m_channels.Count()) != 0)
#endif
    {
      CLog::Log(LOGERROR, "CAEWAVLoader::Initialize - Failed to resample audio: %s", m_filename.c_str());
      _aligned_free(data.data_out);
      return false;
    }

    if (m_outputSamples != m_samples)
      _aligned_free(m_outputSamples);

    m_outputSamples     = data.data_out;
    m_outputFrameCount  = data.output_frames_gen;
    m_outputSampleCount = data.output_frames_gen * m_channels.Count();
    m_outputSampleRate  = resampleRate;
  }
  else
  {
    m_outputSamples     = m_samples;
    m_outputFrameCount  = m_frameCount;
    m_outputSampleCount = m_sampleCount;
    m_outputSampleRate  = m_sampleRate;
  }

  /* if the channel layouts do not match */
  if (m_channels != channelLayout)
  {
    CAERemap remap;
    if (!remap.Initialize(m_channels, channelLayout, false, false, stdChLayout))
      return false;

    /* adjust the output format parameters */
    m_outputSampleCount = m_outputFrameCount * channelLayout.Count();
    m_outputChannels    = channelLayout;

    float *remapped = (float*)_aligned_malloc(sizeof(float) * m_outputSampleCount, 16);
    remap.Remap(m_outputSamples, remapped, m_outputFrameCount);

    /* assign the remapped buffer */
    if (m_outputSamples != m_samples)
      _aligned_free(m_outputSamples);
    m_outputSamples = remapped;
  }

  return true;
}

void CAEWAVLoader::UnLoad()
{
  DeInitialize();

  _aligned_free(m_samples);
  m_samples     = NULL;
  m_sampleCount = 0;
  m_frameCount  = 0;
  m_channels.Reset();

  m_outputSamples     = NULL;
  m_outputSampleCount = 0;
  m_outputFrameCount  = 0;
  m_outputChannels.Reset();
}

void CAEWAVLoader::DeInitialize()
{
  /* only free m_outputSamples if it is a seperate buffer */
  if (m_outputSamples != m_samples)
    _aligned_free(m_outputSamples);

  m_outputSamples      = m_samples;
  m_outputSampleCount  = m_sampleCount;
  m_outputFrameCount   = m_frameCount;
  m_outputChannels     = m_channels;
}

CAEChannelInfo CAEWAVLoader::GetChannelLayout()
{
  return m_outputChannels;
}

unsigned int CAEWAVLoader::GetSampleRate()
{
  return m_outputSampleRate;
}

unsigned int CAEWAVLoader::GetSampleCount()
{
  return m_outputSampleCount;
}

unsigned int CAEWAVLoader::GetFrameCount()
{
  return m_outputFrameCount;
}

float* CAEWAVLoader::GetSamples()
{
  if (!m_valid || m_outputSamples == NULL)
    return NULL;

  return m_outputSamples;
}

bool CAEWAVLoader::IsCompatible(const unsigned int sampleRate, const CAEChannelInfo &channelInfo)
{
  return (
    m_outputSampleRate == sampleRate &&
    m_outputChannels   == channelInfo
  );
}
