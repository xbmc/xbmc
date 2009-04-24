#ifdef __APPLE__
/*
* XBMC Media Center
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
#include "CPortAudio.h"
#include "stdafx.h"
#include "PortaudioDirectSound.h"
#include "AudioContext.h"
#include "Util.h"

//////////////////////////////////////////////////////////////////////////////
//
// History:
//   12.14.07   ESF  Created.
//
//////////////////////////////////////////////////////////////////////////////
PortAudioDirectSound::PortAudioDirectSound()
{
}

bool PortAudioDirectSound::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  CLog::Log(LOGDEBUG,"PortAudioDirectSound::PortAudioDirectSound - opening device");
  
  if (iChannels == 0)
      iChannels = 2;

  bool bAudioOnAllSpeakers = false;
  
  // This should be in the base class.
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_bPause = false;
  m_bCanPause = false;
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

  /* Open the device */
  CStdString device, deviceuse;
  //if (!m_bPassthrough)
    device = g_guiSettings.GetString("audiooutput.audiodevice");
  //else
  //  device = g_guiSettings.GetString("audiooutput.passthroughdevice");

  CLog::Log(LOGINFO, "Asked to open device: [%s]\n", device.c_str());

  m_pStream = CPortAudio::CreateOutputStream(device,
                                             m_uiChannels, 
                                             m_uiSamplesPerSec, 
                                             m_uiBitsPerSample,
                                             m_bPassthrough,
                                             m_dwPacketSize);
  
  // Start the stream.
  SAFELY(Pa_StartStream(m_pStream));

  m_bCanPause = false;
  m_bIsAllocated = true;
  return m_bIsAllocated;
}

//***********************************************************************************************
PortAudioDirectSound::~PortAudioDirectSound()
{
  CLog::Log(LOGDEBUG,"PortAudioDirectSound() dtor");
  Deinitialize();
}


//***********************************************************************************************
HRESULT PortAudioDirectSound::Deinitialize()
{
  CLog::Log(LOGDEBUG,"PortAudioDirectSound::Deinitialize");
  
  if (m_pStream)
  {
    SAFELY(Pa_StopStream(m_pStream));
    SAFELY(Pa_CloseStream(m_pStream));
  }
  
  m_bIsAllocated = false;
  m_pStream = 0;
  
 	CLog::Log(LOGDEBUG,"PortAudioDirectSound::Deinitialize - set active");
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return S_OK;
}

//***********************************************************************************************
void PortAudioDirectSound::Flush() 
{
  if (m_pStream)
  {
    SAFELY(Pa_AbortStream(m_pStream));
    SAFELY(Pa_StartStream(m_pStream));
  }
}

//***********************************************************************************************
HRESULT PortAudioDirectSound::Pause()
{
  if (m_bPause) 
    return S_OK;
  
  m_bPause = true;
  Flush();

  return S_OK;
}

//***********************************************************************************************
HRESULT PortAudioDirectSound::Resume()
{
  // If we are not pause, stream might not be prepared to start flush will do this for us
  if (!m_bPause)
    Flush();

  m_bPause = false;
  return S_OK;
}

//***********************************************************************************************
HRESULT PortAudioDirectSound::Stop()
{
  if (m_pStream)
    SAFELY(Pa_StopStream(m_pStream));

  m_bPause = false;
  return S_OK;
}

//***********************************************************************************************
LONG PortAudioDirectSound::GetMinimumVolume() const
{
  return -60;
}

//***********************************************************************************************
LONG PortAudioDirectSound::GetMaximumVolume() const
{
  return 60;
}

//***********************************************************************************************
LONG PortAudioDirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void PortAudioDirectSound::Mute(bool bMute)
{
  if (!m_bIsAllocated) return;

  if (bMute)
    SetCurrentVolume(GetMinimumVolume());
  else
    SetCurrentVolume(m_nCurrentVolume);

}

//***********************************************************************************************
HRESULT PortAudioDirectSound::SetCurrentVolume(LONG nVolume)
{
  if (!m_bIsAllocated || m_bPassthrough) return -1;
  m_nCurrentVolume = nVolume;
  m_amp.SetVolume(nVolume);
  return S_OK;
}


//***********************************************************************************************
DWORD PortAudioDirectSound::GetSpace()
{
  if (!m_pStream)
    return 0;
  
  // Figure out how much space is available.
  DWORD numFrames = Pa_GetStreamWriteAvailable(m_pStream);
  return numFrames * (m_uiBitsPerSample/8);
}

//***********************************************************************************************
DWORD PortAudioDirectSound::AddPackets(unsigned char *data, DWORD len)
{
  if (!m_pStream) 
  {
    CLog::Log(LOGERROR,"PortAudioDirectSound::AddPackets - sanity failed. no play handle!");
    return len; 
  }
  
  // Find out how much space we have available.
  DWORD framesPassedIn = len / (m_uiChannels * m_uiBitsPerSample/8);
  DWORD framesToWrite  = Pa_GetStreamWriteAvailable(m_pStream);
  
  // Clip to the amount we got passed in. I was using MIN above, but that
  // was a very bad idea since Pa_GetStreamWriteAvailable would get called
  // twice and could return different answers!
  //
  if (framesToWrite > framesPassedIn)
    framesToWrite = framesPassedIn;
  
  unsigned char* pcmPtr = data;
  
  // Handle volume de-amplification.
  if (!m_bPassthrough)
    m_amp.DeAmplify((short *)pcmPtr, framesToWrite * m_uiChannels);
  
  // Write data to the stream.
  SAFELY(Pa_WriteStream(m_pStream, pcmPtr, framesToWrite));

  return framesToWrite * m_uiChannels * (m_uiBitsPerSample/8);
}

//***********************************************************************************************
FLOAT PortAudioDirectSound::GetDelay()
{
  if (m_pStream == 0)
    return 0.0;
  
  const PaStreamInfo* streamInfo = Pa_GetStreamInfo(m_pStream);
  FLOAT delay = (FLOAT)streamInfo->outputLatency;

  if (g_audioContext.IsAC3EncoderActive())
    delay += 0.049;
  else
    delay += 0.008;

  return delay;
}

//***********************************************************************************************
DWORD PortAudioDirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}
//***********************************************************************************************
int PortAudioDirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void PortAudioDirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void PortAudioDirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void PortAudioDirectSound::WaitCompletion()
{

}

void PortAudioDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}
#endif
