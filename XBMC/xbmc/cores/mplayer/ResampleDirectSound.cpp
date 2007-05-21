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
#include "ResampleDirectSound.h"
#include "AsyncDirectSound.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CResampleDirectSound::CResampleDirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, char* strAudioCodec, bool bIsMusic)
{
  m_uiChannels = iChannels;

  m_pRenderer = new CASyncDirectSound(pCallback, iChannels, 48000, 16, strAudioCodec, bIsMusic);

  m_dwOutputSize = m_pRenderer->GetChunkLen();
  m_pSampleData = new unsigned char[m_dwOutputSize];

  m_Resampler.InitConverter(uiSamplesPerSec, uiBitsPerSample, iChannels, 48000, 16, m_dwOutputSize);

  m_dwInputSize = m_Resampler.GetInputSize();
}

//***********************************************************************************************
CResampleDirectSound::~CResampleDirectSound()
{
  Deinitialize();
  delete[] m_pSampleData;
  delete m_pRenderer;
}

//***********************************************************************************************
DWORD CResampleDirectSound::GetSpace()
{
  DWORD dwSize = m_pRenderer->GetSpace();

  // recalculate based on our actual input
  float fBytesPerSecOutput = 48000.0f * 16 * m_uiChannels;
  dwSize = (DWORD)((float)dwSize * (float)m_Resampler.GetInputBitrate() / fBytesPerSecOutput);

  // TODO, should subtract what's already buffered in resampler
  return dwSize;
}

DWORD CResampleDirectSound::AddPackets(unsigned char* data, DWORD len)
{
  DWORD copied = 0;

  // must atleast consume the chunklen we have set
  // caller might be relying on that
  len = min(len, m_dwInputSize);

  // loop around, grabbing data from the input buffer and resampling
  // until we fill up this packet and our given chunk len
  while (copied < len)
  {
    // check if we have resampled data to send
    while(m_Resampler.GetData(m_pSampleData))
    {
      // make sure all of our resampled data is copied
      // to next renderer

      DWORD bytes = 0;
      while(true)
      {
        bytes += m_pRenderer->AddPackets(m_pSampleData+bytes, m_dwOutputSize-bytes);
        if(m_dwOutputSize-bytes <= 0)
          break;

        Sleep(1);
        continue;
      }
    }

    int size = m_Resampler.PutData(data+copied, len-copied);
    if (size == -1) // Failed - we don't have enough data
      return copied;

    copied += size;
  }
  return copied;
}

void CResampleDirectSound::DoWork()
{
  m_pRenderer->DoWork();
}

HRESULT CResampleDirectSound::Deinitialize()
{  
  return m_pRenderer->Deinitialize();
}

HRESULT CResampleDirectSound::Pause()
{
  return m_pRenderer->Pause();
}

HRESULT CResampleDirectSound::Resume()
{
  return m_pRenderer->Resume();
}

HRESULT CResampleDirectSound::Stop()
{
  return m_pRenderer->Stop();
}

LONG CResampleDirectSound::GetMinimumVolume() const
{
  return m_pRenderer->GetMinimumVolume();
}

LONG CResampleDirectSound::GetMaximumVolume() const
{
  return m_pRenderer->GetMaximumVolume();
}

LONG CResampleDirectSound::GetCurrentVolume() const
{
  return m_pRenderer->GetCurrentVolume();
}

void CResampleDirectSound::Mute(bool bMute)
{
  return m_pRenderer->Mute(bMute);
}

HRESULT CResampleDirectSound::SetCurrentVolume(LONG nVolume)
{
  return m_pRenderer->SetCurrentVolume(nVolume);
}

FLOAT CResampleDirectSound::GetDelay()
{
  // TODO, should take resampler buffer into account
  // currently it has no interface for that thou
  return m_pRenderer->GetDelay();
}

DWORD CResampleDirectSound::GetChunkLen()
{
  return m_dwInputSize;
}

int CResampleDirectSound::SetPlaySpeed(int iSpeed)
{
  return m_pRenderer->SetPlaySpeed(iSpeed);
}

void CResampleDirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pRenderer->RegisterAudioCallback(pCallback);
}

void CResampleDirectSound::UnRegisterAudioCallback()
{
  m_pRenderer->UnRegisterAudioCallback();
}

void CResampleDirectSound::WaitCompletion()
{
  m_pRenderer->WaitCompletion();
}

void CResampleDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  m_pRenderer->SwitchChannels(iAudioStream, bAudioOnAllSpeakers);
}

void CResampleDirectSound::SetDynamicRangeCompression(long drc) 
{ 
  m_pRenderer->SetDynamicRangeCompression(drc); 
}
