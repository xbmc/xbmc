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

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

#include <stdio.h>
#include "AsyncDirectSound.h"
#include "../../settings.h"


#define VOLUME_MIN		-6000
#define VOLUME_MAX		DSBVOLUME_MAX

static DWORD dwDonePkts=0; // usefull for debugging
static DWORD dwSendPkts=0; // usefull for debugging


#define CALC_DELAY_START	 0
#define CALC_DELAY_STARTED 1
#define CALC_DELAY_DONE		 2

//***********************************************************************************************
void CALLBACK CASyncDirectSound::StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
	CASyncDirectSound* This = (CASyncDirectSound*) pStreamContext;
	This->StreamCallback(pPacketContext, dwStatus);
}


//***********************************************************************************************
void CASyncDirectSound::StreamCallback(LPVOID pPacketContext, DWORD dwStatus)
{
	dwDonePkts++;
	/*if (CALC_DELAY_STARTED==m_iCalcDelay && ((LPVOID)0x1234)== pPacketContext )
	{
		LARGE_INTEGER curTime;
		QueryPerformanceCounter( &curTime);
		m_delay = curTime.QuadPart-m_startTime ; 
		m_iCalcDelay = CALC_DELAY_DONE;
	}*/
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CASyncDirectSound::CASyncDirectSound(IAudioCallback* pCallback,int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample)
{
	OutputDebugString("CASyncDirectSound() ctor\n");
	m_pCallback=pCallback;
	if(!g_stSettings.m_bAudioOnAllSpeakers ) 
	{
		if (iChannels == 1)
			DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
		else if (iChannels == 2)
			DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
		else
			DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
	} 
	else 
	{
		DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
	}

	LARGE_INTEGER qwTicksPerSec;
  QueryPerformanceFrequency( &qwTicksPerSec );   // ticks/sec
	m_dwTicksPerSec=qwTicksPerSec.QuadPart;
	//mp_msg(0,0,"init directsound chns:%i rate:%i, bits:%i",iChannels,uiSamplesPerSec,uiBitsPerSample);
	m_dwPacketSize		 = 1152 * (uiBitsPerSample/8) * iChannels;
	m_bPause           = false;
	m_iAudioSkip	   = 1;
	m_bIsPlaying       = false;
	m_bIsAllocated     = false;
	m_pDSound          = NULL;
	m_adwStatus        = NULL;
	m_pStream          = NULL;
	m_adwStatus        = NULL;
	m_uiSamplesPerSec = uiSamplesPerSec;
	ZeroMemory(m_pbSampleData,sizeof(m_pbSampleData));
	m_bFirstPackets		 = true;
	m_iCalcDelay       = CALC_DELAY_START;
	m_fCurDelay        = (FLOAT)0.001;
	m_delay            = 1;
	dwDonePkts=0;
	dwSendPkts=0;
	ZeroMemory(&m_wfx,sizeof(m_wfx)); 
	m_wfx.cbSize=sizeof(m_wfx);
	XAudioCreatePcmFormat(  iChannels,
													uiSamplesPerSec,
													uiBitsPerSample,
													&m_wfx
													);


	// Create enough samples to hold approx 2 sec worth of audio.
	m_dwNumPackets = ( (m_wfx.nSamplesPerSec / ( m_dwPacketSize / ((uiBitsPerSample/8) * m_wfx.nChannels) )) / 2);
	for (DWORD dwX=0; dwX < m_dwNumPackets ; dwX++)
		m_pbSampleData[dwX] = (BYTE*)XPhysicalAlloc( m_dwPacketSize, MAXULONG_PTR,0,PAGE_READWRITE|PAGE_NOCACHE);
	m_adwStatus		 = new DWORD[ m_dwNumPackets ];

	 
	  // Create DirectSound
	HRESULT hr;
	hr= DirectSoundCreate( NULL, &m_pDSound, NULL ) ;
  if( DS_OK != hr  )
	{
			//mp_msg(0,0,"DirectSoundCreate() Failed");
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
//	m_wfxex.Samples.wSamplesPerBlock    =0;
//	m_wfxex.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;

	m_wfxex.Samples.wReserved=0;
	m_wfxex.dwChannelMask    = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT;
	dsmb.dwMixBinCount			 = 6;
	dsmb.lpMixBinVolumePairs = dsmbvp6;

	if (!g_stSettings.m_bAudioOnAllSpeakers)
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
				dsmb.dwMixBinCount			 = 5;
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

	DSSTREAMDESC dssd; 
	memset(&dssd,0,sizeof(dssd));

	dssd.dwFlags							= DSSTREAMCAPS_ACCURATENOTIFY; // xbmp=0
	dssd.dwMaxAttachedPackets = m_dwNumPackets;
	dssd.lpwfxFormat					= (WAVEFORMATEX*)&m_wfxex;
	dssd.lpfnCallback					= StaticStreamCallback;
	dssd.lpvContext						= this;
	dssd.lpMixBins						= &dsmb;

	if(DirectSoundCreateStream( &dssd, &m_pStream )!=DS_OK)
	{
//		OUTPUTDEBUG("*WARNING* Unable to create sound stream!");
	}

	m_nCurrentVolume = GetMaximumVolume();
#ifdef FADE_IN
	m_lFadeVolume = 0;
#else
	m_lFadeVolume = m_nCurrentVolume;
#endif
	m_pStream->SetVolume( m_lFadeVolume + VOLUME_MIN);

	m_pStream->SetHeadroom(0);
	m_pStream->Flush();
	
	m_bIsAllocated   = true;
	if (m_pCallback)
	{
		m_pCallback->OnInitialize(iChannels, uiSamplesPerSec, uiBitsPerSample);
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
	return 0;
}

//***********************************************************************************************
LONG CASyncDirectSound::GetMaximumVolume() const
{
	return 6000;
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
	return m_pStream->SetVolume( nVolume + VOLUME_MIN);
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

//***********************************************************************************************
DWORD CASyncDirectSound::AddPackets(unsigned char *data, DWORD len)
{

	DWORD		dwIndex = 0;
	DWORD   iBytesCopied=0;
	if (m_pCallback)
	{
		m_pCallback->OnAudioData(data,len);
	}

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
			xmpAudio.dwMaxSize        = iSize/m_iAudioSkip;
			xmpAudio.pvBuffer         = m_pbSampleData[dwIndex];
			xmpAudio.pdwStatus        = &m_adwStatus[ dwIndex ];
			xmpAudio.pdwCompletedSize = NULL;
			xmpAudio.prtTimestamp     = NULL;
			xmpAudio.pContext         = NULL;

			memcpy(xmpAudio.pvBuffer,&data[iBytesCopied],iSize);

/*			if (m_iCalcDelay == CALC_DELAY_START)
			{
				// start a new delay measurement
				xmpAudio.pContext=((LPVOID)0x1234);	// calculate delay for this packet
				LARGE_INTEGER curTime;
				QueryPerformanceCounter( &curTime);		// set start time
				m_startTime = curTime.QuadPart;
				m_iCalcDelay = CALC_DELAY_STARTED;	// run it
			}

			if (m_iCalcDelay == CALC_DELAY_DONE)
			{
				m_fCurDelay = (FLOAT(m_delay))/ ((FLOAT)m_dwTicksPerSec);

			}
*/
			if (DS_OK != m_pStream->Process( &xmpAudio, NULL ))
			{
				//m_iCalcDelay = CALC_DELAY_DONE;
				//mp_msg(0,0,"IDirectSoundStream::Process() failed");
				return iBytesCopied;
			}
			dwSendPkts++;
			////mp_msg(0,0,"audio decoder process done");
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
FLOAT CASyncDirectSound::GetDelay()
{
	return 0.0;
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetChunkLen()
{
	return m_dwPacketSize;
}
//***********************************************************************************************
int CASyncDirectSound::SetPlaySpeed(int iSpeed)
{
	DWORD	ret_val=0;

  DWORD OrgFreq=(DWORD)m_uiSamplesPerSec;
  DWORD NewFreq;
  if (!m_pStream)
          return 0;

  if (iSpeed < 0)
		return 0;

	iSpeed++;

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
		pCallback->OnInitialize(m_wfx.nChannels, m_wfx.nSamplesPerSec, m_wfx.wBitsPerSample );
	}
	m_pCallback=pCallback;
}

void CASyncDirectSound::UnRegisterAudioCallback()
{
	m_pCallback=NULL;
}

