/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RetroPlayerAutoSave.h"
#include "cores/RetroPlayer/guibridge/IGameCallback.h"
#include "cores/IPlayer.h"
#include "games/GameTypes.h"
#include "guilib/DispResource.h"
#include "threads/CriticalSection.h"

#include <memory>

namespace KODI
{
namespace GAME
{
  class CGameServices;
}

namespace RETRO
{
  class CRetroPlayerInput;
  class CRPProcessInfo;
  class CRPRenderManager;
  class CRPStreamManager;
  class IPlayback;

  class CRetroPlayer : public IPlayer,
                       public IRenderLoop,
                       public IGameCallback,
                       public IAutoSaveCallback
  {
  public:
    explicit CRetroPlayer(IPlayerCallback& callback);
    ~CRetroPlayer() override;

    // implementation of IPlayer
    bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
    bool CloseFile(bool reopen = false) override;
    bool IsPlaying() const override;
    bool CanPause() override;
    void Pause() override;
    bool HasVideo() const override { return true; }
    bool HasAudio() const override { return true; }
    bool HasGame() const override { return true; }
    bool CanSeek() override;
    void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override;
    void SeekPercentage(float fPercent = 0) override;
    float GetCachePercentage() override;
    void SetMute(bool bOnOff) override;
    void SeekTime(int64_t iTime = 0) override;
    bool SeekTimeRelative(int64_t iTime) override;
    void SetSpeed(float speed) override;
    bool OnAction(const CAction &action) override;
    std::string GetPlayerState() override;
    bool SetPlayerState(const std::string& state) override;
    void FrameMove() override;
    void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
    bool IsRenderingVideo() override;

    // Implementation of IGameCallback
    std::string GameClientID() const override;

    // Implementation of IAutoSaveCallback
    bool IsAutoSaveEnabled() const override;
    std::string CreateSavestate() override;

  private:
    void SetSpeedInternal(double speed);

    /*!
     * \brief Called when the speed changes
     * \param newSpeed The new speed, possibly equal to the previous speed
     */
    void OnSpeedChange(double newSpeed);

    // Playback functions
    void CreatePlayback(bool bRestoreState);
    void ResetPlayback();

    /*!
     * \brief Closes the OSD and shows the FullscreenGame window
     */
    void CloseOSD();

    void RegisterWindowCallbacks();
    void UnregisterWindowCallbacks();

    /**
     * \brief Dump game information (if any) to the debug log.
     */
    void PrintGameInfo(const CFileItem &file) const;

    uint64_t GetTime();
    uint64_t GetTotalTime();

    // Construction parameters
    GAME::CGameServices &m_gameServices;

    enum class State
    {
      STARTING,
      FULLSCREEN,
      BACKGROUND,
    };

    State                              m_state = State::STARTING;
    double                             m_priorSpeed = 0.0f; // Speed of gameplay before entering OSD
    std::unique_ptr<CRPProcessInfo>    m_processInfo;
    std::unique_ptr<CRPRenderManager>  m_renderManager;
    std::unique_ptr<CRPStreamManager>  m_streamManager;
    std::unique_ptr<CRetroPlayerInput> m_input;
    std::unique_ptr<IPlayback>         m_playback;
    std::unique_ptr<CRetroPlayerAutoSave> m_autoSave;
    GAME::GameClientPtr                m_gameClient;
    CCriticalSection                   m_mutex;
  };
}
}
