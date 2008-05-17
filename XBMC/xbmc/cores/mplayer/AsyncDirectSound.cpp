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
#include <stdio.h>
#include "AsyncDirectSound.h"
#include "MPlayer.h"
#include "Application.h" // Karaoke patch (114097)
#include "AudioContext.h"
#include "CdgParser.h"

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
//      CLog::Log(LOGDEBUG,"Vis buffer overflow");
  }
}

void CASyncDirectSound::DoWork()
{
  if (m_VisBytes && m_pCallback)
  {
    m_pCallback->OnAudioData(m_VisBuffer, m_VisBytes);
    m_VisBytes = 0;
  }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CASyncDirectSound::CASyncDirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, const char* strAudioCodec, bool bIsMusic)
{
  buffered_bytes = 0;
  m_pCallback = pCallback;

  m_drcTable = NULL;
  m_drcAmount = 0;
  // TODO DRC
  if (!bIsMusic && uiBitsPerSample == 16) SetDynamicRangeCompression((long)(g_stSettings.m_currentVideoSettings.m_VolumeAmplification * 100));

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers,bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  m_pDSound=g_audioContext.GetDirectSoundDevice();

  LARGE_INTEGER qwTicksPerSec;
  QueryPerformanceFrequency( &qwTicksPerSec );   // ticks/sec
  m_TicksPerSec = qwTicksPerSec.QuadPart;

  m_bPause = false;
  m_iAudioSkip = 1;
  m_bIsPlaying = false;
  m_bIsAllocated = false;
  m_adwStatus = NULL;
  m_pStream = NULL;
  m_iCurrentAudioStream = 0;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;
  m_uiChannels = iChannels;
  QueryPerformanceCounter(&m_LastPacketCompletedAt);

  m_bFirstPackets = true;
  m_iCalcDelay = CALC_DELAY_START;
  m_fCurDelay = (FLOAT)0.001;
  m_delay = 1;
  ZeroMemory(&m_wfx, sizeof(m_wfx));
  m_wfx.cbSize = sizeof(m_wfx);

  // we want 1/4th of second worth of data
#if 0 //mplayer is broken on some packets sizes
  m_dwPacketSize = 512; // samples
  m_dwPacketSize *= m_uiChannels * (m_uiBitsPerSample>>3);
#else
  m_dwPacketSize = 768;
#endif
  m_dwNumPackets = m_uiSamplesPerSec * m_uiChannels * (m_uiBitsPerSample>>3);
  m_dwNumPackets /= m_dwPacketSize * 4;

  XAudioCreatePcmFormat( iChannels,
                         m_uiSamplesPerSec,
                         m_uiBitsPerSample,
                         &m_wfx
                       );

  m_adwStatus = new DWORD[ m_dwNumPackets ];
  m_pbSampleData = new PBYTE[ m_dwNumPackets ];

  m_nCurrentVolume = GetMaximumVolume();

  for ( DWORD j = 0; j < m_dwNumPackets; j++ )
    m_adwStatus[ j ] = XMEDIAPACKET_STATUS_SUCCESS;

  ZeroMemory(&m_wfxex, sizeof(m_wfxex));

  m_wfxex.Format = m_wfx;
  m_wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  m_wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ;
  m_wfxex.Samples.wReserved = 0;
  m_wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

  DSMIXBINS dsmb;

  DWORD dwCMask = 0;
  DSMIXBINVOLUMEPAIR dsmbvp8[8];
  int iMixBinCount;

  if (bAudioOnAllSpeakers && iChannels == 2)
  {
    g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOALL, iChannels);
    m_wfxex.dwChannelMask = dwCMask;
    dsmb.dwMixBinCount = iMixBinCount;
    dsmb.lpMixBinVolumePairs = dsmbvp8;
  }
  else
  {
    if (strstr(strAudioCodec, "AAC"))
      g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_AAC, iChannels);
    else if (strstr(strAudioCodec, "DMO")  // this should potentially be made default as it's standard windows
          || strstr(strAudioCodec, "FLAC")
          || strstr(strAudioCodec, "COOK")
          || strstr(strAudioCodec, "PCM"))
      g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_DMO, iChannels);
    else if (strstr(strAudioCodec, "Vorbis"))
      g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_OGG, iChannels);
    else
      g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STANDARD, iChannels);

    m_wfxex.dwChannelMask = dwCMask;
    dsmb.dwMixBinCount = iMixBinCount;
    dsmb.lpMixBinVolumePairs = dsmbvp8;
  }

  DSSTREAMDESC dssd;
  memset(&dssd, 0, sizeof(dssd));

  dssd.dwFlags = DSSTREAMCAPS_ACCURATENOTIFY; // xbmp=0
  dssd.dwMaxAttachedPackets = m_dwNumPackets;
  dssd.lpwfxFormat = (WAVEFORMATEX*) & m_wfxex;
  dssd.lpfnCallback = StaticStreamCallback;
  dssd.lpvContext = this;

  if(iMixBinCount)
    dssd.lpMixBins = &dsmb;

  if (DirectSoundCreateStream( &dssd, &m_pStream ) != DS_OK)
  {
    CLog::Log(LOGERROR, "*WARNING* Unable to create sound stream!");
  }

  // align m_dwPacketSize to dwInputSize
  XMEDIAINFO info;
  m_pStream->GetInfo(&info);    
  m_dwPacketSize /= info.dwInputSize;
  m_dwPacketSize *= info.dwInputSize;

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
    m_pDSound->SetMixBinHeadroom(i, DWORD(g_advancedSettings.m_audioHeadRoom / 6));

  m_bIsAllocated = true;
  if (m_pCallback)
  {
    m_pCallback->OnInitialize(iChannels, m_uiSamplesPerSec, m_uiBitsPerSample);
    m_VisBuffer = (PBYTE)malloc(m_VisMaxBytes = iChannels * m_uiSamplesPerSec * (m_uiBitsPerSample / 8) / 20);
  }
  else
    m_VisBuffer = 0;
  m_VisBytes = 0;
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

  // CDGParser needs to be close since closefile could be called from mplayer
  // WHY?????, what does it matter who removes this for the cdg parser?
  if( g_application.m_pCdgParser )
    g_application.m_pCdgParser->Stop(); 

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
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  if (m_drcTable)
    delete [] m_drcTable;
  m_drcTable = NULL;

  return S_OK;
}


