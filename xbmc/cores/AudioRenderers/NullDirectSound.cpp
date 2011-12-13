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

#include "threads/SystemClock.h"
#include "NullDirectSound.h"
#include "guilib/AudioContext.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "dialogs/GUIDialogKaiToast.h"

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
bool CNullDirectSound::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic, EEncoded encoded)
{
  CLog::Log(LOGERROR,"Creating a Null Audio Renderer, Check your audio settings as this should not happen");
  if (iChannels == 0)
    iChannels = 2;

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, "Failed to initialize audio device", "Check your audiosettings", TOAST_DISPLAY_TIME, false);
  m_timePerPacket = 1.0f / (float)(iChannels*(uiBitsPerSample/8) * uiSamplesPerSec);
  m_packetsSent = 0;
  m_paused = 0;
  m_lastUpdate = XbmcThreads::SystemClockMillis();
  return true;
}

//***********************************************************************************************
CNullDirectSound::~CNullDirectSound()
{
  Deinitialize();
}


//***********************************************************************************************
bool CNullDirectSound::Deinitialize()
{
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  return true;
}

void CNullDirectSound::Flush()
{
  m_lastUpdate = XbmcThreads::SystemClockMillis();
  m_packetsSent = 0;
  Pause();
}

//***********************************************************************************************
bool CNullDirectSound::Pause()
{
  m_paused = true;
  return true;
}

//***********************************************************************************************
bool CNullDirectSound::Resume()
{
  m_paused = false;
  return true;
}

//***********************************************************************************************
bool CNullDirectSound::Stop()
{
  Flush();
  return true;
}

//***********************************************************************************************
long CNullDirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CNullDirectSound::Mute(bool bMute)
{
}

//***********************************************************************************************
bool CNullDirectSound::SetCurrentVolume(long nVolume)
{
  m_nCurrentVolume = nVolume;
  return true;
}


//***********************************************************************************************
unsigned int CNullDirectSound::GetSpace()
{
  Update();

  if(BUFFER > m_packetsSent)
    return (int)BUFFER - m_packetsSent;
  else
    return 0;
}

//***********************************************************************************************
unsigned int CNullDirectSound::AddPackets(const void* data, unsigned int len)
{
  if (m_paused || GetSpace() == 0)
    return 0;

  int add = ( len / GetChunkLen() ) * GetChunkLen();
  m_packetsSent += add;

  return add;
}

//***********************************************************************************************
float CNullDirectSound::GetDelay()
{
  Update();

  return m_timePerPacket * (float)m_packetsSent;
}

float CNullDirectSound::GetCacheTime()
{
  return GetDelay();
}

//***********************************************************************************************
unsigned int CNullDirectSound::GetChunkLen()
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
  unsigned int currentTime = XbmcThreads::SystemClockMillis();
  // because of the if clause below it's possible that m_lastUpdate is larger
  //  than currentTime. We need to handle this.
  long deltaTime = (currentTime - m_lastUpdate); // the diff shouldn't overflow

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
