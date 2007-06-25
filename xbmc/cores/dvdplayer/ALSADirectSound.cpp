/*
* XBoxMediaPlayer
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include "ALSADirectSound.h"
#include "AudioContext.h"

#define CHECK_ALSA(l,s,e) if ((e)<0) CLog::Log(l,"%s - %s, alsa error: %s",__FUNCTION__,s,snd_strerror(e));
#define CHECK_ALSA_RETURN(l,s,e) CHECK_ALSA((l),(s),(e)); if ((e)<0) return ;


void CALSADirectSound::DoWork()
{

}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CALSADirectSound::CALSADirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, char* strAudioCodec, bool bIsMusic)
{

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_bPause = false;
  m_bIsAllocated = false;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;

  m_dwPacketSize = iChannels*(uiBitsPerSample/8)*512;
  m_BufferSize = m_dwPacketSize * 6; // buffer big enough - but not too big...
  m_dwNumPackets = 16;

  snd_pcm_hw_params_t *hw_params=NULL;

  /* Open the device */
  char* device = getenv("XBMC_AUDIODEV");
  if (device == NULL)
  device = "default";

  int nErr = snd_pcm_open(&m_pPlayHandle, device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  CHECK_ALSA_RETURN(LOGERROR,"pcm_open",nErr);

  /* Allocate Hardware Parameters structures and fills it with config space for PCM */
  snd_pcm_hw_params_alloca(&hw_params);

  nErr = snd_pcm_hw_params_any(m_pPlayHandle, hw_params);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_any",nErr);

  nErr = snd_pcm_hw_params_set_access(m_pPlayHandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_access",nErr);

  // always use 16 bit samples
  nErr = snd_pcm_hw_params_set_format(m_pPlayHandle, hw_params, SND_PCM_FORMAT_S16_LE);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_format",nErr);

  nErr = snd_pcm_hw_params_set_rate_near(m_pPlayHandle, hw_params, &m_uiSamplesPerSec, NULL);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_rate",nErr);

  nErr = snd_pcm_hw_params_set_channels(m_pPlayHandle, hw_params, iChannels);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_channels",nErr);

  m_maxFrames = m_dwPacketSize ;
  nErr = snd_pcm_hw_params_set_period_size_near(m_pPlayHandle, hw_params, &m_maxFrames, 0);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_period_size",nErr);

  m_BufferSize = m_dwPacketSize * 20; // buffer big enough - but not too big...
  nErr = snd_pcm_hw_params_set_buffer_size_near(m_pPlayHandle, hw_params, &m_BufferSize);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_buffer_size",nErr);

  unsigned int periodDuration = 0;
  nErr = snd_pcm_hw_params_get_period_time(hw_params,&periodDuration, 0);
  CHECK_ALSA(LOGERROR,"hw_params_get_period_time",nErr);

  /* Assign them to the playback handle and free the parameters structure */
  nErr = snd_pcm_hw_params(m_pPlayHandle, hw_params);
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_hw_params",nErr);

  nErr = snd_pcm_prepare (m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"snd_pcm_prepare",nErr);

  m_bIsAllocated = true;
}

//***********************************************************************************************
CALSADirectSound::~CALSADirectSound()
{
  OutputDebugString("CALSADirectSound() dtor\n");
  Deinitialize();
}


//***********************************************************************************************
HRESULT CALSADirectSound::Deinitialize()
{
  OutputDebugString("CALSADirectSound::Deinitialize\n");

  m_bIsAllocated = false;
  if (m_pPlayHandle)
  {
    snd_pcm_drain(m_pPlayHandle);
    snd_pcm_close(m_pPlayHandle);
  }

  m_pPlayHandle=NULL;
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return S_OK;
}


//***********************************************************************************************
HRESULT CALSADirectSound::Pause()
{
  if (m_bPause) return S_OK;
  m_bPause = true;
  snd_pcm_pause(m_pPlayHandle,1); // this is not supported on all devices. 
  return S_OK;
}

//***********************************************************************************************
HRESULT CALSADirectSound::Resume()
{
  if (!m_bPause) return S_OK;
  m_bPause = false;
  snd_pcm_pause(m_pPlayHandle,0);
  return S_OK;
}

//***********************************************************************************************
HRESULT CALSADirectSound::Stop()
{
  if (m_bPause) return S_OK;
  snd_pcm_drain(m_pPlayHandle);
  return S_OK;
}

//***********************************************************************************************
LONG CALSADirectSound::GetMinimumVolume() const
{
  return -60;
}

//***********************************************************************************************
LONG CALSADirectSound::GetMaximumVolume() const
{
  return 60;
}

//***********************************************************************************************
LONG CALSADirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CALSADirectSound::Mute(bool bMute)
{
  if (!m_bIsAllocated) return;
/*
  if (bMute)
    m_pBuffer->SetVolume(GetMinimumVolume());
  else
    m_pBuffer->SetVolume(m_nCurrentVolume);
*/

#warning volume control not yet implemented for ALSA
}

//***********************************************************************************************
HRESULT CALSADirectSound::SetCurrentVolume(LONG nVolume)
{
  if (!m_bIsAllocated) return -1;
  m_nCurrentVolume = nVolume;
#warning volume control not yet implemented for ALSA
  return S_OK;
}


//***********************************************************************************************
DWORD CALSADirectSound::GetSpace()
{
  return snd_pcm_avail_update(m_pPlayHandle);
}

//***********************************************************************************************
DWORD CALSADirectSound::AddPackets(unsigned char *data, DWORD len)
{

  if (!m_pPlayHandle || snd_pcm_avail_update(m_pPlayHandle) < ( len / (2*m_uiChannels)) || m_bPause)
       return 0;

  unsigned char *pcmPtr = data;
  while ( pcmPtr < data + len) {
       int nPeriodSize = (m_maxFrames * 2 * m_uiChannels); // write a frame.
       if ( pcmPtr + nPeriodSize >  data + len) {
               nPeriodSize = data + len - pcmPtr;
       }

       int framesToWrite = nPeriodSize / (2 * m_uiChannels);
       int writeResult = snd_pcm_writei(m_pPlayHandle, pcmPtr, framesToWrite);
       if (  writeResult == -EPIPE  ) {
               CLog::Log(LOGDEBUG, "CALSADirectSound::AddPackets - buffer underun (tried to write %d frames)",
                       framesToWrite);
               int err = snd_pcm_prepare(m_pPlayHandle);
               CHECK_ALSA(LOGERROR,"prepare after EPIPE", err);
       }
       else if (writeResult != framesToWrite) {
               CLog::Log(LOGERROR, "CALSADirectSound::AddPackets - failed to write %d frames. "
                       "bad write (err: %d) - %s",
                       framesToWrite, writeResult, snd_strerror(writeResult));
               break;
       }

       pcmPtr += nPeriodSize;
  }

  return len;
}

//***********************************************************************************************
FLOAT CALSADirectSound::GetDelay()
{
  long bytes = m_BufferSize - GetSpace();
  double delay = (double)bytes / ( m_uiChannels * m_uiSamplesPerSec * (m_uiBitsPerSample / 8) );

  if (g_audioContext.IsAC3EncoderActive())
    delay += 0.049;
  else
    delay += 0.008;

  return delay;
}

//***********************************************************************************************
DWORD CALSADirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}
//***********************************************************************************************
int CALSADirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CALSADirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void CALSADirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CALSADirectSound::WaitCompletion()
{

}

void CALSADirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}
