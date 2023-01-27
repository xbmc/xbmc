/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RetroPlayerAutoSave.h"
#include "cores/IPlayer.h"
#include "cores/RetroPlayer/guibridge/IGameCallback.h"
#include "cores/RetroPlayer/playback/IPlaybackControl.h"
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
class CCheevos;
class CGUIGameMessenger;
class CRetroPlayerInput;
class CRPProcessInfo;
class CRPRenderManager;
class CRPStreamManager;
class IPlayback;

class CRetroPlayer : public IPlayer,
                     public IRenderLoop,
                     public IGameCallback,
                     public IPlaybackCallback,
                     public IAutoSaveCallback
{
public:
  explicit CRetroPlayer(IPlayerCallback& callback);
  ~CRetroPlayer() override;

  // implementation of IPlayer
  bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override;
  bool CanPause() const override;
  void Pause() override;
  bool HasVideo() const override { return true; }
  bool HasAudio() const override { return true; }
  bool HasGame() const override { return true; }
  bool CanSeek() const override;
  void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override;
  void SeekPercentage(float fPercent = 0) override;
  float GetCachePercentage() const override;
  void SetMute(bool bOnOff) override;
  void SeekTime(int64_t iTime = 0) override;
  bool SeekTimeRelative(int64_t iTime) override;
  void SetSpeed(float speed) override;
  bool OnAction(const CAction& action) override;
  std::string GetPlayerState() override;
  bool SetPlayerState(const std::string& state) override;
  void FrameMove() override;
  void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
  bool IsRenderingVideo() const override;
  bool HasGameAgent() const override;

  // Implementation of IGameCallback
  std::string GameClientID() const override;
  std::string GetPlayingGame() const override;
  std::string CreateSavestate(bool autosave) override;
  bool UpdateSavestate(const std::string& savestatePath) override;
  bool LoadSavestate(const std::string& savestatePath) override;
  void FreeSavestateResources(const std::string& savestatePath) override;
  void CloseOSDCallback() override;

  // Implementation of IPlaybackCallback
  void SetPlaybackSpeed(double speed) override;
  void EnableInput(bool bEnable) override;

  // Implementation of IAutoSaveCallback
  bool IsAutoSaveEnabled() const override;
  std::string CreateAutosave() override;

private:
  void SetSpeedInternal(double speed);

  /*!
   * \brief Called when the speed changes
   * \param newSpeed The new speed, possibly equal to the previous speed
   */
  void OnSpeedChange(double newSpeed);

  // Playback functions
  void CreatePlayback(const std::string& savestatePath);
  void ResetPlayback();

  /*!
   * \brief Opens the OSD
   */
  void OpenOSD();

  /*!
   * \brief Closes the OSD and shows the FullscreenGame window
   */
  void CloseOSD();

  void RegisterWindowCallbacks();
  void UnregisterWindowCallbacks();

  /**
   * \brief Dump game information (if any) to the debug log.
   */
  void PrintGameInfo(const CFileItem& file) const;

  uint64_t GetTime();
  uint64_t GetTotalTime();

  // Construction parameters
  GAME::CGameServices& m_gameServices;

  // Subsystems
  std::unique_ptr<CRPProcessInfo> m_processInfo;
  std::unique_ptr<CGUIGameMessenger> m_guiMessenger;
  std::unique_ptr<CRPRenderManager> m_renderManager;
  std::unique_ptr<CRPStreamManager> m_streamManager;
  std::unique_ptr<CRetroPlayerInput> m_input;
  std::unique_ptr<IPlayback> m_playback;
  std::unique_ptr<IPlaybackControl> m_playbackControl;
  std::unique_ptr<CRetroPlayerAutoSave> m_autoSave;
  std::shared_ptr<CCheevos> m_cheevos;

  // Game parameters
  GAME::GameClientPtr m_gameClient;

  // Synchronization parameters
  CCriticalSection m_mutex;
};
} // namespace RETRO
} // namespace KODI
