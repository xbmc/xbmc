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
#include <stdio.h>
#include "AsyncDirectSound.h"
#include "../../settings.h"
#include "../../utils/log.h"

#define VOLUME_MIN    DSBVOLUME_MIN
#define VOLUME_MAX    DSBVOLUME_MAX


#define CALC_DELAY_START   0
#define CALC_DELAY_STARTED 1
#define CALC_DELAY_DONE    2

static long buffered_bytes=0;
//***********************************************************************************************
void CALLBACK CASyncDirectSound::StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
  CASyncDirectSound* This = (CASyncDirectSound*) pStreamContext;
  This->StreamCallback(pPacketContext, dwStatus);
}


//***********************************************************************************************
void CASyncDirectSound::StreamCallback(LPVOID pPacketContext, DWORD dwStatus)
{
  buffered_bytes -=m_dwPacketSize;
  if (buffered_bytes<0) buffered_bytes=0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CASyncDirectSound::CASyncDirectSound(IAudioCallback* pCallback,int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample)
{
  buffered_bytes=0;
  m_pCallback=pCallback;
  
  m_bResampleAudio = false;
  if (bResample && uiSamplesPerSec != 48000)
	  m_bResampleAudio = true;

  bool  bAudioOnAllSpeakers(false);
  if (g_stSettings.m_bUseDigitalOutput)
  {
    if (g_stSettings.m_bAudioOnAllSpeakers  ) 
    {
      bAudioOnAllSpeakers=true;
    }
  }
  if (bAudioOnAllSpeakers)
  {
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
  }
  else
  {
    if (iChannels == 1)
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
    else if (iChannels == 2)
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
    else
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
  }

  LARGE_INTEGER qwTicksPerSec;
  QueryPerformanceFrequency( &qwTicksPerSec );   // ticks/sec
  m_dwTicksPerSec=qwTicksPerSec.QuadPart;

  m_bPause           = false;
  m_iAudioSkip       = 1;
  m_bIsPlaying       = false;
  m_bIsAllocated     = false;
  m_pDSound          = NULL;
  m_adwStatus        = NULL;
  m_pStream          = NULL;
  m_adwStatus        = NULL;
  if (m_bResampleAudio)
  {
	  m_uiSamplesPerSec = 48000;
	  m_uiBitsPerSample = 16;
  }
  else
  {
	  m_uiSamplesPerSec = uiSamplesPerSec;
	  m_uiBitsPerSample = uiBitsPerSample;
  }
  ZeroMemory(m_pbSampleData,sizeof(m_pbSampleData));
  m_bFirstPackets    = true;
  m_iCalcDelay       = CALC_DELAY_START;
  m_fCurDelay        = (FLOAT)0.001;
  m_delay            = 1;
  ZeroMemory(&m_wfx,sizeof(m_wfx)); 
  m_wfx.cbSize=sizeof(m_wfx);
  XAudioCreatePcmFormat(  iChannels,
                          m_uiSamplesPerSec,
                          m_uiBitsPerSample,
                          &m_wfx
                          );

  // Create enough samples to hold approx 2 sec worth of audio.
  // date    m_dwPacketSize           m_dwNumPackets   description
  // 1  feb   1024                    16               THE avsync fix 
  // 6  feb   1152                    8*iChannels      fix: digital / ac3 passtru didnt work
  // 10 feb   1152(*iChannels/2)      8*iChannels      fix: choppy playback for WMV 
  // 10 feb   1152*iChannels          8*iChannels      fix: mono audio didnt work
  // 10 feb   1152*iChannels*2        8*iChannels      fix: wmv plays choppy

  //m_dwPacketSize     = 1152 * (uiBitsPerSample/8) * iChannels;
  //m_dwNumPackets = ( (m_wfx.nSamplesPerSec / ( m_dwPa2cketSize / ((uiBitsPerSample/8) * m_wfx.nChannels) )) / 2);
  m_dwNumPackets=8*iChannels;
  m_adwStatus    = new DWORD[ m_dwNumPackets ];

   
    // Create DirectSound
  HRESULT hr;
  hr= DirectSoundCreate( NULL, &m_pDSound, NULL ) ;
  if( DS_OK != hr  )
  {
    CLog::Log("DirectSoundCreate() Failed");
      return;
  }
  m_nCurrentVolume = GetMaximumVolume();

  for( DWORD j = 0; j < m_dwNumPackets; j++ )
    m_adwStatus[ j ] = XMEDIAPACKET_STATUS_SUCCESS;

  //DSMIXBINVOLUMEPAIR dsmbvp6[6] = {DSMIXBINVOLUMEPAIRS_DEFAULT_6CHANNEL};
  DSMIXBINVOLUMEPAIR dsmbvp4[4] = {DSMIXBINVOLUMEPAIRS_DEFAULT_4CHANNEL};
  DSMIXBINVOLUMEPAIR dsmbvp2[4] = {DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO};
  DSMIXBINVOLUMEPAIR dsmbvp1[4] = {DSMIXBINVOLUMEPAIRS_DEFAULT_MONO};
  DSMIXBINS dsmb;

  DSMIXBINVOLUMEPAIR dsmbvp6[6] = 
  {
    {DSMIXBIN_FRONT_LEFT ,0},
    {DSMIXBIN_FRONT_RIGHT,0},
    {DSMIXBIN_BACK_LEFT,0},
    {DSMIXBIN_BACK_RIGHT,0},
    {DSMIXBIN_FRONT_CENTER,0},
    {DSMIXBIN_LOW_FREQUENCY,0}

  };

  
  ZeroMemory(&m_wfxex,sizeof(m_wfxex));

	m_wfxex.Format = m_wfx;
  m_wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  m_wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX) ;
  m_wfxex.Samples.wReserved=0;
  m_wfxex.dwChannelMask    = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT;
  dsmb.dwMixBinCount       = 6;
  dsmb.lpMixBinVolumePairs = dsmbvp6;

  if (!bAudioOnAllSpeakers)
  {
    switch (iChannels)
    {
      case 1:
        m_wfxex.dwChannelMask = SPEAKER_FRONT_LEFT;
        dsmb.dwMixBinCount = 1;
        dsmb.lpMixBinVolumePairs = dsmbvp1;
      break;

      case 2:
        m_wfxex.dwChannelMask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT;
        dsmb.dwMixBinCount = 2;
        dsmb.lpMixBinVolumePairs = dsmbvp2;
      break;

      case 3:
      case 4:
        m_wfxex.dwChannelMask =SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT;
        dsmb.dwMixBinCount = 4;
        dsmb.lpMixBinVolumePairs = dsmbvp4;
      break;

      case 5:
        m_wfxex.dwChannelMask =SPEAKER_FRONT_CENTER|SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT;
        dsmb.dwMixBinCount       = 5;
        dsmb.lpMixBinVolumePairs = dsmbvp6;
      break;

      case 6: 
        m_wfxex.dwChannelMask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT;
        dsmb.dwMixBinCount = 6;
        dsmb.lpMixBinVolumePairs = dsmbvp6;
      break;
    }
  }
  m_wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

  //xbox_Ac3encoder_active = false;
  //if ( (dsmb.dwMixBinCount>2) && ((XGetAudioFlags()&(DSSPEAKER_ENABLE_AC3|DSSPEAKER_ENABLE_DTS)) != 0 ) )
    //xbox_Ac3encoder_active = true;

  DSSTREAMDESC dssd; 
  memset(&dssd,0,sizeof(dssd));

  dssd.dwFlags              = DSSTREAMCAPS_ACCURATENOTIFY; // xbmp=0
  dssd.dwMaxAttachedPackets = m_dwNumPackets;
  dssd.lpwfxFormat          = (WAVEFORMATEX*)&m_wfxex;
  dssd.lpfnCallback         = StaticStreamCallback;
  dssd.lpvContext           = this;
  dssd.lpMixBins            = &dsmb;

  if(DirectSoundCreateStream( &dssd, &m_pStream )!=DS_OK)
  {
      CLog::Log("*WARNING* Unable to create sound stream!");
  }

  XMEDIAINFO info;
  m_pStream->GetInfo(&info);
  int fSize = 1024 / info.dwInputSize;
  fSize *= info.dwInputSize;
  m_dwPacketSize=(int)fSize;
  for (DWORD dwX=0; dwX < m_dwNumPackets ; dwX++)
    m_pbSampleData[dwX] = (BYTE*)XPhysicalAlloc( m_dwPacketSize, MAXULONG_PTR,0,PAGE_READWRITE|PAGE_NOCACHE);

  m_nCurrentVolume = GetMaximumVolume();
  m_pStream->SetVolume( m_nCurrentVolume );

  // Set the headroom of the stream to 0 (to allow the maximum volume)
  m_pStream->SetHeadroom(0);
  // Set the default mixbins headroom to 0 (to allow the maximum volume)
  for (DWORD i=0; i<dsmb.dwMixBinCount;i++)
	m_pDSound->SetMixBinHeadroom(i, 0);

  m_pStream->Flush();
  
  m_bIsAllocated   = true;
  if (m_pCallback)
  {
	m_pCallback->OnInitialize(iChannels, m_uiSamplesPerSec, m_uiBitsPerSample);
  }
  if (m_bResampleAudio)
  {
	  m_Resampler.InitConverter(uiSamplesPerSec, uiBitsPerSample, iChannels, 48000, 16, m_dwPacketSize);
  }
}

