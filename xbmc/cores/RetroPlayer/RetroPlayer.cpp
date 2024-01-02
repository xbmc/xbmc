/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayer.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "RetroPlayerAutoSave.h"
#include "RetroPlayerInput.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/DataCacheCore.h"
#include "cores/IPlayerCallback.h"
#include "cores/RetroPlayer/cheevos/Cheevos.h"
#include "cores/RetroPlayer/guibridge/GUIGameMessenger.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guiplayback/GUIPlaybackControl.h"
#include "cores/RetroPlayer/playback/IPlayback.h"
#include "cores/RetroPlayer/playback/RealtimePlayback.h"
#include "cores/RetroPlayer/playback/ReversiblePlayback.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <memory>
#include <mutex>

using namespace KODI;
using namespace GAME;
using namespace RETRO;

CRetroPlayer::CRetroPlayer(IPlayerCallback& callback)
  : IPlayer(callback), m_gameServices(CServiceBroker::GetGameServices())
{
  ResetPlayback();
  CServiceBroker::GetWinSystem()->RegisterRenderLoop(this);
}

CRetroPlayer::~CRetroPlayer()
{
  CServiceBroker::GetWinSystem()->UnregisterRenderLoop(this);
  CloseFile();
}

bool CRetroPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  CFileItem fileCopy(file);

  std::string savestatePath;

  // When playing a game, set the game client that we'll use to open the game.
  // This will prompt the user to select a savestate if there are any.
  // If there are no savestates, or the user wants to create a new savestate
  // it will prompt the user to select a game client
  if (!GAME::CGameUtils::FillInGameClient(fileCopy, savestatePath))
  {
    CLog::Log(LOGINFO,
              "RetroPlayer[PLAYER]: No compatible game client selected, aborting playback");
    return false;
  }

  // Check if we should open in standalone mode
  const bool bStandalone = fileCopy.GetPath().empty();

  m_processInfo = CRPProcessInfo::CreateInstance();
  if (!m_processInfo)
  {
    CLog::Log(LOGERROR, "RetroPlayer[PLAYER]: Failed to create - no process info registered");
    return false;
  }

  m_processInfo->SetDataCache(&CServiceBroker::GetDataCacheCore());
  m_processInfo->ResetInfo();

  m_guiMessenger = std::make_unique<CGUIGameMessenger>(*m_processInfo);
  m_renderManager = std::make_unique<CRPRenderManager>(*m_processInfo);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  if (IsPlaying())
    CloseFile();

  PrintGameInfo(fileCopy);

  bool bSuccess = false;

  std::string gameClientId = fileCopy.GetGameInfoTag()->GetGameClient();

  ADDON::AddonPtr addon;
  if (gameClientId.empty())
  {
    CLog::Log(LOGERROR, "RetroPlayer[PLAYER]: Can't play game, no game client was passed!");
  }
  else if (!CServiceBroker::GetAddonMgr().GetAddon(gameClientId, addon, ADDON::AddonType::GAMEDLL,
                                                   ADDON::OnlyEnabled::CHOICE_YES))
  {
    CLog::Log(LOGERROR, "RetroPlayer[PLAYER]: Can't find add-on {} for game file!", gameClientId);
  }
  else
  {
    m_gameClient = std::static_pointer_cast<CGameClient>(addon);
    if (m_gameClient->Initialize())
    {
      m_streamManager = std::make_unique<CRPStreamManager>(*m_renderManager, *m_processInfo);

      // Initialize input
      m_input = std::make_unique<CRetroPlayerInput>(CServiceBroker::GetPeripherals(),
                                                    *m_processInfo, m_gameClient);
      m_input->StartAgentManager();

      if (!bStandalone)
      {
        std::string redactedPath = CURL::GetRedacted(fileCopy.GetPath());
        CLog::Log(LOGINFO, "RetroPlayer[PLAYER]: Opening: {}", redactedPath);
        bSuccess = m_gameClient->OpenFile(fileCopy, *m_streamManager, m_input.get());
      }
      else
      {
        CLog::Log(LOGINFO, "RetroPlayer[PLAYER]: Opening standalone");
        bSuccess = m_gameClient->OpenStandalone(*m_streamManager, m_input.get());
      }

      if (bSuccess)
        CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Using game client {}", gameClientId);
      else
        CLog::Log(LOGERROR, "RetroPlayer[PLAYER]: Failed to open file using {}", gameClientId);
    }
    else
      CLog::Log(LOGERROR, "RetroPlayer[PLAYER]: Failed to initialize {}", gameClientId);
  }

  if (bSuccess && !bStandalone)
  {
    CSavestateDatabase savestateDb;

    std::unique_ptr<ISavestate> save = CSavestateDatabase::AllocateSavestate();
    if (savestateDb.GetSavestate(savestatePath, *save))
    {
      // Check if game client is the same
      if (save->GameClientID() != m_gameClient->ID())
      {
        ADDON::AddonPtr addon;
        if (CServiceBroker::GetAddonMgr().GetAddon(save->GameClientID(), addon,
                                                   ADDON::OnlyEnabled::CHOICE_YES))
        {
          // Warn the user that continuing with a different game client will
          // overwrite the save
          bool dummy;
          if (!CGUIDialogYesNo::ShowAndGetInput(
                  438, StringUtils::Format(g_localizeStrings.Get(35217), addon->Name()), dummy, 222,
                  35218, 0))
            bSuccess = false;
        }
      }
    }
  }

  if (bSuccess)
  {
    // Switch to fullscreen
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SWITCHTOFULLSCREEN);

    m_cheevos = std::make_shared<CCheevos>(m_gameClient.get(),
                                           m_gameServices.GameSettings().GetRAUsername(),
                                           m_gameServices.GameSettings().GetRAToken());

    m_cheevos->EnableRichPresence();

    // Initialize gameplay
    CreatePlayback(savestatePath);
    RegisterWindowCallbacks();
    m_playbackControl = std::make_unique<CGUIPlaybackControl>(*this);
    m_callback.OnPlayBackStarted(fileCopy);
    m_callback.OnAVStarted(fileCopy);
    if (!bStandalone)
      m_autoSave = std::make_unique<CRetroPlayerAutoSave>(*this, m_gameServices.GameSettings());

    // Set video framerate
    m_processInfo->SetVideoFps(static_cast<float>(m_gameClient->GetFrameRate()));
  }
  else
  {
    m_input.reset();
    m_streamManager.reset();
    if (m_gameClient)
      m_gameClient->Unload();
    m_gameClient.reset();
  }

  return bSuccess;
}

