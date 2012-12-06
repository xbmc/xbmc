/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
 
#include "threads/SystemClock.h"
#include "system.h"
#include "DummyVideoPlayer.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUIFont.h" // for XBFONT_* defines
#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

CDummyVideoPlayer::CDummyVideoPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread("CDummyVideoPlayer")
{
  m_paused = false;
  m_clock = 0;
  m_lastTime = 0;
  m_speed = 1;
}

CDummyVideoPlayer::~CDummyVideoPlayer()
{
  CloseFile();
}

bool CDummyVideoPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  try
  {
    Create();
    if( options.starttime > 0 )
      SeekTime( (int64_t)(options.starttime * 1000) );
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR,"%s - Exception thrown on open", __FUNCTION__);
    return false;
  }
}

bool CDummyVideoPlayer::CloseFile()
{
  StopThread();
  return true;
}

bool CDummyVideoPlayer::IsPlaying() const
{
  return !m_bStop;
}

void CDummyVideoPlayer::Process()
{
  m_clock = 0;
  m_lastTime = XbmcThreads::SystemClockMillis();

  m_callback.OnPlayBackStarted();
  while (!m_bStop)
  {
    if (!m_paused)
      m_clock += (XbmcThreads::SystemClockMillis() - m_lastTime)*m_speed;
    m_lastTime = XbmcThreads::SystemClockMillis();
    Sleep(0);
    g_graphicsContext.Lock();
    if (g_graphicsContext.IsFullScreenVideo())
    {
#ifdef HAS_DX	
      g_Windowing.Get3DDevice()->BeginScene();
#endif
      g_graphicsContext.Clear();
      g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
      Render();
      g_application.RenderNoPresent();
#ifdef HAS_DX     
      g_Windowing.Get3DDevice()->EndScene();
#endif      
    }
    g_graphicsContext.Unlock();
  }
  if (m_bStop)
    m_callback.OnPlayBackEnded();
}

void CDummyVideoPlayer::Pause()
{
  if (m_paused)
    m_callback.OnPlayBackResumed();
  else
	  m_callback.OnPlayBackPaused();
  m_paused = !m_paused;
}

bool CDummyVideoPlayer::IsPaused() const
{
  return m_paused;
}

bool CDummyVideoPlayer::HasVideo() const
{
  return true;
}

bool CDummyVideoPlayer::HasAudio() const
{
  return true;
}

void CDummyVideoPlayer::SwitchToNextLanguage()
{
}

void CDummyVideoPlayer::ToggleSubtitles()
{
}

bool CDummyVideoPlayer::CanSeek()
{
  return GetTotalTime() > 0;
}

void CDummyVideoPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2000*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    int seek = 0;
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    // do the seek
    SeekTime(GetTime() + seek * 1000);
  }
  else
  {
    float percent = GetPercentage();
    if (bLargeStep)
      percent += bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent += bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;

    if (percent >= 0 && percent <= 100)
    {
      // should be modified to seektime
      SeekPercentage(percent);
    }
  }
}

void CDummyVideoPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  strAudioInfo = "DummyVideoPlayer - nothing to see here";
}

void CDummyVideoPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  strVideoInfo = "DummyVideoPlayer - nothing to see here";
}

void CDummyVideoPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = "DummyVideoPlayer - what are you still looking for?";
}

void CDummyVideoPlayer::SwitchToNextAudioLanguage()
{
}

void CDummyVideoPlayer::SeekPercentage(float iPercent)
{
  int64_t iTime = (int64_t)(GetTotalTime() * iPercent / 100);
  SeekTime(iTime);
}

float CDummyVideoPlayer::GetPercentage()
{
  int64_t iTotalTime = GetTotalTime();

  if (iTotalTime != 0)
  {
    return GetTime() * 100 / (float)iTotalTime;
  }

  return 0.0f;
}

//This is how much audio is delayed to video, we count the oposite in the dvdplayer
void CDummyVideoPlayer::SetAVDelay(float fValue)
{
}

float CDummyVideoPlayer::GetAVDelay()
{
  return 0.0f;
}

void CDummyVideoPlayer::SetSubTitleDelay(float fValue)
{
}

float CDummyVideoPlayer::GetSubTitleDelay()
{
  return 0.0;
}

void CDummyVideoPlayer::SeekTime(int64_t iTime)
{
  int seekOffset = (int)(iTime - m_clock);
  m_clock = iTime;
  m_callback.OnPlayBackSeek((int)iTime, seekOffset);
}

// return the time in milliseconds
int64_t CDummyVideoPlayer::GetTime()
{
  return m_clock;
}

// return length in milliseconds
int64_t CDummyVideoPlayer::GetTotalTime()
{
  return 1000000;
}

void CDummyVideoPlayer::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
  m_callback.OnPlayBackSpeedChanged(iSpeed);
}

void CDummyVideoPlayer::ShowOSD(bool bOnoff)
{
}

CStdString CDummyVideoPlayer::GetPlayerState()
{
  return "";
}

bool CDummyVideoPlayer::SetPlayerState(CStdString state)
{
  return true;
}

void CDummyVideoPlayer::Render()
{
  const CRect vw = g_graphicsContext.GetViewWindow();
#ifdef HAS_DX
  D3DVIEWPORT9 newviewport;
  D3DVIEWPORT9 oldviewport;
  g_Windowing.Get3DDevice()->GetViewport(&oldviewport);
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = (DWORD)vw.x1;
  newviewport.Y = (DWORD)vw.y1;
  newviewport.Width = (DWORD)vw.Width();
  newviewport.Height = (DWORD)vw.Height();
  g_graphicsContext.SetClipRegion(vw.x1, vw.y1, vw.Width(), vw.Height());
#else
  g_graphicsContext.SetViewPort(vw.x1, vw.y1, vw.Width(), vw.Height());
#endif 
  CGUIFont *font = g_fontManager.GetFont("font13");
  if (font)
  {
    // minor issue: The font rendering here is performed in screen coords
    // so shouldn't really be scaled
    int mins = (int)(m_clock / 60000);
    int secs = (int)((m_clock / 1000) % 60);
    int ms = (int)(m_clock % 1000);
    CStdString currentTime;
    currentTime.Format("Video goes here %02i:%02i:%03i", mins, secs, ms);
    float posX = (vw.x1 + vw.x2) * 0.5f;
    float posY = (vw.y1 + vw.y2) * 0.5f;
    CGUITextLayout::DrawText(font, posX, posY, 0xffffffff, 0, currentTime, XBFONT_CENTER_X | XBFONT_CENTER_Y);
  }
#ifdef HAS_DX
  g_graphicsContext.RestoreClipRegion();
#else
  g_graphicsContext.RestoreViewPort();
#endif
}