//***********************************************************************************************
CASyncDirectSound::~CASyncDirectSound()
{
  OutputDebugString("CASyncDirectSound() dtor\n");
  Deinitialize();
}


//***********************************************************************************************
HRESULT CASyncDirectSound::Deinitialize()
{
  OutputDebugString("CASyncDirectSound::Deinitialize\n");
  m_bIsAllocated = false;
  if (m_pStream)
  {
    m_pStream->Flush();
    m_pStream->Pause( DSSTREAMPAUSE_PAUSE );

    m_pStream->Release();
    m_pStream=NULL;
  }
  if ( m_pDSound )
  {
    m_pDSound->Release() ;
  }
  m_pDSound =NULL;

  for (DWORD dwX=0; dwX < m_dwNumPackets ; dwX++)
  {
    if (m_pbSampleData[dwX]) XPhysicalFree(m_pbSampleData[dwX]) ;
    m_pbSampleData[dwX]=NULL;
  }
  if ( m_adwStatus )
    delete [] m_adwStatus;
  m_adwStatus=NULL;

  return S_OK;
}


//***********************************************************************************************
HRESULT CASyncDirectSound::Pause()
{
  if (m_bPause) return S_OK;
  m_bPause=true;
  m_pStream->Pause( DSSTREAMPAUSE_PAUSE );
  return S_OK;
}