//***********************************************************************************************
HRESULT CASyncDirectSound::Pause()
{
  if (m_bPause) return S_OK;
  CLog::Log(LOGDEBUG,"Pause stream");
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
  CLog::Log(LOGDEBUG,"Stop stream");
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
  return DSBVOLUME_MIN;
}

//***********************************************************************************************
LONG CASyncDirectSound::GetMaximumVolume() const
{
  return DSBVOLUME_MAX;
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
  return iFreePackets * m_dwPacketSize;
}

//***********************************************************************************************
DWORD CASyncDirectSound::AddPackets(unsigned char *data, DWORD len)
{
  DWORD dwIndex = 0;
  DWORD iBytesCopied = 0;

  while (len >= m_dwPacketSize)
  {
    if ( FindFreePacket(dwIndex) )
    {
      XMEDIAPACKET xmpAudio = {0};

      m_adwStatus[ dwIndex ] = XMEDIAPACKET_STATUS_PENDING;

      // Set up audio packet

      xmpAudio.dwMaxSize = m_dwPacketSize / m_iAudioSkip;
      xmpAudio.pvBuffer = m_pbSampleData[dwIndex];
      xmpAudio.pdwStatus = &m_adwStatus[ dwIndex ];
      xmpAudio.pdwCompletedSize = NULL;
      xmpAudio.prtTimestamp = NULL;
      xmpAudio.pContext = m_pbSampleData[dwIndex];

      if (m_drcAmount)
        ApplyDynamicRangeCompression(xmpAudio.pvBuffer, &data[iBytesCopied], m_dwPacketSize / m_iAudioSkip);
      else
        memcpy(xmpAudio.pvBuffer, &data[iBytesCopied], m_dwPacketSize / m_iAudioSkip);

      if (DS_OK != m_pStream->Process( &xmpAudio, NULL ))
        return iBytesCopied;

      buffered_bytes += (m_dwPacketSize / m_iAudioSkip);
      iBytesCopied += m_dwPacketSize;
      len -= m_dwPacketSize;
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
      m_pStream->SetVolume( m_lFadeVolume + DSBVOLUME_MIN);
    }
    else
    {
      m_pStream->SetVolume( m_nCurrentVolume + DSBVOLUME_MIN);
      m_bFirstPackets = false;
    }
  }
#endif
  return iBytesCopied;
}

