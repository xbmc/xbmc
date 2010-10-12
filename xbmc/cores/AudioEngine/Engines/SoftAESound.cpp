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

#if (defined USE_EXTERNAL_FFMPEG)
  #include <libavutil/avutil.h>
#else
  #include "cores/dvdplayer/Codecs/ffmpeg/libavutil/avutil.h"
#endif

#include "AESound.h"

#include <samplerate.h>
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"
#include "FileSystem/FileFactory.h"
#include "FileSystem/IFile.h"

#include "AEFactory.h"
#include "AEAudioFormat.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AEUtil.h"

#include "SoftAE.h"
#include "SoftAESound.h"

/* typecast the global AE to CSoftAE */
#define AE (*((CSoftAE*)&AE))

typedef struct
{
  char     chunk_id[4];
  uint32_t chunksize;
} WAVE_CHUNK;

CSoftAESound::CSoftAESound(const CStdString &filename) :
  IAESound(filename),
  m_refcount    (1    ),
  m_valid       (false),
  m_channelCount(0    ),
  m_samples     (NULL ),
  m_frameCount  (0    ),
  m_sampleCount (0    ),
  m_volume      (1.0f )
{
  m_filename = filename;
}

CSoftAESound::~CSoftAESound()
{
  DeInitialize();
}

void CSoftAESound::DeInitialize()
{
  CSingleLock lock(m_critSection);
  _aligned_free(m_samples);
  m_samples      = NULL;
  m_frameCount   = 0;
  m_channelCount = 0;

  if (m_valid)
  {
    m_valid = false;
    CLog::Log(LOGINFO, "CSoftAESound::Deinitialize - Sound Unloaded: %s", m_filename.c_str());
  }
}