//***********************************************************************************************
HRESULT CASyncDirectSound::Resume()
{ 
  if (!m_bPause) return S_OK;
  m_bPause=false;
  m_pStream->Pause( DSSTREAMPAUSE_RESUME );

  return S_OK;
}

//***********************************************************************************************
HRESULT CASyncDirectSound::Stop()
{
  buffered_bytes=0;
  if (m_bPause) return S_OK;
  m_bPause=true;

  // Check the status of each packet
  //mp_msg(0,0,"CASyncDirectSound::Stop");
  if (m_pStream) 
  {   
    m_pStream->Flush();
    m_pStream->Pause( DSSTREAMPAUSE_PAUSE );
  }
  for( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    m_adwStatus[ i ] = XMEDIAPACKET_STATUS_SUCCESS;
  }

  m_bFirstPackets=true;
  return S_OK;
}

//***********************************************************************************************
LONG CASyncDirectSound::GetMinimumVolume() const
{
  return VOLUME_MIN;
}

//***********************************************************************************************
LONG CASyncDirectSound::GetMaximumVolume() const
{
  return VOLUME_MAX;
}

//***********************************************************************************************
LONG CASyncDirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
HRESULT CASyncDirectSound::SetCurrentVolume(LONG nVolume)
{
  if (!m_bIsAllocated) return -1;
  m_nCurrentVolume = nVolume;
  return m_pStream->SetVolume( m_nCurrentVolume );
}