//***********************************************************************************************
FLOAT CASyncDirectSound::GetDelay()
{
  FLOAT delay = 0.0f;

  // calculate delay in buffer
  delay += (FLOAT)buffered_bytes / (m_uiChannels * m_uiSamplesPerSec * (m_uiBitsPerSample>>3));

  // correct this delay by how long since we completed last packet
  LARGE_INTEGER llPerfCount;
  QueryPerformanceCounter(&llPerfCount);
  delay -= (FLOAT)(llPerfCount.QuadPart-m_LastPacketCompletedAt.QuadPart) / m_TicksPerSec;

  if(delay < 0.0)
    delay = 0.0;

  // add the static delay in the output
  if (g_audioContext.IsAC3EncoderActive()) //2 channel materials will not be encoded as ac3 stream
    delay += 0.049f;      //(Ac3 encoder 29ms)+(receiver 20ms)
  else
    delay += 0.008f;      //PCM output 8ms

  return delay;
}

//***********************************************************************************************
DWORD CASyncDirectSound::GetChunkLen()
{
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
    pCallback->OnInitialize(m_wfx.nChannels, m_wfx.nSamplesPerSec, m_wfx.wBitsPerSample );
    m_VisBuffer = (PBYTE)malloc(m_VisMaxBytes = m_wfx.nChannels * m_uiSamplesPerSec * (m_uiBitsPerSample / 8) / 20);
    m_VisBytes = 0;
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
      g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOALL, 2);
    else
      g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STANDARD, 2);
    break;
  case 1:     //Left only
    g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOLEFT, 2);
    break;
  case 2:     //Right only
    g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREORIGHT, 2);
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
    m_pDSound->SetMixBinHeadroom(i, DWORD(g_advancedSettings.m_audioHeadRoom / 6));
  m_iCurrentAudioStream = iAudioStream;
}


void GetSigmoidCurve(float scale, short *curve)
{
  ASSERT(scale > 1);
  // f(x) = a/(1 + e^{-bx}) + c

  // want it to map 0 .. 1, and have f'(0) = scale > 1
  // f(0) = a/2 + c = 0
  // f(1) = a/(1 + e^{-b}) + c = 1
  // f'(0) = -a/(1 + 1)^2.1.-b = ab/4 = scale
  // so a = 4scale/b
  // f(0) = 2scale/b + c = 0
  // f(1) = 4scale/b(1 + e^{-b}) + c = 1

  float b = 8/3*scale;   // reasonable approximation at least when k > 4
  for (int i = 0; i < 2000; i++)
  {
    // Newton-Raphson approximation
    float eb = exp(-b);
    float f = 4*scale/(b*(1 + eb)) - 2*scale/b - 1;
    float fp = -4*scale/(b*(1 + eb)*b*(1+eb))*(b*(1 - eb)+(1 + eb)) + 2*scale/(b*b);
    float b1 = b - f/fp;
    if (fabs(b1-b) < 1e-10)
      break;
    b = b1;
  }
  float a = 4*scale / b;
  float c = -a/2;

  // generate our mapping now
  for (float x = 0.0f; x <= 32767.0f; x++)
  {
    float y = a/(1 + exp(-b*(x/32767)))+c;
    *curve++ = (short)(y * 32767.0f + 0.5f);
  }
}

void GetCompressionCurve(float drc, short *curve)
{
  float powerdB = (90.0f - drc) / 90.0f;
  float scaledB = pow(32767.0f, 1 - powerdB);
  for (float in = 0; in <= 32767.0f; in++)
  {
    float out = pow(in, powerdB) * scaledB;
    *curve++ = (short)(out + 0.5f);
  }
}

void CASyncDirectSound::SetDynamicRangeCompression(long drc)
{
  if (m_drcAmount == drc)
    return;

  m_drcAmount = drc;

  // compute DRC table
  if (!m_drcTable && m_drcAmount)
    m_drcTable = new short[32768];

#define USE_SIGMOID

#ifdef USE_SIGMOID
  if (m_drcAmount)
  {
    float scaledB = pow(10.0f, m_drcAmount * 0.01f / 20);
    GetSigmoidCurve(scaledB, m_drcTable);
  }
  else
  { // no amplification, so reset to a linear scale
    for (int i = 0; i < 32768; i++)
      m_drcTable[i] = i;
  }
#else
  GetCompressionCurve(m_drcAmount*0.01f, m_drcTable);
#endif
}

void CASyncDirectSound::ApplyDynamicRangeCompression(void *dest, const void *source, const int bytes)
{
  if (!m_drcTable)
    return;
  const int shorts = bytes >> 1;
  short *input = (short *)source;
  short *output = (short *)dest;
  // now do the conversion
  for (int i = 0; i < shorts; i++)
  {
    short out = m_drcTable[abs(*input)];
    if (*input++ < 0) out = -out;
    *output++ = out;
  }
}