bool CRetroPlayer::CloseFile(bool reopen /* = false */)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Closing file");

  m_autoSave.reset();

  UnregisterWindowCallbacks();

  m_playbackControl.reset();

  std::unique_lock<CCriticalSection> lock(m_mutex);

  if (m_gameClient && m_gameServices.GameSettings().AutosaveEnabled())
  {
    std::string savePath = m_playback->CreateSavestate(true);
    if (!savePath.empty())
      CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Saved state to {}", CURL::GetRedacted(savePath));
    else
      CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Failed to save state at close");
  }

  m_playback.reset();

  if (m_input)
    m_input->StopAgentManager();

  m_cheevos.reset();

  if (m_gameClient)
    m_gameClient->CloseFile();

  m_input.reset();

  m_streamManager.reset();

  if (m_gameClient)
    m_gameClient->Unload();
  m_gameClient.reset();

  m_renderManager.reset();
  if (m_processInfo)
  {
    m_processInfo->ResetInfo();
  }
  m_processInfo.reset();
  CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Playback ended");
  m_callback.OnPlayBackEnded();

  return true;
}

bool CRetroPlayer::IsPlaying() const
{
  if (m_gameClient)
    return m_gameClient->IsPlaying();
  return false;
}

bool CRetroPlayer::CanPause() const
{
  return m_playback->CanPause();
}

void CRetroPlayer::Pause()
{
  if (!CanPause())
    return;

  float speed;

  if (m_playback->GetSpeed() == 0.0)
    speed = 1.0f;
  else
    speed = 0.0f;

  SetSpeed(speed);
}

bool CRetroPlayer::CanSeek() const
{
  return m_playback->CanSeek();
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
        m_playback->BigSkipForward();
      else
        m_playback->SmallSkipForward();
    }
    else
    {
      if (bLargeStep)
        m_playback->BigSkipBackward();
      else
        m_playback->SmallSkipBackward();
    }
    */
  }
}

void CRetroPlayer::SeekPercentage(float fPercent /* = 0 */)
{
  if (!CanSeek())
    return;

  if (fPercent < 0.0f)
    fPercent = 0.0f;
  else if (fPercent > 100.0f)
    fPercent = 100.0f;

  uint64_t totalTime = GetTotalTime();
  if (totalTime != 0)
    SeekTime(static_cast<int64_t>(totalTime * fPercent / 100.0f));
}