bool CSoftAESound::Initialize()
{
  DeInitialize();
  CSingleLock lock(m_critSection);

  XFILE::IFile *file = XFILE::CFileFactory::CreateLoader(m_filename);
  if (!file)
  {
    CLog::Log(LOGERROR, "CSoftAESound::Initialize - Failed to create loader: %s", m_filename.c_str());
    return false;
  }

  struct __stat64 st;
  if (!file->Open(m_filename) || file->Stat(&st) < 0)
  {
    CLog::Log(LOGERROR, "CSoftAESound::Initialize - Failed to stat file: %s", m_filename.c_str());
    delete file;
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
  while(file->Read(&chunk, sizeof(chunk)) == sizeof(chunk)) {
    chunk.chunksize = Endian_SwapLE32(chunk.chunksize);

    /* if its the RIFF header */
    if (!isRIFF && memcmp(chunk.chunk_id, "RIFF", 4) == 0)
    {
      isRIFF = true;
      /* sanity check on the chunksize */
      if (chunk.chunksize > st.st_size - 8)
      {
        CLog::Log(LOGERROR, "CSoftAESound::Initialize - Corrupt WAV header: %s", m_filename.c_str());
        file->Close();
        delete file;
        return false;
      }

      /* we only support WAVE files */
      char format[4];
      if (file->Read(&format, 4) != 4) break;
      isWAVE = memcmp(format, "WAVE", 4) == 0;
      if (!isWAVE) break;
    }
    /* if its the fmt section */
    else if (!isFMT && memcmp(chunk.chunk_id, "fmt ", 4) == 0)
    {
      isFMT = true;
      if (chunk.chunksize != 16) break;
      uint16_t format;
      if (file->Read(&format, sizeof(format)) != sizeof(format)) break;
      format = Endian_SwapLE16(format);
      if (format != WAVE_FORMAT_PCM) break;

      uint16_t channelCount;
      if (file->Read(&channelCount , 2) != 2) break;
      if (file->Read(&sampleRate   , 4) != 4) break;
      if (file->Read(&byteRate     , 4) != 4) break;
      if (file->Read(&blockAlign   , 2) != 2) break;
      if (file->Read(&bitsPerSample, 2) != 2) break;

      m_channelCount = Endian_SwapLE16(channelCount );
      sampleRate     = Endian_SwapLE32(sampleRate   );
      byteRate       = Endian_SwapLE32(byteRate     );
      blockAlign     = Endian_SwapLE16(blockAlign   );
      bitsPerSample  = Endian_SwapLE16(bitsPerSample);

      if (m_channelCount > 2) break;
      isPCM = true;
    }
    /* if we have the PCM info and its the DATA section */
    else if (isPCM && !isDATA && memcmp(chunk.chunk_id, "data", 4) == 0)
    {
       unsigned int bytesPerSample = bitsPerSample >> 3;
       m_sampleCount = chunk.chunksize / bytesPerSample;
       m_frameCount  = m_sampleCount / m_channelCount;
       isDATA        = m_frameCount > 0;

       /* get the conversion function */
       CAEConvert::AEConvertToFn convertFn;
       switch(bitsPerSample)
       {
         case 8 : convertFn = CAEConvert::ToFloat(AE_FMT_U8   ); break;
         case 16: convertFn = CAEConvert::ToFloat(AE_FMT_S16LE); break;
         case 32: convertFn = CAEConvert::ToFloat(AE_FMT_S32LE); break;
         default:
           CLog::Log(LOGERROR, "CSoftAESound::Initialize - Unsupported data format in wav: %s", m_filename.c_str());
           file->Close();
           delete file;
           return false;
       }

       /* read in each sample */
       unsigned int s;
       m_samples = (float*)_aligned_malloc(sizeof(float) * m_sampleCount, 16);
       uint8_t *raw = (uint8_t *)_aligned_malloc(bytesPerSample, 16);
       for(s = 0; s < m_sampleCount; ++s)
       {
         if (file->Read(raw, bytesPerSample) != bytesPerSample)
         {
           CLog::Log(LOGERROR, "CSoftAESound::Initialize - WAV data shorter then expected: %s", m_filename.c_str());
           _aligned_free(m_samples);
           _aligned_free(raw);
           m_samples = NULL;
           file->Close();
           delete file;
           return false;
         }
         
         /* convert the sample to float */
         convertFn(raw, 1, &m_samples[s]);
       }
       _aligned_free(raw);

       static AEChannel layouts[][3] = {
         {AE_CH_FC, AE_CH_NULL},
         {AE_CH_FL, AE_CH_FR, AE_CH_NULL}
       };

       /* setup the remapper to remap straight to the sink format as we inject sounds VERY late into the output */
       CAERemap remap;
       if (!remap.Initialize(layouts[m_channelCount - 1], AE.GetChannelLayout(), false))
       {
         CLog::Log(LOGERROR, "CSoftAESound::Initialize - Failed to initialize the remapper: %s", m_filename.c_str());
         _aligned_free(m_samples);
         m_samples = NULL;

         file->Close();
         delete file;
         return false;
       }

       /* remap the frames */
       float *remapped = (float*)_aligned_malloc(sizeof(float) * m_frameCount * AE.GetChannelCount(), 16);
       remap.Remap(m_samples, remapped, m_frameCount);
       _aligned_free(m_samples);
       m_samples = remapped;
       m_channelCount = AE.GetChannelCount();
    }
    else
    {
      /* skip any unknown sections */
      file->Seek(chunk.chunksize, SEEK_CUR);
    }
  }

  if (!isRIFF || !isWAVE || !isFMT || !isPCM || !isDATA || m_sampleCount == 0)
  {
    CLog::Log(LOGERROR, "CSoftAESound::Initialize - Invalid, or un-supported WAV file: %s", m_filename.c_str());
    file->Close();
    delete file;
    return false;
  }

  /* close the file as we have the samples now */
  file->Close();
  delete file;

  /* if we got here, the file was valid and we have the data but it may need re-sampling still */
  if (sampleRate != AE.GetSampleRate())
  {
    unsigned int space = (unsigned int)((((float)m_sampleCount / (float)sampleRate) * (float)AE.GetSampleRate()) * 2.0f);
    SRC_DATA data;
    data.data_in       = m_samples;
    data.input_frames  = m_frameCount;
    data.data_out      = (float*)_aligned_malloc(sizeof(float) * space, 16);
    data.output_frames = space / m_channelCount;
    data.src_ratio     = (double)AE.GetSampleRate() / (double)sampleRate;
    if (src_simple(&data, SRC_SINC_MEDIUM_QUALITY, m_channelCount) != 0)
    {
      CLog::Log(LOGERROR, "CSoftAESound::Initialize - Failed to resample audio: %s", m_filename.c_str());
      _aligned_free(data.data_out);
      _aligned_free(m_samples);
      m_samples = NULL;
      return false;
    }

    /* reassign m_samples */
    _aligned_free(m_samples);
    m_samples     = data.data_out;
    m_frameCount  = data.output_frames_gen;
    m_sampleCount = data.output_frames_gen * m_channelCount;
  }

  CLog::Log(LOGINFO, "CSoftAESound::Initialize - Sound Loaded: %s", m_filename.c_str());
  m_valid = true;
  return true;
}

float* CSoftAESound::GetSamples()
{
  if (!m_valid || m_samples == NULL) return NULL;
  return m_samples;
}

bool CSoftAESound::IsPlaying()
{
  if (!m_valid) return false;
  return AE.IsPlaying(this);
}

void CSoftAESound::Play()
{
  if (!m_valid) return;
  AE.PlaySound(this);
}

void CSoftAESound::Stop()
{
  if (!m_valid) return;
  AE.StopSound(this);
}

