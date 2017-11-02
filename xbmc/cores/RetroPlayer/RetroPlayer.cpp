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
#include "RetroPlayerAutoSave.h"
#include "RetroPlayerInput.h"
#include "RetroPlayerVideo.h"
#include "addons/AddonManager.h"
#include "cores/DataCacheCore.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/GUIGameRenderManager.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/File.h"
#include "games/addons/playback/IGameClientPlayback.h"
#include "games/addons/savestates/Savestate.h"
#include "games/addons/savestates/SavestateUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientTiming.h" //! @todo
#include "games/dialogs/osd/DialogGameVideoSelect.h"
#include "games/ports/PortManager.h"
#include "games/tags/GameInfoTag.h"
#include "games/GameServices.h"
#include "games/GameUtils.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/Action.h"
#include "input/ActionIDs.h"
#include "settings/MediaSettings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"

using namespace KODI;
using namespace GAME;
using namespace RETRO;

CRetroPlayer::CRetroPlayer(IPlayerCallback& callback) :
  IPlayer(callback),
  m_gameServices(CServiceBroker::GetGameServices())
{
}

CRetroPlayer::~CRetroPlayer()
{
  CloseFile();
}

bool CRetroPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  CFileItem fileCopy(file);

  // When playing a game, set the game client that we'll use to open the game
  // Currently this may prompt the user, the goal is to figure this out silently
  if (!GAME::CGameUtils::FillInGameClient(fileCopy, true))
  {
    CLog::Log(LOGINFO, "RetroPlayer: No compatible game client selected, aborting playback");
    return false;
  }

  // Check if we should open in standalone mode
  const bool bStandalone = fileCopy.GetPath().empty();

  m_processInfo.reset(CRPProcessInfo::CreateInstance());
  if (!m_processInfo)
  {
    CLog::Log(LOGERROR, "Failed to create RetroPlayer - no process info registered");
    return false;
  }

  m_processInfo->SetDataCache(&CServiceBroker::GetDataCacheCore());
  m_processInfo->ResetInfo();

  m_renderManager.reset(new CRPRenderManager(*m_processInfo));

  CSingleLock lock(m_mutex);

  if (IsPlaying())
    CloseFile();

  PrintGameInfo(fileCopy);

  bool bSuccess = false;

  std::string gameClientId = fileCopy.GetGameInfoTag()->GetGameClient();

  ADDON::AddonPtr addon;
  if (gameClientId.empty())
  {
    CLog::Log(LOGERROR, "Can't play game, no game client was passed to RetroPlayer!");
  }
  else if (!CServiceBroker::GetAddonMgr().GetAddon(gameClientId, addon, ADDON::ADDON_GAMEDLL))
  {
    CLog::Log(LOGERROR, "Can't find add-on %s for game file!", gameClientId.c_str());
  }
  else
  {
    m_gameClient = std::static_pointer_cast<CGameClient>(addon);
    if (m_gameClient->Initialize())
    {
      m_audio.reset(new CRetroPlayerAudio(*m_processInfo));
      m_video.reset(new CRetroPlayerVideo(*m_renderManager, *m_processInfo));
      m_input.reset(new CRetroPlayerInput(CServiceBroker::GetPeripherals()));

      if (!bStandalone)
      {
        std::string redactedPath = CURL::GetRedacted(fileCopy.GetPath());
        CLog::Log(LOGINFO, "RetroPlayer: Opening: %s", redactedPath.c_str());
        bSuccess = m_gameClient->OpenFile(fileCopy, m_audio.get(), m_video.get(), m_input.get());
      }
      else
      {
        CLog::Log(LOGINFO, "RetroPlayer: Opening standalone");
        bSuccess = m_gameClient->OpenStandalone(m_audio.get(), m_video.get(), m_input.get());
      }

      if (bSuccess)
        CLog::Log(LOGDEBUG, "RetroPlayer: Using game client %s", gameClientId.c_str());
      else
        CLog::Log(LOGERROR, "RetroPlayer: Failed to open file using %s", gameClientId.c_str());
    }
    else
      CLog::Log(LOGERROR, "RetroPlayer: Failed to initialize %s", gameClientId.c_str());
  }

  if (bSuccess && !bStandalone)
  {
    std::string savestatePath = CSavestateUtils::MakeMetadataPath(fileCopy.GetPath());

    CSavestate save;
    if (save.Deserialize(savestatePath))
    {
      // Check if game client is the same
      if (save.GameClient() != m_gameClient->ID())
      {
        ADDON::AddonPtr addon;
        if (CServiceBroker::GetAddonMgr().GetAddon(save.GameClient(), addon))
        {
          // Warn the user that continuing with a different game client will
          // overwrite the save
          bool dummy;
          if (!CGUIDialogYesNo::ShowAndGetInput(438, StringUtils::Format(g_localizeStrings.Get(35217), addon->Name()), dummy, 222, 35218, 0))
            bSuccess = false;
        }
      }
    }

    if (bSuccess)
    {
      std::string redactedSavestatePath = CURL::GetRedacted(savestatePath);
      CLog::Log(LOGDEBUG, "RetroPlayer: Loading savestate %s", redactedSavestatePath.c_str());

      if (!SetPlayerState(savestatePath))
        CLog::Log(LOGERROR, "RetroPlayer: Failed to load savestate");
    }
  }

  if (bSuccess)
  {
    RegisterWindowCallbacks();
    SetSpeedInternal(1.0);
    m_callback.OnPlayBackStarted(fileCopy);
    if (!bStandalone)
      m_autoSave.reset(new CRetroPlayerAutoSave(*m_gameClient));
    m_processInfo->SetVideoFps(static_cast<float>(m_gameClient->Timing().GetFrameRate()));
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

  m_autoSave.reset();

  CSingleLock lock(m_mutex);

  if (m_gameClient)
  {
    std::string savePath = m_gameClient->GetPlayback()->CreateSavestate();
    if (!savePath.empty())
      CLog::Log(LOGDEBUG, "Saved state to %s", CURL::GetRedacted(savePath).c_str());
    else
      CLog::Log(LOGDEBUG, "Failed to save state at close");

    UnregisterWindowCallbacks();
    m_gameClient->CloseFile();
    m_gameClient->Unload();
    m_gameClient.reset();
    m_callback.OnPlayBackEnded();
  }

  m_audio.reset();
  m_video.reset();

  m_renderManager.reset();
  m_processInfo.reset();

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
    float speed;

    if (m_gameClient->GetPlayback()->GetSpeed() == 0.0)
      speed = 1.0f;
    else
      speed = 0.0f;

    SetSpeed(speed);
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

  uint64_t totalTime = GetTotalTime();
  if (totalTime != 0)
    SeekTime(static_cast<int64_t>(totalTime * fPercent / 100.0f));
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
    OnSpeedChange(m_gameClient->GetPlayback()->GetSpeed());
  }
}

