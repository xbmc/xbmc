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
#include "Settings.h"
#include "Ac97DirectSound.h"
#include "AudioContext.h"

#define VOLUME_MIN  DSBVOLUME_MIN
#define VOLUME_MAX  DSBVOLUME_MAX

//***********************************************************************************************
void CALLBACK CAc97DirectSound::StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
  CAc97DirectSound* This = (CAc97DirectSound*) pStreamContext;
  This->StreamCallback(pPacketContext, dwStatus);
}


//***********************************************************************************************
void CAc97DirectSound::StreamCallback(LPVOID pPacketContext, DWORD dwStatus)
{}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CAc97DirectSound::CAc97DirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bAC3DTS)
{
  m_pCallback = pCallback;
  m_bAc3DTS = bAC3DTS;

  m_bPause = false;
  m_bMute = false;
  m_bIsAllocated = false;
  m_adwStatus = NULL;
  m_dwNumPackets = 0;
  m_pDigitalOutput = NULL;
  m_dwTotalBytesAdded=0;
  ZeroMemory(m_pbSampleData, sizeof(m_pbSampleData));

  if(uiSamplesPerSec != 48000)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid samplerate %d, Ac97 interface only handles 48000", uiSamplesPerSec);
    return;
  }

  if(iChannels != 2)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid number of channels %d, Ac97 interface only handles 2 channels", iChannels);
    return;
  }

  // we want 1/4th of a second worth of data
#if 0 //mplayer is broken on some packets sizes
  m_dwPacketSize = 512; // samples
  m_dwPacketSize *= m_uiChannels * (m_uiBitsPerSample>>3);
#else
  m_dwPacketSize = 1024;
#endif
  m_dwNumPackets = 48000 * 2 * (16>>3);
  m_dwNumPackets /= m_dwPacketSize * 4;

  // don't go larger than soundcard can handle
  if(m_dwNumPackets > DSAC97_MAX_ATTACHED_PACKETS)
    m_dwNumPackets = DSAC97_MAX_ATTACHED_PACKETS;

  WAVEFORMATEX m_wfx = {};
  XAudioCreatePcmFormat( 2, 48000, 16, &m_wfx ); //passthrough is always 2ch 48000KHz/16bit
  
  m_adwStatus = new DWORD[ m_dwNumPackets ];
  for ( DWORD j = 0; j < m_dwNumPackets; j++ )
    m_adwStatus[ j ] = XMEDIAPACKET_STATUS_SUCCESS;

  g_audioContext.SetActiveDevice(CAudioContext::AC97_DEVICE);
  m_pDigitalOutput = g_audioContext.GetAc97Device();

  // align m_dwPacketSize to dwInputSize
  XMEDIAINFO info;
  m_pDigitalOutput->GetInfo(&info);  
  m_dwPacketSize /= info.dwInputSize;
  m_dwPacketSize *= info.dwInputSize;

  // XphysicalAlloc has page (4k) granularity, so allocate all the buffers in one chunk to avoid wasting 3k per buffer
  m_pbSampleData[0] = (BYTE*)XPhysicalAlloc(m_dwPacketSize * m_dwNumPackets, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
  for (DWORD dwX = 1; dwX < m_dwNumPackets; dwX++)
    m_pbSampleData[dwX] = m_pbSampleData[dwX - 1] + m_dwPacketSize;

  HRESULT hr = m_pDigitalOutput->SetMode(bAC3DTS ? DSAC97_MODE_ENCODED : DSAC97_MODE_PCM);
  m_bIsAllocated = true;
}

//***********************************************************************************************
CAc97DirectSound::~CAc97DirectSound()
{
  Deinitialize();
}


//***********************************************************************************************
HRESULT CAc97DirectSound::Deinitialize()
{
  m_bIsAllocated = false;

  if (m_pbSampleData[0])
    XPhysicalFree(m_pbSampleData[0]);
  memset(m_pbSampleData, 0, m_dwNumPackets * sizeof(m_pbSampleData[0]));

  if ( m_adwStatus )
    delete [] m_adwStatus;
  m_adwStatus = NULL;

  m_pDigitalOutput=NULL;  
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return S_OK;
}


//***********************************************************************************************
HRESULT CAc97DirectSound::Pause()
{
  if (m_bPause) return S_OK;
  m_bPause = true;

  //In lack of a better way, stop the stream
  Stop();

  return S_OK;
}

//***********************************************************************************************
HRESULT CAc97DirectSound::Resume()
{
  if (!m_bPause) return S_OK;
  m_bPause = false;

  return S_OK;
}

//***********************************************************************************************
HRESULT CAc97DirectSound::Stop()
{
  if (m_bPause) return S_OK;

  if (m_pDigitalOutput)
  {
    m_pDigitalOutput->Flush();
  }
  for ( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    m_adwStatus[ i ] = XMEDIAPACKET_STATUS_SUCCESS;
  }

  //Set our counter to current position
  DWORD m_dwPos=0;
  m_pDigitalOutput->GetCurrentPosition(&m_dwPos);
  m_dwTotalBytesAdded = m_dwPos;


  return S_OK;
}

