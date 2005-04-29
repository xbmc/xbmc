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

#include "../../stdafx.h"
#include <stdio.h>
#include "AsyncDirectSound.h"
#include "MPlayer.h"
#include "../../util.h"
#include "../../application.h" // Karaoke patch (114097)
#include "AudioContext.h"

#define VOLUME_MIN    DSBVOLUME_MIN
#define VOLUME_MAX    DSBVOLUME_MAX


#define CALC_DELAY_START   0
#define CALC_DELAY_STARTED 1
#define CALC_DELAY_DONE    2

static long buffered_bytes = 0;
//***********************************************************************************************
void CALLBACK CASyncDirectSound::StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
  CASyncDirectSound* This = (CASyncDirectSound*) pStreamContext;
  This->StreamCallback(pPacketContext, dwStatus);
}


//***********************************************************************************************
void CASyncDirectSound::StreamCallback(LPVOID pPacketContext, DWORD dwStatus)
{
  QueryPerformanceCounter(&m_LastPacketCompletedAt);

  buffered_bytes -= m_dwPacketSize;
  if (buffered_bytes < 0) buffered_bytes = 0;

  if (dwStatus == XMEDIAPACKET_STATUS_SUCCESS && m_VisBuffer)
  {
    if (m_VisBytes + m_dwPacketSize <= m_VisMaxBytes)
    {
      memcpy(m_VisBuffer + m_VisBytes, pPacketContext, m_dwPacketSize);
      m_VisBytes += m_dwPacketSize;
    }
    // COMMENTED by JM 6 April 2005.
    // This causes hardlockups when CLog::Log() then attempts to log.
    // removing the CSingleLock lock(critSec) in CLog::Log() also prevents
    // the lockup, but this is a much better hack-fix.
//    else
//      CLog::DebugLog("Vis buffer overflow");
  }
}

void CASyncDirectSound::DoWork()
{
  if (m_VisBytes && m_pCallback)
  {
    m_pCallback->OnAudioData(m_VisBuffer, m_VisBytes);
    m_VisBytes = 0;
  }

  g_application.m_CdgParser.ProcessVoice(); // Karaoke patch (114097)
}