bool CRetroPlayer::SeekTimeRelative(int64_t iTime)
{
  if (!CanSeek())
    return false;

  SeekTime(GetTime() + iTime);

  return true;
}

uint64_t CRetroPlayer::GetTime()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->GetTimeMs();
  return 0;
}

uint64_t CRetroPlayer::GetTotalTime()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->GetTotalTimeMs();
  return 0;
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

    SetSpeedInternal(static_cast<double>(speed));
  }
}

bool CRetroPlayer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PLAYER_RESET:
  {
    if (m_gameClient)
    {
      float speed = static_cast<float>(m_gameClient->GetPlayback()->GetSpeed());

      m_gameClient->GetPlayback()->SetSpeed(0.0);

      m_gameServices.PortManager().HardwareReset();

      // If rewinding or paused, begin playback
      if (speed <= 0.0f)
        speed = 1.0f;

      SetSpeed(speed);
    }
    return true;
  }
  case ACTION_SHOW_OSD:
  {
    if (m_gameClient && m_gameClient->GetPlayback()->GetSpeed() == 0.0)
    {
      CloseOSD();
      return true;
    }
  }
  default:
    break;
  }

  return false;
}

std::string CRetroPlayer::GetPlayerState()
{
  std::string savestatePath;

  if (m_gameClient && m_autoSave)
  {
    savestatePath = m_gameClient->GetPlayback()->CreateSavestate();
    if (savestatePath.empty())
    {
      CLog::Log(LOGDEBUG, "Continuing without saving");
      m_autoSave.reset();
    }
  }
  return savestatePath;
}

bool CRetroPlayer::SetPlayerState(const std::string& state)
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->LoadSavestate(state);
  return false;
}

void CRetroPlayer::FrameMove()
{
  if (m_renderManager)
    m_renderManager->FrameMove();

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

        if (m_priorSpeed != 0.0)
        {
          IPlayerCallback *callback = &m_callback;
          CJobManager::GetInstance().Submit([callback]()
            {
              callback->OnPlayBackPaused();
            }, CJob::PRIORITY_NORMAL);
        }

        SetSpeedInternal(0.0);

        m_state = State::BACKGROUND;
      }
      break;
    }
    case State::BACKGROUND:
    {
      if (bFullscreen)
      {
        if (m_gameClient->GetPlayback()->GetSpeed() == 0.0 && m_priorSpeed != 0.0)
        {
          IPlayerCallback *callback = &m_callback;
          CJobManager::GetInstance().Submit([callback]()
            {
              callback->OnPlayBackResumed();
            }, CJob::PRIORITY_NORMAL);

          SetSpeedInternal(m_priorSpeed);
        }

        m_state = State::FULLSCREEN;
      }
      break;
    }
    }

    m_processInfo->SetPlayTimes(0, GetTime(), 0, GetTotalTime());
  }
}

void CRetroPlayer::Render(bool clear, uint32_t alpha /* = 255 */, bool gui /* = true */)
{
  // Performed by callbacks
}

void CRetroPlayer::FlushRenderer()
{
  if (m_renderManager)
    m_renderManager->Flush();
}

void CRetroPlayer::TriggerUpdateResolution()
{
  if (m_renderManager)
    m_renderManager->TriggerUpdateResolution();
}

bool CRetroPlayer::IsRenderingVideo()
{
  return true;
}

void CRetroPlayer::SetSpeedInternal(double speed)
{
  OnSpeedChange(speed);

  if (speed == 0.0)
    m_gameClient->GetPlayback()->PauseAsync();
  else
    m_gameClient->GetPlayback()->SetSpeed(speed);
}

void CRetroPlayer::OnSpeedChange(double newSpeed)
{
  m_audio->Enable(newSpeed == 1.0);
  m_input->SetSpeed(newSpeed);
  m_renderManager->SetSpeed(newSpeed);
  m_processInfo->SetSpeed(static_cast<float>(newSpeed));
  if (newSpeed != 0.0)
    CloseOSD();
}

void CRetroPlayer::CloseOSD()
{
  g_windowManager.CloseDialogs(true);
}

void CRetroPlayer::RegisterWindowCallbacks()
{
  m_gameServices.GameRenderManager().RegisterPlayer(m_renderManager->GetGUIRenderTargetFactory(),
                                                    m_renderManager.get());
}

void CRetroPlayer::UnregisterWindowCallbacks()
{
  m_gameServices.GameRenderManager().UnregisterPlayer();
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
    CLog::Log(LOGDEBUG, "RetroPlayer: Game client: %s", tag->GetGameClient().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: ---------------------------------------");
  }
}