//***********************************************************************************************
bool CASyncDirectSound::FindFreePacket( DWORD &dwIndex )
{
  if (!m_bIsAllocated) return false;

  // Check the status of each packet
  for( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if( m_adwStatus[ i ] != XMEDIAPACKET_STATUS_PENDING )
    {
        dwIndex  = i;
        return true;
    }
  }
  return false;
}

//***********************************************************************************************
bool CASyncDirectSound::SupportsSurroundSound()  const
{
  return false;
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetSpace()
{
  DWORD iFreePackets(0);
 
  // Check the status of each packet
  for( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if( m_adwStatus[ i ] != XMEDIAPACKET_STATUS_PENDING )
    {
      iFreePackets++;
    }
  }
  DWORD dwSize= iFreePackets*m_dwPacketSize;
  return dwSize;
}

// 48kHz resampled data method
DWORD CASyncDirectSound::AddPacketsResample(unsigned char *pData, DWORD iLeft)
{
	DWORD   dwIndex = 0;
	DWORD   iBytesCopied=0;
	while (true)
	{
		// Get the next free packet to fill with our output audio
		if( FindFreePacket(dwIndex) )
		{
			XMEDIAPACKET xmpAudio = {0};

			// loop around, grabbing data from the input buffer and resampling 
			// until we fill up this packet
			while(true)
			{
				// check if we have resampled data to send
				if (m_Resampler.GetData(m_pbSampleData[dwIndex]))
				{
					// Set up audio packet
					m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;
					xmpAudio.dwMaxSize        = m_dwPacketSize;
					xmpAudio.pvBuffer         = m_pbSampleData[dwIndex];
					xmpAudio.pdwStatus        = &m_adwStatus[ dwIndex ];
					xmpAudio.pdwCompletedSize = NULL;
					xmpAudio.prtTimestamp     = NULL;
					xmpAudio.pContext         = NULL;
					// Pass audio to the callback functions for visualization
					if (m_pCallback)
					{
						m_pCallback->OnAudioData(m_pbSampleData[dwIndex],m_dwPacketSize);
					}
					// Process the audio
					if (DS_OK != m_pStream->Process( &xmpAudio, NULL ))
					{
						return iBytesCopied;
					}
					// Okay - we've done this bit, update our data info
					buffered_bytes+=m_dwPacketSize;
					// break to get another packet and restart the loop
					break;
				}
				else
				{	// put more data into the resampler
					DWORD iSize = m_Resampler.PutData(&pData[iBytesCopied], iLeft);
					if (iSize == -1)
					{	// Failed - we don't have enough data
						return iBytesCopied;
					}
					else
					{	// Success - update the amount that we have processed
						iBytesCopied+=iSize;
						iLeft -=iSize;
					}
					// Now loop back around and output data, or read more in
				}
			}
		}
		else
		{	// no packets free - send data back to sender
			return iBytesCopied;
		}
	}
	return iBytesCopied;
}

//***********************************************************************************************
DWORD CASyncDirectSound::AddPackets(unsigned char *data, DWORD len)
{
	// Check if we are resampling using SSRC
  if (m_bResampleAudio)
	return AddPacketsResample(data, len);

  DWORD   dwIndex = 0;
  DWORD   iBytesCopied=0;

  while (len)
  {
    if( FindFreePacket(dwIndex) )
    {
      XMEDIAPACKET xmpAudio = {0};
      
      DWORD iSize=m_dwPacketSize;
      if (len < m_dwPacketSize) 
      {
        // we don't accept half full packets...
        iSize=len;
        return iBytesCopied;
      }
      m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;

      // Set up audio packet

      xmpAudio.dwMaxSize        = iSize/m_iAudioSkip;
      xmpAudio.pvBuffer         = m_pbSampleData[dwIndex];
      xmpAudio.pdwStatus        = &m_adwStatus[ dwIndex ];
      xmpAudio.pdwCompletedSize = NULL;
      xmpAudio.prtTimestamp     = NULL;
      xmpAudio.pContext         = NULL;
      
      memcpy(xmpAudio.pvBuffer,&data[iBytesCopied],iSize/m_iAudioSkip);
      if (m_pCallback)
      {
        m_pCallback->OnAudioData(&data[iBytesCopied],iSize/m_iAudioSkip);
      }
      if (DS_OK != m_pStream->Process( &xmpAudio, NULL ))
      {
        return iBytesCopied;
      }
      buffered_bytes+=(iSize/m_iAudioSkip);
      iBytesCopied+=iSize;
      len -=iSize;
    }
    else 
    {
      break;
    }
  }
  
#ifdef FADE_IN
  if (m_bFirstPackets)
  {
    if ( m_lFadeVolume < m_nCurrentVolume)
    {
      m_lFadeVolume+=200;
      m_pStream->SetVolume( m_lFadeVolume + VOLUME_MIN);
    }
    else 
    {
      m_pStream->SetVolume( m_nCurrentVolume + VOLUME_MIN);
      m_bFirstPackets=false;
    }
  }
#endif  
  return iBytesCopied;
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetBytesInBuffer()
{
  return buffered_bytes;
  /*
  DWORD dwBytesInBuffer=0;
  for( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if( m_adwStatus[ i ] == XMEDIAPACKET_STATUS_PENDING)
    {
      dwBytesInBuffer+=m_dwPacketSize;
    }
  }
  return dwBytesInBuffer;*/
}

//***********************************************************************************************
FLOAT CASyncDirectSound::GetDelay()
{
  if (g_stSettings.m_bUseDigitalOutput)
    return 0.049f;      //(Ac3 encoder 29ms)+(receiver 20ms)
  else
    return 0.008f;      //PCM output 8ms
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}
//***********************************************************************************************
int CASyncDirectSound::SetPlaySpeed(int iSpeed)
{
  DWORD ret_val=0;

  DWORD OrgFreq=(DWORD)m_uiSamplesPerSec;
  DWORD NewFreq;
  if (!m_pStream)
          return 0;

  if (iSpeed < 0)
    return 0;

  //iSpeed++;

  NewFreq = OrgFreq * (iSpeed);
  
  if (NewFreq > DSSTREAMFREQUENCY_MAX) 
  {
    // DirectSound sampling can't go beyond DSSTREAMFREQUENCY_MAX
    // Use Skip sample method
    if (!(1152%iSpeed))  // We have to copy complete samples, Have sample may crash direct sound
      m_iAudioSkip = iSpeed;
    m_pStream->SetFrequency( OrgFreq );
    
    return 1;
  }

   m_iAudioSkip = 1;

   m_pStream->SetFrequency( NewFreq );

  return 1;
}

void CASyncDirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  if (!m_pCallback)
  {
	  if (m_bResampleAudio)
		pCallback->OnInitialize(m_wfx.nChannels, 48000, 16);
	  else
		pCallback->OnInitialize(m_wfx.nChannels, m_wfx.nSamplesPerSec, m_wfx.wBitsPerSample );
  }
  m_pCallback=pCallback;
}

void CASyncDirectSound::UnRegisterAudioCallback()
{
  m_pCallback=NULL;
}