float CRetroPlayer::GetCachePercentage() const
{
  const float cacheMs = static_cast<float>(m_playback->GetCacheTimeMs());
  const float totalMs = static_cast<float>(m_playback->GetTotalTimeMs());

  if (totalMs != 0.0f)
    return cacheMs / totalMs * 100.0f;

  return 0.0f;
}

void CRetroPlayer::SetMute(bool bOnOff)
{
  if (m_streamManager)
    m_streamManager->EnableAudio(!bOnOff);
}

void CRetroPlayer::SeekTime(int64_t iTime /* = 0 */)
{
  if (!CanSeek())
    return;

  m_playback->SeekTimeMs(static_cast<unsigned int>(iTime));
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
  return m_playback->GetTimeMs();
}

uint64_t CRetroPlayer::GetTotalTime()
{
  return m_playback->GetTotalTimeMs();
}

void CRetroPlayer::SetSpeed(float speed)
{
  if (m_playback->GetSpeed() != static_cast<double>(speed))
  {
    if (speed == 1.0f)
      m_callback.OnPlayBackResumed();
    else if (speed == 0.0f)
      m_callback.OnPlayBackPaused();

    SetSpeedInternal(static_cast<double>(speed));

    if (speed == 0.0f)
    {
      const int dialogId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog();
      if (dialogId == WINDOW_FULLSCREEN_GAME)
      {
        CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Opening OSD via speed change ({:f})", speed);
        OpenOSD();
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Closing OSD via speed change ({:f})", speed);
      CloseOSD();
    }
  }
}

bool CRetroPlayer::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PLAYER_RESET:
    {
      if (m_gameClient)
      {
        float speed = static_cast<float>(m_playback->GetSpeed());

        m_playback->SetSpeed(0.0);

        CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Sending reset command via ACTION_PLAYER_RESET");
        m_cheevos->ResetRuntime();
        m_gameClient->Input().HardwareReset();

        // If rewinding or paused, begin playback
        if (speed <= 0.0f)
          speed = 1.0f;

        SetSpeed(speed);
      }
      return true;
    }
    case ACTION_SHOW_OSD:
    {
      if (m_gameClient)
      {
        CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Closing OSD via ACTION_SHOW_OSD");
        CloseOSD();
        return true;
      }
      break;
    }
    default:
      break;
  }

  return false;
}

std::string CRetroPlayer::GetPlayerState()
{
  std::string savestatePath;

  if (m_autoSave)
  {
    savestatePath = m_playback->CreateSavestate(true);
    if (savestatePath.empty())
    {
      CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Continuing without saving");
      m_autoSave.reset();
    }
  }
  return savestatePath;
}

bool CRetroPlayer::SetPlayerState(const std::string& state)
{
  return m_playback->LoadSavestate(state);
}

void CRetroPlayer::FrameMove()
{
  if (m_renderManager)
    m_renderManager->FrameMove();

  if (m_playbackControl)
    m_playbackControl->FrameMove();

  if (m_processInfo)
    m_processInfo->SetPlayTimes(0, GetTime(), 0, GetTotalTime());
}

void CRetroPlayer::Render(bool clear, uint32_t alpha /* = 255 */, bool gui /* = true */)
{
  // Performed by callbacks
}

bool CRetroPlayer::IsRenderingVideo() const
{
  return true;
}

bool CRetroPlayer::HasGameAgent() const
{
  if (m_gameClient)
    return m_gameClient->Input().HasAgent();

  return false;
}

std::string CRetroPlayer::GameClientID() const
{
  if (m_gameClient)
    return m_gameClient->ID();

  return "";
}

std::string CRetroPlayer::GetPlayingGame() const
{
  if (m_gameClient)
    return m_gameClient->GetGamePath();

  return "";
}

std::string CRetroPlayer::CreateSavestate(bool autosave)
{
  if (m_playback)
    return m_playback->CreateSavestate(autosave);

  return "";
}

bool CRetroPlayer::UpdateSavestate(const std::string& savestatePath)
{
  if (m_playback)
    return !m_playback->CreateSavestate(false, savestatePath).empty();

  return false;
}

bool CRetroPlayer::LoadSavestate(const std::string& savestatePath)
{
  if (m_playback)
    return m_playback->LoadSavestate(savestatePath);

  return false;
}

