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
      CThread()
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
      SeekTime( (__int64)(options.starttime * 1000) );
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
  m_lastTime = CTimeUtils::GetTimeMS();

  m_callback.OnPlayBackStarted();
  while (!m_bStop)
  {
    if (!m_paused)
      m_clock += (CTimeUtils::GetTimeMS() - m_lastTime)*m_speed;
    m_lastTime = CTimeUtils::GetTimeMS();
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

bool CDummyVideoPlayer::HasVideo()
{
  return true;
}

bool CDummyVideoPlayer::HasAudio()
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
  if (g_advancedSettings.VideoSettings()->CanVideoUseTimeSeeking() && GetTotalTime() > 2*g_advancedSettings.VideoSettings()->TimeSeekForwardBig())
  {
    int seek = 0;
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.VideoSettings()->TimeSeekForwardBig() : g_advancedSettings.VideoSettings()->TimeSeekBackwardBig();
    else
      seek = bPlus ? g_advancedSettings.VideoSettings()->TimeSeekForward() : g_advancedSettings.VideoSettings()->TimeSeekBackward();
    // do the seek
    SeekTime(GetTime() + seek * 1000);
  }
  else
  {
    float percent = GetPercentage();
    if (bLargeStep)
      percent += bPlus ? g_advancedSettings.VideoSettings()->PercentSeekForwardBig() : g_advancedSettings.VideoSettings()->PercentSeekBackwardBig();
    else
      percent += bPlus ? g_advancedSettings.VideoSettings()->PercentSeekForward() : g_advancedSettings.VideoSettings()->PercentSeekBackward();

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
  __int64 iTotalMsec = GetTotalTime() * 1000;
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);
  SeekTime(iTime);
}

float CDummyVideoPlayer::GetPercentage()
{
  __int64 iTotalTime = GetTotalTime() * 1000;

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

void CDummyVideoPlayer::SeekTime(__int64 iTime)
{
  int seekOffset = (int)(iTime - m_clock);
  m_clock = iTime;
  m_callback.OnPlayBackSeek((int)iTime, seekOffset);
}

// return the time in milliseconds
__int64 CDummyVideoPlayer::GetTime()
{
  return m_clock;
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CDummyVideoPlayer::GetTotalTime()
{
  return 1000;
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
