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
#include "Application.h"

#define BUFFER CHUNKLEN * 20
#define CHUNKLEN 512


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

  g_application.m_guiDialogKaiToast.QueueNotification("Failed to initialize audio device", "Check your audiosettings");
  m_timePerPacket = 1.0f / (float)(iChannels*(uiBitsPerSample/8) * uiSamplesPerSec);
  m_packetsSent = 0;
  m_paused = 0;
  m_lastUpdate = timeGetTime();
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
  m_lastUpdate = timeGetTime();
  m_packetsSent = 0;
}

//***********************************************************************************************
HRESULT CNullDirectSound::Pause()
{
  m_paused = true;
  return S_OK;
}

//***********************************************************************************************
HRESULT CNullDirectSound::Resume()
{
  m_paused = false;
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
  Update();

  return (int)BUFFER - m_packetsSent;
}

//***********************************************************************************************
DWORD CNullDirectSound::AddPackets(unsigned char *data, DWORD len)
{
  if (m_paused)
    return 0;
  Update();

  int add = ( len / GetChunkLen() ) * GetChunkLen();
  m_packetsSent += add;

  return add;
}

//***********************************************************************************************
FLOAT CNullDirectSound::GetDelay()
{
  Update();

  return m_timePerPacket * (float)m_packetsSent;
}

//***********************************************************************************************
DWORD CNullDirectSound::GetChunkLen()
{
  return (int)CHUNKLEN;
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
  while(m_packetsSent > 0)
    Update();
}

void CNullDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}

void CNullDirectSound::Update()
{
  long currentTime = timeGetTime();
  long deltaTime = (currentTime - m_lastUpdate);

  if (m_paused)
  {
    m_lastUpdate += deltaTime;
    return;
  }

  double d = (double)deltaTime / 1000.0f;

  if (currentTime != m_lastUpdate)
  {
    double i = (d / (double)m_timePerPacket);
    m_packetsSent -= (long)i;
    if (m_packetsSent < 0)
      m_packetsSent = 0;
    m_lastUpdate = currentTime;
  }
}
