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
CALSADirectSound::CALSADirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  CLog::Log(LOGDEBUG,"CALSADirectSound::CALSADirectSound - opening alsa device");
  if (iChannels == 0)
    iChannels = 2;

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_bPause = false;
  m_bIsAllocated = false;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;
  m_bPassthrough = bPassthrough;

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;
  if (!m_bPassthrough)
     m_amp.SetVolume(m_nCurrentVolume);

  m_dwPacketSize = iChannels*(uiBitsPerSample/8)*512;
  m_dwNumPackets = 16;

  snd_pcm_hw_params_t *hw_params=NULL;
  snd_pcm_sw_params_t *sw_params=NULL;

  /* Open the device */
  const char* device;
  if (!m_bPassthrough)
    device = g_guiSettings.GetString("audiooutput.audiodevice").c_str();
  else
    device = g_guiSettings.GetString("audiooutput.passthroughdevice").c_str();

  int nErr = snd_pcm_open(&m_pPlayHandle, device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  CHECK_ALSA_RETURN(LOGERROR,"pcm_open",nErr);

  /* Allocate Hardware Parameters structures and fills it with config space for PCM */
  snd_pcm_hw_params_malloc(&hw_params);

  /* Allocate Software Parameters structures and fills it with config space for PCM */
  snd_pcm_sw_params_malloc(&sw_params);

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

  m_BufferSize = snd_pcm_bytes_to_frames(m_pPlayHandle,m_dwPacketSize * 10); // buffer big enough - but not too big...
  nErr = snd_pcm_hw_params_set_buffer_size_near(m_pPlayHandle, hw_params, &m_BufferSize);
  CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_buffer_size",nErr);

  unsigned int periodDuration = 0;
  nErr = snd_pcm_hw_params_get_period_time(hw_params,&periodDuration, 0);
  CHECK_ALSA(LOGERROR,"hw_params_get_period_time",nErr);

  /* Assign them to the playback handle and free the parameters structure */
  nErr = snd_pcm_hw_params(m_pPlayHandle, hw_params);
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_hw_params",nErr);

  nErr = snd_pcm_sw_params_current(m_pPlayHandle, sw_params);
  CHECK_ALSA_RETURN(LOGERROR,"sw_params_current",nErr);

  nErr = snd_pcm_sw_params_set_start_threshold(m_pPlayHandle, sw_params, m_dwPacketSize);
  CHECK_ALSA_RETURN(LOGERROR,"sw_params_set_start_threshold",nErr);

  nErr = snd_pcm_sw_params(m_pPlayHandle, sw_params);
  CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_sw_params",nErr);

  snd_pcm_hw_params_free (hw_params);
  CHECK_ALSA(LOGERROR,"snd_pcm_hw_params_free",nErr);

  snd_pcm_sw_params_free (sw_params);
  CHECK_ALSA(LOGERROR,"snd_pcm_sw_params_free",nErr);

  nErr = snd_pcm_prepare (m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"snd_pcm_prepare",nErr);

  m_bIsAllocated = true;
}

//***********************************************************************************************
CALSADirectSound::~CALSADirectSound()
{
  CLog::Log(LOGDEBUG,"CALSADirectSound() dtor\n");
  Deinitialize();
}


//***********************************************************************************************
HRESULT CALSADirectSound::Deinitialize()
{
  CLog::Log(LOGDEBUG,"CALSADirectSound::Deinitialize\n");

  m_bIsAllocated = false;
  if (m_pPlayHandle)
  {
    snd_pcm_drop(m_pPlayHandle);
    snd_pcm_close(m_pPlayHandle);
  }

  m_pPlayHandle=NULL;
  	CLog::Log(LOGDEBUG,"CALSADirectSound::Deinitialize - set active\n");
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
	
  return S_OK;
}

void CALSADirectSound::Flush() {
  if (m_pPlayHandle == NULL)
     return;

  int nErr = snd_pcm_drop(m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"flush-drain",nErr); 
  nErr = snd_pcm_prepare(m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"flush-prepare",nErr);  
  nErr = snd_pcm_start(m_pPlayHandle);
  CHECK_ALSA(LOGERROR,"flush-start",nErr); 
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
  // Resume is called not only after Pause but also at certain other points. like after stop when DVDPlayer is flushed.
  m_bPause = false;
  snd_pcm_pause(m_pPlayHandle,0);

  Flush();

  return S_OK;
}

