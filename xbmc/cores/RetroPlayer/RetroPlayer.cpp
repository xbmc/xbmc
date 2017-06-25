/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayer.h"
#include "RetroPlayerAudio.h"
#include "RetroPlayerVideo.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "games/addons/playback/IGameClientPlayback.h"
#include "games/addons/GameClient.h"
#include "games/tags/GameInfoTag.h"
#include "games/GameUtils.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "FileItem.h"
#include "URL.h"

using namespace KODI;
using namespace GAME;
using namespace RETRO;

CRetroPlayer::CRetroPlayer(IPlayerCallback& callback) :
  IPlayer(callback),
  m_renderManager(m_clock, this),
  m_processInfo(CProcessInfo::CreateInstance())
{
}

CRetroPlayer::~CRetroPlayer()
{
  CloseFile();
}

bool CRetroPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  std::string redactedPath = CURL::GetRedacted(file.GetPath());
  CLog::Log(LOGINFO, "RetroPlayer: Opening: %s", redactedPath.c_str());

  CSingleLock lock(m_mutex);

  if (IsPlaying())
    CloseFile();

  PrintGameInfo(file);

  bool bSuccess = false;

  m_gameClient = CGameUtils::OpenGameClient(file);
  if (m_gameClient)
  {
    if (m_gameClient->Initialize())
    {
      m_audio.reset(new CRetroPlayerAudio(*m_processInfo));
      m_video.reset(new CRetroPlayerVideo(m_renderManager, *m_processInfo));

      if (!file.GetPath().empty())
        bSuccess = m_gameClient->OpenFile(file, m_audio.get(), m_video.get());
      else
        bSuccess = m_gameClient->OpenStandalone(m_audio.get(), m_video.get());

      if (bSuccess)
        CLog::Log(LOGDEBUG, "RetroPlayer: Using game client %s", m_gameClient->ID().c_str());
      else
        CLog::Log(LOGERROR, "RetroPlayer: Failed to open file using %s", m_gameClient->ID().c_str());
    }
    else
      CLog::Log(LOGERROR, "RetroPlayer: Failed to initialize %s", m_gameClient->ID().c_str());
  }
  else
    CLog::Log(LOGERROR, "RetroPlayer: Can't find add-on for game file");

  if (bSuccess)
  {
    if (file.m_lStartOffset == STARTOFFSET_RESUME && file.HasGameInfoTag())
    {
      std::string redactedSavestatePath = CURL::GetRedacted(file.GetGameInfoTag()->GetSavestate());
      CLog::Log(LOGDEBUG, "RetroPlayer: Loading savestate %s", redactedSavestatePath.c_str());

      if (!SetPlayerState(file.GetGameInfoTag()->GetSavestate()))
        CLog::Log(LOGERROR, "RetroPlayer: Failed to load savestate");
    }

    SetSpeed(1);

    m_callback.OnPlayBackStarted();
  }
  else
  {
    m_gameClient.reset();
    m_audio.reset();
    m_video.reset();
  }

  return bSuccess;
}

bool CRetroPlayer::CloseFile(bool reopen /* = false */)
{
  CLog::Log(LOGDEBUG, "RetroPlayer: Closing file");

  CSingleLock lock(m_mutex);

  if (m_gameClient)
  {
    m_gameClient->CloseFile();
    m_gameClient->Unload();
    m_gameClient.reset();
    m_callback.OnPlayBackEnded();
  }

  m_audio.reset();
  m_video.reset();

  return true;
}

bool CRetroPlayer::IsPlaying() const
{
  if (m_gameClient)
    return m_gameClient->IsPlaying();
  return false;
}

bool CRetroPlayer::CanPause()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->CanPause();
  return false;
}

void CRetroPlayer::Pause()
{
  if (!CanPause())
    return;

  if (m_gameClient)
  {
    m_gameClient->GetPlayback()->PauseUnpause();
    m_audio->Enable(m_gameClient->GetPlayback()->GetSpeed() == 1.0);

    if (m_gameClient->GetPlayback()->GetSpeed() != 0.0)
      CloseOSD();
  }
}

bool CRetroPlayer::CanSeek()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->CanSeek();
  return false;
}

void CRetroPlayer::Seek(bool bPlus /* = true */,
                        bool bLargeStep /* = false */,
                        bool bChapterOverride /* = false */)
{
  if (!CanSeek())
    return;

  if (m_gameClient)
  {
    //! @todo
    /*
    if (bPlus)
    {
      if (bLargeStep)
        m_gameClient->GetPlayback()->BigSkipForward();
      else
        m_gameClient->GetPlayback()->SmallSkipForward();
    }
    else
    {
      if (bLargeStep)
        m_gameClient->GetPlayback()->BigSkipBackward();
      else
        m_gameClient->GetPlayback()->SmallSkipBackward();
    }
    */
  }
}

void CRetroPlayer::SeekPercentage(float fPercent /* = 0 */)
{
  if (!CanSeek())
    return;

  if (fPercent < 0.0f  )
    fPercent = 0.0f;
  else if (fPercent > 100.0f)
    fPercent = 100.0f;

  int64_t totalTime = GetTotalTime();
  if (totalTime != 0)
    SeekTime(static_cast<int64_t>(totalTime * fPercent / 100.0f));
}

float CRetroPlayer::GetPercentage()
{
  if (m_gameClient)
  {
    const float timeMs = static_cast<float>(m_gameClient->GetPlayback()->GetTimeMs());
    const float totalMs = static_cast<float>(m_gameClient->GetPlayback()->GetTotalTimeMs());

    if (totalMs != 0.0f)
      return timeMs / totalMs * 100.0f;
  }

  return 0.0f;
}

