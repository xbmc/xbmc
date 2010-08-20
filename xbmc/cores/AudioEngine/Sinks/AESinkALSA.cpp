/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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

#include "AESinkALSA.h"
#include <stdint.h>

#include "AEUtil.h"
#include "utils/log.h"
#include "StdString.h"

CAESinkALSA::CAESinkALSA() :
  m_pcm(NULL)
{
  /* ensure that ALSA has been initialized */
  if(!snd_config)
    snd_config_update();
}

CAESinkALSA::~CAESinkALSA()
{
}

bool CAESinkALSA::Initialize(AEAudioFormat format)
{
  static enum AEChannel ALSAChannelMap[9] =
    {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

  int i, c;

  format.m_channelCount = 0;
  for(c = 0; c < 8; ++c)
    for(i = 0; format.m_channelLayout[i] != AE_CH_NULL; ++i)
      if (format.m_channelLayout[i] == ALSAChannelMap[c])
      {
        format.m_channelCount = c + 1;
        break;
      }

  if (format.m_channelCount == 0)
    return false;

  CStdString       pcm_device = "default"; //FIXME: load from settings
  snd_config_t     *config;

  format.m_frameSamples = 512;
  format.m_frames       = 16;

  if (pcm_device == "default")
    switch(m_format.m_channelCount)
    {
      case 8: pcm_device = "plug:surround71"; break;
      case 6: pcm_device = "plug:surround51"; break;
      case 5: pcm_device = "plug:surround50"; break;
      case 4: pcm_device = "plug:surround40"; break;
    }

  /* get the sound config */
  snd_config_copy(&config, snd_config);
  int error;

  error = snd_pcm_open_lconf(&m_pcm, pcm_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0, config);
  if (error < 0)
  {
    CLog::Log(LOGERROR, "AESinkALSA::Initialize - snd_pcm_open_lconf(%d)", error);
    snd_config_delete(config);
    return false;
  }

  /* free the sound config */
  snd_config_delete(config);

  if (!InitializeHW(format)) return false;
  if (!InitializeSW(format)) return false;

  /* calculate the frame size */
  format.m_frameSize =
    format.m_frameSamples *
    (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  m_format = format;
  return true;
}

bool CAESinkALSA::InitializeHW(AEAudioFormat &format)
{
  snd_pcm_hw_params_t *hw_params = NULL;

  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(m_pcm, hw_params);
  snd_pcm_hw_params_set_access(m_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

  snd_pcm_format_t fmt;
  switch(format.m_dataFormat) {
    case AE_FMT_S8   : fmt = SND_PCM_FORMAT_S8    ; break;
    case AE_FMT_U8   : fmt = SND_PCM_FORMAT_U8    ; break;
    case AE_FMT_S16LE: fmt = SND_PCM_FORMAT_S16_LE; break;
    case AE_FMT_S16BE: fmt = SND_PCM_FORMAT_S16_BE; break;
    case AE_FMT_FLOAT: fmt = SND_PCM_FORMAT_FLOAT ; break;

    default:
      /* if we dont support the requested format, fallback to float */
      format.m_dataFormat = AE_FMT_FLOAT;
      fmt                 = SND_PCM_FORMAT_FLOAT;
  }

  /* if the chosen format is not supported */
  if (snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::InitializeHW - Your hardware does not support %s", CAEUtil::DataFormatToStr(format.m_dataFormat));
    snd_pcm_hw_params_free(hw_params);
    return false;
  }

  snd_pcm_uframes_t periodSize = 512;
  unsigned int      periods    = 64;
  snd_pcm_uframes_t bufferSize;

  snd_pcm_hw_params_set_rate_near       (m_pcm, hw_params, &m_format.m_sampleRate  , NULL);
  snd_pcm_hw_params_set_channels        (m_pcm, hw_params,  m_format.m_channelCount      );
  snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params, &periodSize             , NULL);
  snd_pcm_hw_params_set_periods_near    (m_pcm, hw_params, &periods                , NULL);
  snd_pcm_hw_params_get_buffer_size     (hw_params, &bufferSize);

  /* set the parameters */
  if (snd_pcm_hw_params(m_pcm, hw_params) < 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::InitializeHW - Failed to Initialize HW Parameters");
    snd_pcm_hw_params_free(hw_params);
    return false;
  }

  snd_pcm_hw_params_free(hw_params);
  return true;
}

bool CAESinkALSA::InitializeSW(AEAudioFormat &format)
{
  return false;
}

void CAESinkALSA::Deinitialize()
{
}

void CAESinkALSA::Run()
{
}

void CAESinkALSA::Stop()
{
}

AEAudioFormat CAESinkALSA::GetAudioFormat()
{
  return m_format;
}

unsigned int CAESinkALSA::GetDelay()
{
  return 0;
}

unsigned int CAESinkALSA::AddPackets(uint8_t *data, unsigned int samples)
{
  return samples;
}