//***********************************************************************************************
LONG CAc97DirectSound::GetMinimumVolume() const
{
  return VOLUME_MIN;
}

//***********************************************************************************************
LONG CAc97DirectSound::GetMaximumVolume() const
{
  return VOLUME_MAX;
}

//***********************************************************************************************
LONG CAc97DirectSound::GetCurrentVolume() const
{
  return VOLUME_MAX;
}

//***********************************************************************************************
void CAc97DirectSound::Mute(bool bMute)
{
  m_bMute = bMute;
}

//***********************************************************************************************
HRESULT CAc97DirectSound::SetCurrentVolume(LONG nVolume)
{
  if (!m_bIsAllocated) return -1;
  if (nVolume == VOLUME_MINIMUM)
    m_bMute = true;
  else
    m_bMute = false;
  return S_OK;
}

//***********************************************************************************************
bool CAc97DirectSound::FindFreePacket( DWORD &dwIndex )
{
  if (!m_bIsAllocated) return false;

  // Check the status of each packet
  for ( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if ( m_adwStatus[ i ] != XMEDIAPACKET_STATUS_PENDING )
    {
      dwIndex = i;
      return true;
    }
  }
  return false;
}

//***********************************************************************************************
DWORD CAc97DirectSound::GetSpace()
{
  DWORD iFreePackets(0);

  // Check the status of each packet
  for ( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if ( m_adwStatus[ i ] != XMEDIAPACKET_STATUS_PENDING )
    {
      iFreePackets++;
    }
  }
  return iFreePackets * m_dwPacketSize;
}

//***********************************************************************************************
DWORD CAc97DirectSound::AddPackets(unsigned char *data, DWORD len)
{
  // Don't accept packets when paused
  if( m_bPause ) return 0;

  HRESULT hr;
  DWORD dwIndex = 0;
  DWORD iBytesCopied = 0;

  while (len)
  {
    if ( FindFreePacket(dwIndex) )
    {
      XMEDIAPACKET xmpAudio = {0};

      //   DirectSoundDoWork();


      DWORD iSize = m_dwPacketSize;
      if (len < m_dwPacketSize)
      {
        // we don't accept half full packets...
        iSize = len;
        return iBytesCopied;
      }
      m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;

      // Set up audio packet
      xmpAudio.dwMaxSize = iSize;
      xmpAudio.pvBuffer = m_pbSampleData[dwIndex];
      xmpAudio.pdwStatus = &m_adwStatus[ dwIndex ];
      xmpAudio.pdwCompletedSize = NULL;
      xmpAudio.prtTimestamp = NULL;
      xmpAudio.pContext = NULL;

      if (m_bMute)
        memset(xmpAudio.pvBuffer, 0, iSize);
      else
        memcpy(xmpAudio.pvBuffer, &data[iBytesCopied], iSize);

      // no need to do analogue out - analogue should be disabled as we're
      // passing non-PCM streams only using AC97
      hr = m_pDigitalOutput->Process( &xmpAudio, NULL );
      //  hr=m_pAnalogOutput->Process( &xmpAudio, NULL );

      m_dwTotalBytesAdded+=iSize;
      iBytesCopied += iSize;
      len -= iSize;
    }
    else
    {
      break;
    }
  }  
  return iBytesCopied;
}

//***********************************************************************************************
FLOAT CAc97DirectSound::GetDelay()
{
  DWORD m_dwPos=0;
  if( FAILED(m_pDigitalOutput->GetCurrentPosition(&m_dwPos)))
    return 0.0f;

  if( m_dwTotalBytesAdded < m_dwPos )
  {
    CLog::Log(LOGWARNING, " - Stream position larger than what we've added");
    m_dwTotalBytesAdded = m_dwPos;
  }

  // buffer delay
  FLOAT delay = (FLOAT)(m_dwTotalBytesAdded - m_dwPos) / (2 * 48000 * (16>>3));
  
  // static delay
  if (m_bAc3DTS)
    delay += 0.028f;  //(fake PCM output 8ms) + (receiver 20ms)
  else
    delay += 0.008f;  //PCM passthrough

  return delay;
}

//***********************************************************************************************
DWORD CAc97DirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}
//***********************************************************************************************
int CAc97DirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CAc97DirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  if (!m_pCallback)
  {
    pCallback->OnInitialize(2, 48000, 16 );
  }
  m_pCallback = pCallback;
}

void CAc97DirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CAc97DirectSound::WaitCompletion()
{
  if (!m_pDigitalOutput)
    return ;
  m_pDigitalOutput->Discontinuity();
  DWORD status;
  do
  {
    Sleep(10);
    m_pDigitalOutput->GetStatus(&status);
  }
  while (status & DSSTREAMSTATUS_PLAYING);
}
