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
#include "Ac97DirectSound.h"
#include "../../settings.h"
#include "../../util.h"

#define VOLUME_MIN		DSBVOLUME_MIN
#define VOLUME_MAX		DSBVOLUME_MAX


#define CALC_DELAY_START	 0
#define CALC_DELAY_STARTED 1
#define CALC_DELAY_DONE		 2

//***********************************************************************************************
void CALLBACK CAc97DirectSound::StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
	CAc97DirectSound* This = (CAc97DirectSound*) pStreamContext;
	This->StreamCallback(pPacketContext, dwStatus);
}


//***********************************************************************************************
void CAc97DirectSound::StreamCallback(LPVOID pPacketContext, DWORD dwStatus)
{
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CAc97DirectSound::CAc97DirectSound(IAudioCallback* pCallback,int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample)
{
	m_pCallback=pCallback;
	m_dwPacketSize		 = 1152 * (uiBitsPerSample/8) * iChannels;
	m_bPause           = false;
	m_bMute            = false;
	m_bIsAllocated     = false;
	m_pDSound          = NULL;
	m_adwStatus        = NULL;
	ZeroMemory(m_pbSampleData,sizeof(m_pbSampleData));
	
	ZeroMemory(&m_wfx,sizeof(m_wfx)); 
	m_wfx.cbSize=sizeof(m_wfx);
	XAudioCreatePcmFormat(  iChannels,
													uiSamplesPerSec, 
													uiBitsPerSample,
													&m_wfx
													);

  m_dwNumPackets=8*iChannels;

	HRESULT hr;
/*
#if 0	 
	  // Create DirectSound
	hr= DirectSoundCreate( NULL, &m_pDSound, NULL ) ;
  if( DS_OK != hr  )
	{
		OutputDebugString("DirectSoundCreate() failed");
    return;
	}
#endif
*/
	m_nCurrentVolume = GetMaximumVolume();

  m_adwStatus    = new DWORD[ m_dwNumPackets ];
	for( DWORD j = 0; j < m_dwNumPackets; j++ )
		m_adwStatus[ j ] = XMEDIAPACKET_STATUS_SUCCESS;


	
	m_pDigitalOutput=NULL;
	hr=Ac97CreateMediaObject(DSAC97_CHANNEL_DIGITAL, NULL, NULL, &m_pDigitalOutput);
	if ( hr !=DS_OK )
	{
		OutputDebugString("failed to create digital Ac97CreateMediaObject()\n");
	}

  XMEDIAINFO info;
  m_pDigitalOutput->GetInfo(&info);
  int fSize = 1024 / info.dwInputSize;
  fSize *= info.dwInputSize;
  m_dwPacketSize=(int)fSize;

	// XphysicalAlloc has page (4k) granularity, so allocate all the buffers in one chunk to avoid wasting 3k per buffer
	m_pbSampleData[0] = (BYTE*)XPhysicalAlloc(m_dwPacketSize * m_dwNumPackets, MAXULONG_PTR,0,PAGE_READWRITE|PAGE_WRITECOMBINE);
	for (DWORD dwX=1; dwX < m_dwNumPackets; dwX++)
		m_pbSampleData[dwX] = m_pbSampleData[dwX-1] + m_dwPacketSize;
	
	bool bAC3DTS=true;
	hr=m_pDigitalOutput->SetMode(bAC3DTS ? DSAC97_MODE_ENCODED : DSAC97_MODE_PCM);
	m_bIsAllocated   = true;
}

//***********************************************************************************************
CAc97DirectSound::~CAc97DirectSound()
{
	Deinitialize();
}


//***********************************************************************************************
HRESULT CAc97DirectSound::Deinitialize()
{
	OutputDebugString("CAc97DirectSound::Deinitialize\n");
	m_bIsAllocated = false;
	if (m_pDigitalOutput)
	{
		m_pDigitalOutput->Release();
		m_pDigitalOutput=NULL;
	}
	if ( m_pDSound )
	{
		m_pDSound->Release() ;
		m_pDSound =NULL;
	}

	if (m_pbSampleData[0])
		XPhysicalFree(m_pbSampleData[0]);
	memset(m_pbSampleData, 0, m_dwNumPackets * sizeof(m_pbSampleData[0]));

	if ( m_adwStatus )
		delete [] m_adwStatus;
	m_adwStatus=NULL;

	return S_OK;
}


//***********************************************************************************************
HRESULT CAc97DirectSound::Pause()
{
	if (m_bPause) return S_OK;
	m_bPause=true;
	return S_OK;
}

//***********************************************************************************************
HRESULT CAc97DirectSound::Resume()
{	
	if (!m_bPause) return S_OK;
	m_bPause=false;

	return S_OK;
}

//***********************************************************************************************
HRESULT CAc97DirectSound::Stop()
{
	if (m_bPause) return S_OK;
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
bool CAc97DirectSound::SupportsSurroundSound()  const
{
	return false;
}

//***********************************************************************************************
DWORD CAc97DirectSound::GetSpace()
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

//***********************************************************************************************
DWORD CAc97DirectSound::AddPackets(unsigned char *data, DWORD len)
{

	HRESULT hr;
	DWORD		dwIndex = 0;
	DWORD   iBytesCopied=0;

	while (len)
	{
		if( FindFreePacket(dwIndex) )
		{
			XMEDIAPACKET xmpAudio = {0};

//			DirectSoundDoWork();

			
			DWORD iSize=m_dwPacketSize;
			if (len < m_dwPacketSize) 
			{
				// we don't accept half full packets...
				iSize=len;
				return iBytesCopied;
			}
			m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;

			// Set up audio packet
			xmpAudio.dwMaxSize        = iSize;
			xmpAudio.pvBuffer         = m_pbSampleData[dwIndex];
			xmpAudio.pdwStatus        = &m_adwStatus[ dwIndex ];
			xmpAudio.pdwCompletedSize = NULL;
			xmpAudio.prtTimestamp     = NULL;
			xmpAudio.pContext         = NULL;

			if (m_bMute)
				fast_memset(xmpAudio.pvBuffer, 0, iSize);
			else
				fast_memcpy(xmpAudio.pvBuffer,&data[iBytesCopied],iSize);

			// no need to do analogue out - analogue should be disabled as we're
			// passing non-PCM streams only using AC97
			hr=m_pDigitalOutput->Process( &xmpAudio, NULL );
			//  hr=m_pAnalogOutput->Process( &xmpAudio, NULL );

			iBytesCopied+=iSize;
			len -=iSize;
		}
		else 
		{
			break;
		}
	}
	
	return iBytesCopied;
}

//***********************************************************************************************
DWORD CAc97DirectSound::GetBytesInBuffer()
{
	DWORD dwBytesInBuffer=0;
	for( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if( m_adwStatus[ i ] == XMEDIAPACKET_STATUS_PENDING)
    {
			dwBytesInBuffer+=m_dwPacketSize;
		}
	}
	return dwBytesInBuffer;
}

//***********************************************************************************************
FLOAT CAc97DirectSound::GetDelay()
{
	return 0.028f;		//(fake PCM output 8ms) + (receiver 20ms)
}

//***********************************************************************************************
DWORD CAc97DirectSound::GetChunkLen()
{
	return m_dwPacketSize;
}
//***********************************************************************************************
int CAc97DirectSound::SetPlaySpeed(int iSpeed)
{

	return 1;
}

void CAc97DirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
	if (!m_pCallback)
	{
		pCallback->OnInitialize(m_wfx.nChannels, m_wfx.nSamplesPerSec, m_wfx.wBitsPerSample );
	}
	m_pCallback=pCallback;
}

void CAc97DirectSound::UnRegisterAudioCallback()
{
	m_pCallback=NULL;
}

void CAc97DirectSound::WaitCompletion()
{
	if (!m_pDigitalOutput)
		return;
	m_pDigitalOutput->Discontinuity();
	DWORD status;
	do {
		Sleep(10);
		m_pDigitalOutput->GetStatus(&status);
	}	while (status & DSSTREAMSTATUS_PLAYING);
}