void CRetroPlayer::FreeSavestateResources(const std::string& savestatePath)
{
  if (m_renderManager)
    m_renderManager->ClearVideoFrame(savestatePath);
}

void CRetroPlayer::CloseOSDCallback()
{
  CloseOSD();
}

void CRetroPlayer::SetPlaybackSpeed(double speed)
{
  if (m_playback)
  {
    if (m_playback->GetSpeed() != speed)
    {
      if (speed == 1.0)
      {
        IPlayerCallback* callback = &m_callback;
        CServiceBroker::GetJobManager()->Submit([callback]() { callback->OnPlayBackResumed(); },
                                                CJob::PRIORITY_NORMAL);
      }
      else if (speed == 0.0)
      {
        IPlayerCallback* callback = &m_callback;
        CServiceBroker::GetJobManager()->Submit([callback]() { callback->OnPlayBackPaused(); },
                                                CJob::PRIORITY_NORMAL);
      }
    }
  }

  SetSpeedInternal(speed);
}

void CRetroPlayer::EnableInput(bool bEnable)
{
  if (m_input)
    m_input->EnableInput(bEnable);
}

bool CRetroPlayer::IsAutoSaveEnabled() const
{
  return m_playback->GetSpeed() > 0.0;
}

std::string CRetroPlayer::CreateAutosave()
{
  return m_playback->CreateSavestate(true);
}

void CRetroPlayer::SetSpeedInternal(double speed)
{
  OnSpeedChange(speed);

  if (speed == 0.0)
    m_playback->PauseAsync();
  else
    m_playback->SetSpeed(speed);
}

void CRetroPlayer::OnSpeedChange(double newSpeed)
{
  m_streamManager->EnableAudio(newSpeed == 1.0);
  m_input->SetSpeed(newSpeed);
  m_renderManager->SetSpeed(newSpeed);
  m_processInfo->SetSpeed(static_cast<float>(newSpeed));
}

void CRetroPlayer::CreatePlayback(const std::string& savestatePath)
{
  if (m_gameClient->RequiresGameLoop())
  {
    m_playback->Deinitialize();
    m_playback = std::make_unique<CReversiblePlayback>(
        m_gameClient.get(), *m_renderManager, m_cheevos.get(), *m_guiMessenger,
        m_gameClient->GetFrameRate(), m_gameClient->GetSerializeSize());
  }
  else
    ResetPlayback();

  if (!savestatePath.empty())
  {
    const bool bStandalone = m_gameClient->GetGamePath().empty();
    if (!bStandalone)
    {
      CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Loading savestate");

      if (!SetPlayerState(savestatePath))
        CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to load savestate");
    }
  }

  m_playback->Initialize();
}

void CRetroPlayer::ResetPlayback()
{
  // Called from the constructor, m_playback might not be initialized
  if (m_playback)
    m_playback->Deinitialize();

  m_playback = std::make_unique<CRealtimePlayback>();
}

void CRetroPlayer::OpenOSD()
{
  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_GAME_OSD);
}

void CRetroPlayer::CloseOSD()
{
  CServiceBroker::GetGUI()->GetWindowManager().CloseDialogs(true);
}

void CRetroPlayer::RegisterWindowCallbacks()
{
  m_gameServices.GameRenderManager().RegisterPlayer(m_renderManager->GetGUIRenderTargetFactory(),
                                                    m_renderManager.get(), this);
}

void CRetroPlayer::UnregisterWindowCallbacks()
{
  m_gameServices.GameRenderManager().UnregisterPlayer();
}

void CRetroPlayer::PrintGameInfo(const CFileItem& file) const
{
  const CGameInfoTag* tag = file.GetGameInfoTag();
  if (tag)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: ---------------------------------------");
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Game tag loaded");
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: URL: {}", tag->GetURL());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Title: {}", tag->GetTitle());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Platform: {}", tag->GetPlatform());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Genres: {}",
              StringUtils::Join(tag->GetGenres(), ", "));
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Developer: {}", tag->GetDeveloper());
    if (tag->GetYear() > 0)
      CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Year: {}", tag->GetYear());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Game Code: {}", tag->GetID());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Region: {}", tag->GetRegion());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Publisher: {}", tag->GetPublisher());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Format: {}", tag->GetFormat());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Cartridge type: {}", tag->GetCartridgeType());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: Game client: {}", tag->GetGameClient());
    CLog::Log(LOGDEBUG, "RetroPlayer[PLAYER]: ---------------------------------------");
  }
}