float CRetroPlayer::GetCachePercentage()
{
  if (m_gameClient)
  {
    const float cacheMs = static_cast<float>(m_gameClient->GetPlayback()->GetCacheTimeMs());
    const float totalMs = static_cast<float>(m_gameClient->GetPlayback()->GetTotalTimeMs());

    if (totalMs != 0.0f)
      return cacheMs / totalMs * 100.0f;
  }
  return 0.0f;
}

void CRetroPlayer::SetMute(bool bOnOff)
{
  if (m_audio)
    m_audio->Enable(!bOnOff);
}

void CRetroPlayer::SeekTime(int64_t iTime /* = 0 */)
{
  if (!CanSeek())
    return;

  if (m_gameClient)
  {
    m_gameClient->GetPlayback()->SeekTimeMs(static_cast<unsigned int>(iTime));
    m_audio->Enable(m_gameClient->GetPlayback()->GetSpeed() == 1.0);

    if (m_gameClient->GetPlayback()->GetSpeed() != 0.0)
      CloseOSD();
  }
}

bool CRetroPlayer::SeekTimeRelative(int64_t iTime)
{
  if (!CanSeek())
    return false;

  SeekTime(GetTime() + iTime);

  return true;
}

int64_t CRetroPlayer::GetTime()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->GetTimeMs();
  return 0;
}

int64_t CRetroPlayer::GetTotalTime()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->GetTotalTimeMs();
  return 0;
}

bool CRetroPlayer::GetStreamDetails(CStreamDetails &details)
{
  //! @todo
  return false;
}

void CRetroPlayer::SetSpeed(float speed)
{
  if (m_gameClient)
  {
    if (m_gameClient->GetPlayback()->GetSpeed() != speed)
    {
      if (speed == 1.0f)
        m_callback.OnPlayBackResumed();
      else if (speed == 0.0f)
        m_callback.OnPlayBackPaused();
    }

    m_gameClient->GetPlayback()->SetSpeed(speed);
    m_audio->Enable(m_gameClient->GetPlayback()->GetSpeed() == 1.0);

    if (m_gameClient->GetPlayback()->GetSpeed() != 0.0)
      CloseOSD();
  }
}

float CRetroPlayer::GetSpeed()
{
  if (m_gameClient)
    return static_cast<float>(m_gameClient->GetPlayback()->GetSpeed());
  return 0;
}

std::string CRetroPlayer::GetPlayerState()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->CreateManualSavestate();
  return "";
}

bool CRetroPlayer::SetPlayerState(const std::string& state)
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->LoadSavestate(state);
  return false;
}

void CRetroPlayer::FrameMove()
{
  m_renderManager.FrameMove();

  if (m_gameClient)
  {
    const bool bFullscreen = (g_windowManager.GetActiveWindowID() == WINDOW_FULLSCREEN_GAME);

    switch (m_state)
    {
    case State::STARTING:
    {
      if (bFullscreen)
        m_state = State::FULLSCREEN;
      break;
    }
    case State::FULLSCREEN:
    {
      if (!bFullscreen)
      {
        m_priorSpeed = m_gameClient->GetPlayback()->GetSpeed();
        m_gameClient->GetPlayback()->SetSpeed(0.0);
        m_state = State::BACKGROUND;
      }
      break;
    }
    case State::BACKGROUND:
    {
      if (bFullscreen)
      {
        if (m_gameClient->GetPlayback()->GetSpeed() == 0.0)
          m_gameClient->GetPlayback()->SetSpeed(m_priorSpeed);
        m_state = State::FULLSCREEN;
      }
      break;
    }
    }
  }
}

bool CRetroPlayer::Supports(EINTERLACEMETHOD method)
{
  return m_processInfo->Supports(method);
}

EINTERLACEMETHOD CRetroPlayer::GetDeinterlacingMethodDefault()
{
  return m_processInfo->GetDeinterlacingMethodDefault();
}

void CRetroPlayer::UpdateClockSync(bool enabled)
{
  m_processInfo->SetRenderClockSync(enabled);
}

void CRetroPlayer::UpdateRenderInfo(CRenderInfo &info)
{
  m_processInfo->UpdateRenderInfo(info);
}

void CRetroPlayer::CloseOSD()
{
  CGUIDialog *pDialog = g_windowManager.GetDialog(WINDOW_DIALOG_VIDEO_OSD);
  if (pDialog)
    pDialog->Close(true, WINDOW_FULLSCREEN_GAME);
}

void CRetroPlayer::PrintGameInfo(const CFileItem &file) const
{
  const CGameInfoTag *tag = file.GetGameInfoTag();
  if (tag)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: ---------------------------------------");
    CLog::Log(LOGDEBUG, "RetroPlayer: Game tag loaded");
    CLog::Log(LOGDEBUG, "RetroPlayer: URL: %s", tag->GetURL().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Title: %s", tag->GetTitle().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Platform: %s", tag->GetPlatform().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Genres: %s", StringUtils::Join(tag->GetGenres(), ", ").c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Developer: %s", tag->GetDeveloper().c_str());
    if (tag->GetYear() > 0)
      CLog::Log(LOGDEBUG, "RetroPlayer: Year: %u", tag->GetYear());
    CLog::Log(LOGDEBUG, "RetroPlayer: Game Code: %s", tag->GetID().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Region: %s", tag->GetRegion().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Publisher: %s", tag->GetPublisher().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Format: %s", tag->GetFormat().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Cartridge type: %s", tag->GetCartridgeType().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Save state: %s", tag->GetSavestate().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Game client: %s", tag->GetGameClient().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: ---------------------------------------");
  }
}