//***********************************************************************************************
HRESULT CALSADirectSound::Stop()
{
  if (m_bPause) 
     return S_OK;
  
  if (m_pPlayHandle)
     snd_pcm_drop(m_pPlayHandle);

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

  if (bMute)
    SetCurrentVolume(GetMinimumVolume());
  else
    SetCurrentVolume(m_nCurrentVolume);

}

//***********************************************************************************************
HRESULT CALSADirectSound::SetCurrentVolume(LONG nVolume)
{
  if (!m_bIsAllocated || m_bPassthrough) return -1;
  m_nCurrentVolume = nVolume;
  m_amp.SetVolume(nVolume);
  return S_OK;
}


//***********************************************************************************************
DWORD CALSADirectSound::GetSpace()
{
  if (!m_bIsAllocated) return 0;

  int nSpace = snd_pcm_avail_update(m_pPlayHandle);
  if (nSpace < 0) {
     CLog::Log(LOGWARNING,"CALSADirectSound::GetSpace - get space failed. err: %d (%s)", nSpace, snd_strerror(nSpace));
     nSpace = 0;
  }
  return nSpace;
}

//***********************************************************************************************
DWORD CALSADirectSound::AddPackets(unsigned char *data, DWORD len)
{
  if (!m_pPlayHandle) {
	CLog::Log(LOGERROR,"CALSADirectSound::AddPackets - sanity failed. no play handle!");
	return len; 
  }

  DWORD nAvailSpace = GetSpace();

  // if there is no room in the buffer - even for one frame, return 
  if ( snd_pcm_frames_to_bytes(m_pPlayHandle,nAvailSpace) < (int) len )
  {
    if (nAvailSpace <= m_maxFrames || (DWORD)snd_pcm_frames_to_bytes(m_pPlayHandle,nAvailSpace) <= m_dwPacketSize)
       return 0;

    len = snd_pcm_frames_to_bytes(m_pPlayHandle,nAvailSpace);
  }

  unsigned char *pcmPtr = data;

  while (pcmPtr < data + (int)len){  
	int nPeriodSize = snd_pcm_frames_to_bytes(m_pPlayHandle,m_maxFrames); // write max frames.
	if ( pcmPtr + nPeriodSize > data + (int)len) {
		nPeriodSize = data + (int)len - pcmPtr;
	}
	
	int framesToWrite = snd_pcm_bytes_to_frames(m_pPlayHandle,nPeriodSize);

	// handle volume de-amp 
	if (!m_bPassthrough)
           m_amp.DeAmplify((short *)pcmPtr, framesToWrite * m_uiChannels);
	
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
	}

    if (writeResult>0)
		pcmPtr += snd_pcm_frames_to_bytes(m_pPlayHandle,writeResult);
	else
		pcmPtr += snd_pcm_frames_to_bytes(m_pPlayHandle,framesToWrite); 
  }

  return len;
}

//***********************************************************************************************
FLOAT CALSADirectSound::GetDelay()
{
  double delay = 0.0;

  double fbps = (double)m_uiSamplesPerSec * 2.0 * (double)m_uiChannels;
  snd_pcm_sframes_t frames = 0;
    
  int nErr = snd_pcm_delay(m_pPlayHandle, &frames);
  CHECK_ALSA(LOGERROR,"snd_pcm_delay",nErr); 
  if (nErr < 0)
     return (double)snd_pcm_frames_to_bytes(m_pPlayHandle,m_BufferSize) / fbps;

  if (frames < 0) {
#if SND_LIB_VERSION >= 0x000901 /* snd_pcm_forward() exists since 0.9.0rc8 */
    snd_pcm_forward(m_pPlayHandle, -frames);
#endif
    frames = 0;
  }

  int nBytes = snd_pcm_frames_to_bytes(m_pPlayHandle,frames);
  delay = (double)nBytes / fbps;

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
