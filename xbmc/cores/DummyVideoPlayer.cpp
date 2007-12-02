
#include "../stdafx.h"
#include "DummyVideoPlayer.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"
#include "../Application.h"

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
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown on open");
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
  m_lastTime = timeGetTime();

  m_callback.OnPlayBackStarted();
  while (!m_bStop)
  {
    if (!m_paused)
      m_clock += (timeGetTime() - m_lastTime)*m_speed;
    m_lastTime = timeGetTime();
    Sleep(0);
    g_graphicsContext.Lock();
    if (g_graphicsContext.IsFullScreenVideo())
    {
      g_graphicsContext.Get3DDevice()->BeginScene();
      g_graphicsContext.Clear();
      g_graphicsContext.SetScalingResolution(g_graphicsContext.GetVideoResolution(), 0, 0, false);
      Render();
      if (g_application.NeedRenderFullScreen())
        g_application.RenderFullScreen();
      g_graphicsContext.Get3DDevice()->EndScene();
    }
    g_graphicsContext.Unlock();
  }
  if (m_bStop)
    m_callback.OnPlayBackEnded();
}

void CDummyVideoPlayer::Pause()
{
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
  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_videoTimeSeekForwardBig)
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

void CDummyVideoPlayer::ToggleFrameDrop()
{
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
  m_clock = iTime;
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
  RECT vw = g_graphicsContext.GetViewWindow();
  D3DVIEWPORT8 newviewport;
  D3DVIEWPORT8 oldviewport;
  g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = vw.left;
  newviewport.Y = vw.top;
  newviewport.Width = vw.right - vw.left;
  newviewport.Height = vw.bottom - vw.top;
  g_graphicsContext.SetClipRegion((float)vw.left, (float)vw.top, (float)vw.right - vw.left, (float)vw.bottom - vw.top);
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
    float posX = (vw.left + vw.right) * 0.5f;
    float posY = (vw.top + vw.bottom) * 0.5f;
    CGUITextLayout::DrawText(font, posX, posY, 0xffffffff, 0, currentTime, XBFONT_CENTER_X | XBFONT_CENTER_Y);
  }
  g_graphicsContext.RestoreClipRegion();
}