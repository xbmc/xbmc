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

#pragma once

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
  class CRetroPlayerAutoSave;
  class CRetroPlayerInput;
  class CRPProcessInfo;
  class CRPRenderManager;
  class CRPStreamManager;

  class CRetroPlayer : public IPlayer, public IRenderLoop, public IGameCallback
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

  private:
    void SetSpeedInternal(double speed);

    /*!
     * \brief Called when the speed changes
     * \param newSpeed The new speed, possibly equal to the previous speed
     */
    void OnSpeedChange(double newSpeed);

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
    std::unique_ptr<CRetroPlayerAutoSave> m_autoSave;
    GAME::GameClientPtr                m_gameClient;
    CCriticalSection                   m_mutex;
  };
}
}
