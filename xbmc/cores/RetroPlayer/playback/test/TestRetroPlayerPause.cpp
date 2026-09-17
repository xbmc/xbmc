/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlaybackTestEnvironment.h"
#include "cores/DataCacheCore.h"
#include "cores/RetroPlayer/RetroPlayer.h"
#include "cores/RetroPlayer/RetroPlayerInput.h"
#include "cores/RetroPlayer/playback/GameLoop.h"
#include "cores/RetroPlayer/playback/RealtimePlayback.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"

#include <atomic>
#include <chrono>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace KODI::RETRO
{
namespace
{
class CPlayerCallback : public IPlayerCallback
{
public:
  void OnPlayBackEnded() override {}
  void OnPlayBackStarted(const CFileItem&) override {}
  void OnPlayBackStopped() override {}
  void OnPlayBackError() override {}
  void OnQueueNextItem() override {}
  void OnPlayBackPaused() override { ++paused; }
  void OnPlayBackResumed() override { ++resumed; }

  unsigned int paused{0};
  unsigned int resumed{0};
};

class CPlayback : public CRealtimePlayback, private IGameLoopCallback
{
public:
  CPlayback() { m_loop.SetSpeed(1.0); }
  ~CPlayback() override
  {
    allowFrameReturn.Set();
    m_loop.Stop();
  }
  bool CanPause() const override { return true; }
  double GetSpeed() const override { return m_loop.GetSpeed(); }
  void SetSpeed(double speed) override { m_loop.SetSpeed(speed); }
  void PauseAsync() override { m_loop.PauseAsync(); }
  void Start() { m_loop.Start(); }

  CEvent firstFrame{true};
  CEvent nextFrame{true};
  CEvent allowFrameReturn{true};

private:
  void FrameEvent() override
  {
    if (m_frames.fetch_add(1) == 0)
      firstFrame.Set();
    else
      nextFrame.Set();
    allowFrameReturn.Wait();
  }
  void RewindEvent() override {}
  void EndEvent() override {}

  std::atomic<unsigned int> m_frames{0};
  CGameLoop m_loop{this, 60.0};
};
} // namespace

class TestRetroPlayerPause : public testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(CServiceBroker::GetGUI(), nullptr);
    m_player = std::make_unique<CRetroPlayer>(m_callback);
    m_player->m_processInfo = std::make_unique<CProcessInfo>();
    m_player->m_processInfo->SetDataCache(&m_cache);
    m_player->m_renderManager = std::make_unique<CRPRenderManager>(*m_player->m_processInfo);
    m_player->m_streamManager =
        std::make_unique<CRPStreamManager>(*m_player->m_renderManager, *m_player->m_processInfo);
    m_player->m_input = std::make_unique<CRetroPlayerInput>(
        m_environment.Peripherals(), *m_player->m_processInfo, GAME::GameClientPtr{});
    m_player->m_playback = std::make_unique<CPlayback>();
  }

  void TearDown() override
  {
    if (!m_player)
      return;

    m_player->m_input.reset();
    m_player->m_streamManager.reset();
    m_player->m_renderManager.reset();
    // The headless fixture has no render system to restore on CloseFile().
    m_player->m_processInfo.reset();
    m_player.reset();
  }

  IPlayer& Player() { return *m_player; }
  CPlayback& Playback() { return static_cast<CPlayback&>(*m_player->m_playback); }
  double Speed() const { return m_player->m_playback->GetSpeed(); }
  void UseUnpausablePlayback() { m_player->m_playback = std::make_unique<CRealtimePlayback>(); }

  CPlayerCallback m_callback;
  CDataCacheCore m_cache;

private:
  class CProcessInfo : public CRPProcessInfo
  {
  public:
    CProcessInfo() : CRPProcessInfo("test") {}
  };

  CPlaybackTestEnvironment m_environment;
  std::unique_ptr<CRetroPlayer> m_player;
};

TEST_F(TestRetroPlayerPause, SilentPauseUpdatesPlaybackWithoutGUI)
{
  Player().Pause(false);

  EXPECT_DOUBLE_EQ(Speed(), 0.0);
  EXPECT_FLOAT_EQ(m_cache.GetSpeed(), 0.0f);
  EXPECT_EQ(m_callback.paused, 1U);
  EXPECT_EQ(m_callback.resumed, 0U);
}

TEST_F(TestRetroPlayerPause, SilentResumeUpdatesPlaybackWithoutGUI)
{
  Player().Pause(false);
  Player().Pause(false);

  EXPECT_DOUBLE_EQ(Speed(), 1.0);
  EXPECT_FLOAT_EQ(m_cache.GetSpeed(), 1.0f);
  EXPECT_EQ(m_callback.paused, 1U);
  EXPECT_EQ(m_callback.resumed, 1U);
}

TEST_F(TestRetroPlayerPause, SilentPauseRespectsPlaybackCapability)
{
  UseUnpausablePlayback();
  Player().Pause(false);

  EXPECT_DOUBLE_EQ(Speed(), 1.0);
  EXPECT_EQ(m_callback.paused, 0U);
}

TEST_F(TestRetroPlayerPause, SilentPauseStopsAfterTheInFlightFrameAndCanResume)
{
  auto& playback = Playback();
  playback.Start();
  ASSERT_TRUE(playback.firstFrame.Wait(1s));

  Player().Pause(false);
  EXPECT_DOUBLE_EQ(Speed(), 0.0);

  playback.allowFrameReturn.Set();
  EXPECT_FALSE(playback.nextFrame.Wait(100ms));

  Player().Pause(false);
  EXPECT_TRUE(playback.nextFrame.Wait(1s));
}
} // namespace KODI::RETRO
