/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "NullDirectSound.h"
#include "AudioContext.h"
#include "Util.h"

void CNullDirectSound::DoWork()
{

}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CNullDirectSound::CNullDirectSound()
{
}
bool CNullDirectSound::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  CLog::Log(LOGERROR,"Creating a Null Audio Renderer, Check your audio settings as this should not happen");
  if (iChannels == 0)
    iChannels = 2;

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_dwPacketSize = iChannels*(uiBitsPerSample/8)*512;

  return true;
}

//***********************************************************************************************
CNullDirectSound::~CNullDirectSound()
{
  Deinitialize();
}


//***********************************************************************************************
HRESULT CNullDirectSound::Deinitialize()
{
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  return S_OK;
}

void CNullDirectSound::Flush()
{
}

//***********************************************************************************************
HRESULT CNullDirectSound::Pause()
{
  return S_OK;
}

//***********************************************************************************************
HRESULT CNullDirectSound::Resume()
{
  return S_OK;
}

//***********************************************************************************************
HRESULT CNullDirectSound::Stop()
{
  return S_OK;
}

//***********************************************************************************************
LONG CNullDirectSound::GetMinimumVolume() const
{
  return -60;
}

//***********************************************************************************************
LONG CNullDirectSound::GetMaximumVolume() const
{
  return 60;
}

//***********************************************************************************************
LONG CNullDirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CNullDirectSound::Mute(bool bMute)
{
}

//***********************************************************************************************
HRESULT CNullDirectSound::SetCurrentVolume(LONG nVolume)
{
  m_nCurrentVolume = nVolume;
  return S_OK;
}


//***********************************************************************************************
DWORD CNullDirectSound::GetSpace()
{
  return GetChunkLen();
}

//***********************************************************************************************
DWORD CNullDirectSound::AddPackets(unsigned char *data, DWORD len)
{
  return len;
}

//***********************************************************************************************
FLOAT CNullDirectSound::GetDelay()
{
  return 0.0;
}

//***********************************************************************************************
DWORD CNullDirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}
//***********************************************************************************************
int CNullDirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CNullDirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
}

void CNullDirectSound::UnRegisterAudioCallback()
{
}

void CNullDirectSound::WaitCompletion()
{
}

void CNullDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}