bool CASyncDirectSound::GetMixBin(DSMIXBINVOLUMEPAIR* dsmbvp, int* MixBinCount, DWORD* dwChannelMask, int Type, int Channels)
{
  //3, 5, >6 channel are invalid XBOX wav formats thus can not be processed at this stage

  *MixBinCount = Channels;

  if (Channels == 6) //Handle 6 channels.
  {
    *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;

    switch (Type)
    {
    case DSMIXBINTYPE_DMO:  //FL, FR, C, LFE, SL, SR
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_AAC:  //C, FL, FR, SL, SR, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_OGG:  //FL, C, FR, SL, SR, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_STANDARD:  //FL, FR, SL, SR, C, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    }
    //Didn't manage to get anything
    CLog::Log(LOGERROR, "Invalid Mixbin type specified, reverting to standard");
    GetMixBin(dsmbvp, MixBinCount, dwChannelMask, DSMIXBINTYPE_STANDARD, Channels);
    return true;
  }
  else if (Channels == 4)
  {
    DSMIXBINVOLUMEPAIR dsm[4] = { DSMIXBINVOLUMEPAIRS_DEFAULT_4CHANNEL };
    memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
    *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    return true;
  }
  else if (Channels == 2)
  {
    if ( Type == DSMIXBINTYPE_STEREOALL )
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_FRONT_RIGHT, 0},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_BACK_RIGHT, 0},
          // left and right both to center and LFE, but attenuate each 3dB first
          // so they're the same level.
          // attenuate the center another 3dB so that it is a total 6dB lower
          // so that stereo effect is not lost.
          {DSMIXBIN_LOW_FREQUENCY, -301},
          {DSMIXBIN_LOW_FREQUENCY, -301},
          {DSMIXBIN_FRONT_CENTER, -602},
          {DSMIXBIN_FRONT_CENTER, -602}
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else if (Type == DSMIXBINTYPE_STEREOLEFT)
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          // left route to 4 channels
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_FRONT_RIGHT , 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_BACK_RIGHT, 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else if (Type == DSMIXBINTYPE_STEREORIGHT)
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          // right route to 4 channels
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_FRONT_RIGHT , 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_LOW_FREQUENCY, VOLUME_MIN},
          {DSMIXBIN_BACK_RIGHT, 0},
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else
    {
      DSMIXBINVOLUMEPAIR dsm[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    }
    return true;
  }
  else if (Channels == 1)
  {
    *MixBinCount = 2;
    DSMIXBINVOLUMEPAIR dsm[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_MONO };
    memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
    *dwChannelMask = SPEAKER_FRONT_LEFT;
    return true;
  }
  CLog::Log(LOGERROR, "Invalid Mixbin channels specified, get MixBins failed");
  return false;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CASyncDirectSound::CASyncDirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, int iNumBuffers, char* strAudioCodec)
{
  g_audioContext.RemoveActiveDevice();

  buffered_bytes = 0;
  m_pCallback = pCallback;

  m_bResampleAudio = false;
  if (bResample && g_guiSettings.GetBool("AudioOutput.HighQualityResampling") && uiSamplesPerSec != 48000)
    m_bResampleAudio = true;

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers);

  LARGE_INTEGER qwTicksPerSec;
  QueryPerformanceFrequency( &qwTicksPerSec );   // ticks/sec
  m_TicksPerSec = qwTicksPerSec.QuadPart;

  m_bPause = false;
  m_iAudioSkip = 1;
  m_bIsPlaying = false;
  m_bIsAllocated = false;
  m_pDSound = NULL;
  m_adwStatus = NULL;
  m_pStream = NULL;
  m_iCurrentAudioStream = 0;
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
  m_uiChannels = iChannels;
  QueryPerformanceCounter(&m_LastPacketCompletedAt);

  m_bFirstPackets = true;
  m_iCalcDelay = CALC_DELAY_START;
  m_fCurDelay = (FLOAT)0.001;
  m_delay = 1;
  ZeroMemory(&m_wfx, sizeof(m_wfx));
  m_wfx.cbSize = sizeof(m_wfx);
  XAudioCreatePcmFormat( iChannels,
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

  // if iNumBuffers is set, use this many buffers instead of the usual number (used for CDDAPlayer)
  if (iNumBuffers)
    m_dwNumPackets = (DWORD)iNumBuffers;
  else if (!mplayer_HasVideo())
    m_dwNumPackets = 64 * iChannels;
  else
    m_dwNumPackets = 12 * iChannels;

  m_adwStatus = new DWORD[ m_dwNumPackets ];
  m_pbSampleData = new PBYTE[ m_dwNumPackets ];

  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  m_pDSound=g_audioContext.GetDirectSoundDevice();

  m_nCurrentVolume = GetMaximumVolume();

  for ( DWORD j = 0; j < m_dwNumPackets; j++ )
    m_adwStatus[ j ] = XMEDIAPACKET_STATUS_SUCCESS;

  DSMIXBINS dsmb;

  DWORD dwCMask;
  DSMIXBINVOLUMEPAIR dsmbvp8[8];
  int iMixBinCount;

  ZeroMemory(&m_wfxex, sizeof(m_wfxex));

  m_wfxex.Format = m_wfx;
  m_wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  m_wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ;
  m_wfxex.Samples.wReserved = 0;

  if (bAudioOnAllSpeakers && iChannels == 2)
  {
    GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOALL, iChannels);
    m_wfxex.dwChannelMask = dwCMask;
    dsmb.dwMixBinCount = iMixBinCount;
    dsmb.lpMixBinVolumePairs = dsmbvp8;
  }
  else
  {
    if (strstr(strAudioCodec, "AAC"))
      GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_AAC, iChannels);
    else if (strstr(strAudioCodec, "DMO"))
      GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_DMO, iChannels);
    else if (strstr(strAudioCodec, "OggVorbis"))
      GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_OGG, iChannels);
    else
      GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STANDARD, iChannels);

    m_wfxex.dwChannelMask = dwCMask;
    dsmb.dwMixBinCount = iMixBinCount;
    dsmb.lpMixBinVolumePairs = dsmbvp8;
  }

  m_wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

  xbox_Ac3encoder_active = false;
  if ( (dsmb.dwMixBinCount > 2) && ((XGetAudioFlags()&(DSSPEAKER_ENABLE_AC3 | DSSPEAKER_ENABLE_DTS)) != 0 ) )
    xbox_Ac3encoder_active = true;

  DSSTREAMDESC dssd;
  memset(&dssd, 0, sizeof(dssd));

  dssd.dwFlags = DSSTREAMCAPS_ACCURATENOTIFY; // xbmp=0
  dssd.dwMaxAttachedPackets = m_dwNumPackets;
  dssd.lpwfxFormat = (WAVEFORMATEX*) & m_wfxex;
  dssd.lpfnCallback = StaticStreamCallback;
  dssd.lpvContext = this;
  dssd.lpMixBins = &dsmb;

  if (DirectSoundCreateStream( &dssd, &m_pStream ) != DS_OK)
  {
    CLog::Log(LOGERROR, "*WARNING* Unable to create sound stream!");
  }

  XMEDIAINFO info;
  m_pStream->GetInfo(&info);
  //align m_dwPacketSize to dwInputSize
  int fSize = 768 / info.dwInputSize;
  fSize *= info.dwInputSize;
  m_dwPacketSize = (int)fSize;

  // XphysicalAlloc has page (4k) granularity, so allocate all the buffers in one chunk to avoid wasting 3k per buffer
  m_pbSampleData[0] = (BYTE*)XPhysicalAlloc(m_dwPacketSize * m_dwNumPackets, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
  for (DWORD dwX = 1; dwX < m_dwNumPackets ; dwX++)
    m_pbSampleData[dwX] = m_pbSampleData[dwX - 1] + m_dwPacketSize;

  // set volume (from settings)
  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;
  m_pStream->SetVolume( m_nCurrentVolume );

  // Set the headroom of the stream to 0 (to allow the maximum volume)
  m_pStream->SetHeadroom(0);
  // Set the default mixbins headroom to appropriate level as set in the settings file (to allow the maximum volume)
  for (DWORD i = 0; i < dsmb.dwMixBinCount;i++)
    m_pDSound->SetMixBinHeadroom(i, DWORD(g_guiSettings.GetInt("AudioOutput.Headroom") / 6));

  m_bIsAllocated = true;
  if (m_pCallback)
  {
    if (m_bResampleAudio)
    {
      pCallback->OnInitialize(iChannels, 48000, 16);
      m_VisBuffer = (PBYTE)malloc(m_VisMaxBytes = iChannels * 96000 / 20);
    }
    else
    {
      m_pCallback->OnInitialize(iChannels, m_uiSamplesPerSec, m_uiBitsPerSample);
      m_VisBuffer = (PBYTE)malloc(m_VisMaxBytes = iChannels * m_uiSamplesPerSec * (m_uiBitsPerSample / 8) / 20);
    }
  }
  else
    m_VisBuffer = 0;
  m_VisBytes = 0;
  if (m_bResampleAudio)
  {
    m_Resampler.InitConverter(uiSamplesPerSec, uiBitsPerSample, iChannels, 48000, 16, m_dwPacketSize);
  }

  // Karaoke patch (114097) ...
  if ( g_guiSettings.GetBool("Karaoke.VoiceEnabled") )
  {
    CDG_VOICE_MANAGER_CONFIG VoiceConfig;
    VoiceConfig.dwVoicePacketTime = 20;       // 20ms
    VoiceConfig.dwMaxStoredPackets = 5;
    VoiceConfig.pDSound = m_pDSound;
    VoiceConfig.pCallbackContext = this;
    VoiceConfig.pfnVoiceDeviceCallback = NULL;
    VoiceConfig.pfnVoiceDataCallback = CdgVoiceDataCallback;
    g_application.m_CdgParser.StartVoice(&VoiceConfig);
  }
  // ... Karaoke patch (114097)
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

  g_application.m_CdgParser.FreeVoice(); // Karaoke patch (114097)

  m_bIsAllocated = false;
  if (m_pStream)
  {
    m_pStream->Flush();
    m_pStream->Pause( DSSTREAMPAUSE_PAUSE );

    m_pStream->Release();
    m_pStream = NULL;
  }
  if (m_VisBuffer)
    free(m_VisBuffer);
  m_VisBuffer = NULL;

  if (m_pbSampleData)
  {
    if (m_pbSampleData[0])
      XPhysicalFree(m_pbSampleData[0]);
    delete [] m_pbSampleData;
  }
  m_pbSampleData = NULL;

  if ( m_adwStatus )
    delete [] m_adwStatus;
  m_adwStatus = NULL;

  m_pDSound = NULL;
  g_audioContext.RemoveActiveDevice();
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return S_OK;
}


//***********************************************************************************************
HRESULT CASyncDirectSound::Pause()
{
  if (m_bPause) return S_OK;
  CLog::DebugLog("Pause stream");
  m_bPause = true;
  m_pStream->Flush();
  m_pStream->Pause( DSSTREAMPAUSE_PAUSE );
  return S_OK;
}

//***********************************************************************************************
HRESULT CASyncDirectSound::Resume()
{
  if (!m_bPause) return S_OK;
  m_bPause = false;
  m_pStream->Pause( DSSTREAMPAUSE_RESUME );

  return S_OK;
}

//***********************************************************************************************
HRESULT CASyncDirectSound::Stop()
{
  buffered_bytes = 0;
  if (m_bPause) return S_OK;

  // Check the status of each packet
  //mp_msg(0,0,"CASyncDirectSound::Stop");
  CLog::DebugLog("Stop stream");
  if (m_pStream)
  {
    m_pStream->Flush();
  }
  for ( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    m_adwStatus[ i ] = XMEDIAPACKET_STATUS_SUCCESS;
  }
  m_VisBytes = 0;
  m_LastPacketCompletedAt.QuadPart=0;

  m_bFirstPackets = true;
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
void CASyncDirectSound::Mute(bool bMute)
{
  if (bMute)
    m_pStream->SetVolume(GetMinimumVolume());
  else
    m_pStream->SetVolume(m_nCurrentVolume);
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
  for ( DWORD i = 0; i < m_dwNumPackets; i++ )
  {
    // If we find a non-pending packet, return it
    if ( m_adwStatus[ i ] != XMEDIAPACKET_STATUS_PENDING)
    {
      dwIndex = i;
      return true;
    }
  }
  return false;
}

//***********************************************************************************************
bool CASyncDirectSound::SupportsSurroundSound() const
{
  return false;
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetSpace()
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
  DWORD dwSize = iFreePackets * m_dwPacketSize;
  if (m_bResampleAudio)
  { // calculate the actual amount of data that we can handle
    float fBytesPerSecOutput = 48000.0f * 16 * m_wfx.nChannels; //use correct channel, do not assume 2 channel
    dwSize = (DWORD)((float)dwSize * (float)m_Resampler.GetInputBitrate() / fBytesPerSecOutput);
  }

  return dwSize;
}

// 48kHz resampled data method
DWORD CASyncDirectSound::AddPacketsResample(unsigned char *pData, DWORD iLeft)
{
  DWORD dwIndex = 0;
  DWORD iBytesCopied = 0;
  while (true)
  {
    // Get the next free packet to fill with our output audio
    if ( FindFreePacket(dwIndex) )
    {
      XMEDIAPACKET xmpAudio = {0};

      // loop around, grabbing data from the input buffer and resampling
      // until we fill up this packet
      while (true)
      {
        // check if we have resampled data to send
        if (m_Resampler.GetData(m_pbSampleData[dwIndex]))
        {
          // Set up audio packet
          m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;
          xmpAudio.dwMaxSize = m_dwPacketSize;
          xmpAudio.pvBuffer = m_pbSampleData[dwIndex];
          xmpAudio.pdwStatus = &m_adwStatus[ dwIndex ];
          xmpAudio.pdwCompletedSize = NULL;
          xmpAudio.prtTimestamp = NULL;
          xmpAudio.pContext = m_pbSampleData[dwIndex];
          // Process the audio
          if (DS_OK != m_pStream->Process( &xmpAudio, NULL ))
          {
            return iBytesCopied;
          }
          // Okay - we've done this bit, update our data info
          buffered_bytes += m_dwPacketSize;
          // break to get another packet and restart the loop
          break;
        }
        else
        { // put more data into the resampler
          int iSize = m_Resampler.PutData(&pData[iBytesCopied], iLeft);
          if (iSize == -1)
          { // Failed - we don't have enough data
            return iBytesCopied;
          }
          else
          { // Success - update the amount that we have processed
            iBytesCopied += (DWORD)iSize;
            iLeft -= iSize;
          }
          // Now loop back around and output data, or read more in
        }
      }
    }
    else
    { // no packets free - send data back to sender
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

  DWORD dwIndex = 0;
  DWORD iBytesCopied = 0;

  while (len)
  {
    if ( FindFreePacket(dwIndex) )
    {
      XMEDIAPACKET xmpAudio = {0};

      DWORD iSize = m_dwPacketSize;
      if (len < m_dwPacketSize)
      {
        // we don't accept half full packets...
        iSize = len;
        return iBytesCopied;
      }
      m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;

      // Set up audio packet

      xmpAudio.dwMaxSize = iSize / m_iAudioSkip;
      xmpAudio.pvBuffer = m_pbSampleData[dwIndex];
      xmpAudio.pdwStatus = &m_adwStatus[ dwIndex ];
      xmpAudio.pdwCompletedSize = NULL;
      xmpAudio.prtTimestamp = NULL;
      xmpAudio.pContext = m_pbSampleData[dwIndex];

      fast_memcpy(xmpAudio.pvBuffer, &data[iBytesCopied], iSize / m_iAudioSkip);
      //   if (m_pCallback)
      //   {
      //    m_pCallback->OnAudioData(&data[iBytesCopied],iSize/m_iAudioSkip);
      //   }
      if (DS_OK != m_pStream->Process( &xmpAudio, NULL ))
      {
        return iBytesCopied;
      }
      buffered_bytes += (iSize / m_iAudioSkip);
      iBytesCopied += iSize;
      len -= iSize;
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
      m_lFadeVolume += 200;
      m_pStream->SetVolume( m_lFadeVolume + VOLUME_MIN);
    }
    else
    {
      m_pStream->SetVolume( m_nCurrentVolume + VOLUME_MIN);
      m_bFirstPackets = false;
    }
  }
#endif
  return iBytesCopied;
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetBytesInBuffer()
{
  //Calculate how much of current packet that has been played, so we can adjust with that
  LARGE_INTEGER llPerfCount;
  QueryPerformanceCounter(&llPerfCount);

  LONGLONG adjustbytes = (llPerfCount.QuadPart-m_LastPacketCompletedAt.QuadPart) * m_uiChannels * m_uiSamplesPerSec / m_TicksPerSec;
  if( adjustbytes < m_dwPacketSize && adjustbytes <= buffered_bytes)
    return buffered_bytes - (DWORD)adjustbytes;

  return buffered_bytes;
}

void CASyncDirectSound::ResetBytesInBuffer()
{
  buffered_bytes = 0;
}

//***********************************************************************************************
FLOAT CASyncDirectSound::GetDelay()
{
  if (xbox_Ac3encoder_active) //2 channel materials will not be encoded as ac3 stream
    return 0.049f;      //(Ac3 encoder 29ms)+(receiver 20ms)
  else
    return 0.008f;      //PCM output 8ms
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetChunkLen()
{
  if (m_bResampleAudio)
    return m_Resampler.GetMaxInputSize();
  else
    return m_dwPacketSize;
}
//***********************************************************************************************
int CASyncDirectSound::SetPlaySpeed(int iSpeed)
{
  DWORD ret_val = 0;

  DWORD OrgFreq = (DWORD)m_uiSamplesPerSec;
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
    if (!(1152 % iSpeed))  // We have to copy complete samples, Have sample may crash direct sound
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
    {
      pCallback->OnInitialize(m_wfx.nChannels, 48000, 16);
      m_VisBuffer = (PBYTE)malloc(m_VisMaxBytes = m_wfx.nChannels * 96000 / 20);
      m_VisBytes = 0;
    }
    else
    {
      pCallback->OnInitialize(m_wfx.nChannels, m_wfx.nSamplesPerSec, m_wfx.wBitsPerSample );
      m_VisBuffer = (PBYTE)malloc(m_VisMaxBytes = m_wfx.nChannels * m_uiSamplesPerSec * (m_uiBitsPerSample / 8) / 20);
      m_VisBytes = 0;
    }
  }
  m_pCallback = pCallback;
}

void CASyncDirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
  free(m_VisBuffer);
  m_VisBuffer = NULL;
}

void CASyncDirectSound::WaitCompletion()
{
  if (!m_pStream)
    return ;

  m_pStream->Discontinuity();
  DWORD status;
  do
  {
    Sleep(10);
    m_pStream->GetStatus(&status);
  }
  while (status & DSSTREAMSTATUS_PLAYING);
}

void CASyncDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  if (m_iCurrentAudioStream == iAudioStream)
    return ;
  if (!m_pStream)
    return ;

  DSMIXBINS dsmb;
  DWORD dwCMask;
  DSMIXBINVOLUMEPAIR dsmbvp8[8];
  int iMixBinCount;

  switch ( iAudioStream )
  {
  case 0:     //Normal
    if ( bAudioOnAllSpeakers )
      GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOALL, 2);
    else
      GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STANDARD, 2);
    break;
  case 1:     //Left only
    GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOLEFT, 2);
    break;
  case 2:     //Right only
    GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREORIGHT, 2);
    break;
  default:    //Undefined
    CLog::Log(LOGERROR, "Invalid Audio channel type specified, doing nothing");
    return ;
  }

  m_wfxex.dwChannelMask = dwCMask;
  dsmb.dwMixBinCount = iMixBinCount;
  dsmb.lpMixBinVolumePairs = dsmbvp8;

  m_pStream->SetFormat((LPCWAVEFORMATEX)&m_wfxex);
  m_pStream->SetMixBins(&dsmb);
  m_pStream->SetVolume( m_nCurrentVolume );
  // Set the headroom of the stream to 0 (to allow the maximum volume)
  m_pStream->SetHeadroom(0);
  // Set the default mixbins headroom to appropriate level as set in the settings file (to allow the maximum volume)
  for (DWORD i = 0; i < dsmb.dwMixBinCount;i++)
    m_pDSound->SetMixBinHeadroom(i, DWORD(g_guiSettings.GetInt("AudioOutput.Headroom") / 6));
  m_iCurrentAudioStream = iAudioStream;
}

// Voice Manager Callback (Karaoke patch (114097))
void CASyncDirectSound::CdgVoiceDataCallback( DWORD dwPort, DWORD dwSize, VOID* pvData, VOID* pContext )
{
  CASyncDirectSound* pThis = (CASyncDirectSound*) pContext;
  if (pThis->m_pCallback)
    pThis->m_pCallback->OnAudioData( (unsigned char*) pvData , dwSize );
}
