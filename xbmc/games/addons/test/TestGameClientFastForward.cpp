/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/GameClientFastForward.h"

#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
class CPlayer : public CGameClientFastForward::IPlayerSpeed
{
public:
  bool IsPlayingGame() const override { return playing; }
  float GetSpeed() const override { return speed; }
  void SetSpeed(float newSpeed) override { speed = round ? std::round(newSpeed) : newSpeed; }

  bool playing = true;
  bool round = false; // Takes only whole speeds
  float speed = 1.0f;
};

class TestGameClientFastForward : public ::testing::Test
{
protected:
  TestGameClientFastForward()
    : m_player(std::make_shared<CPlayer>()),
      m_fastForward(m_player,
                    [this](std::function<void()> task)
                    {
                      if (!m_canPost)
                        return false;
                      m_tasks.emplace_back(std::move(task));
                      return true;
                    })
  {
  }

  // What the application thread does when it gets to the requests
  void RunTasks()
  {
    std::vector<std::function<void()>> tasks;
    tasks.swap(m_tasks);
    for (const auto& task : tasks)
      task();
  }

  void Request(bool fastForward, double ratio = 0.0)
  {
    m_fastForward.Request(fastForward, ratio, static_cast<double>(m_player->speed));
  }

  double ClientSpeed() const
  {
    return m_fastForward.ClientSpeed(static_cast<double>(m_player->speed));
  }

  std::shared_ptr<CPlayer> m_player;
  CGameClientFastForward m_fastForward;
  std::vector<std::function<void()>> m_tasks;
  bool m_canPost = true;
};
} // namespace

TEST_F(TestGameClientFastForward, FastForwardsAndReturnsToNormalSpeed)
{
  Request(true);
  RunTasks();
  EXPECT_EQ(m_player->speed, 16.0f);

  Request(false);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);
}

TEST_F(TestGameClientFastForward, UsesTheSpeedAskedFor)
{
  Request(true, 4.0);
  RunTasks();
  EXPECT_EQ(m_player->speed, 4.0f);
}

TEST_F(TestGameClientFastForward, UsesTheDefaultForASpeedItCantUse)
{
  for (const double ratio : {0.5, 1.0, 17.0, 1e300, std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()})
  {
    Request(true, ratio);
    RunTasks();
    EXPECT_EQ(m_player->speed, 16.0f) << ratio;

    Request(false);
    RunTasks();
  }
}

TEST_F(TestGameClientFastForward, LeavesTheUsersSpeedAlone)
{
  for (const float speed : {0.0f, 0.5f, 2.0f, -1.0f})
  {
    m_player->speed = speed;

    Request(true);
    RunTasks();
    EXPECT_EQ(m_player->speed, speed);

    Request(false);
    RunTasks();
    EXPECT_EQ(m_player->speed, speed);
  }
}

TEST_F(TestGameClientFastForward, LeavesTheUserAtTheSpeedItWouldHaveSet)
{
  m_player->speed = 16.0f;

  Request(true);
  RunTasks();
  Request(false);
  RunTasks();

  EXPECT_EQ(m_player->speed, 16.0f);
}

TEST_F(TestGameClientFastForward, ReturnsFromTheSpeedThePlayerTook)
{
  m_player->round = true;

  Request(true, 1.4);
  RunTasks();
  ASSERT_EQ(m_player->speed, 1.0f);

  Request(true, 2.6);
  RunTasks();
  ASSERT_EQ(m_player->speed, 3.0f);

  Request(false);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);
}

TEST_F(TestGameClientFastForward, ReturnsFromASpeedAFloatCantHold)
{
  Request(true, 1.1);
  RunTasks();
  ASSERT_EQ(m_player->speed, 1.1f);

  Request(false);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);
}

TEST_F(TestGameClientFastForward, AppliesRequestsInOrder)
{
  Request(true);
  Request(false);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);

  Request(false);
  Request(true);
  RunTasks();
  EXPECT_EQ(m_player->speed, 16.0f);
}

TEST_F(TestGameClientFastForward, ChangesToANewSpeedAskedFor)
{
  Request(true, 4.0);
  RunTasks();
  Request(true, 8.0);
  RunTasks();
  EXPECT_EQ(m_player->speed, 8.0f);

  Request(false);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);
}

TEST_F(TestGameClientFastForward, LetsTheUserEndAFastForward)
{
  Request(true);
  RunTasks();
  m_player->speed = 1.0f;

  // The client asking again doesn't override the user
  Request(true);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);

  // Once it has stopped, it can fast-forward again
  Request(false);
  RunTasks();
  Request(true);
  RunTasks();
  EXPECT_EQ(m_player->speed, 16.0f);
}

TEST_F(TestGameClientFastForward, DropsRequestsWhenTheGameCloses)
{
  Request(true);
  m_fastForward.Reset();
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);
}

TEST_F(TestGameClientFastForward, LeavesTheNextGamesSpeedAlone)
{
  Request(true);
  RunTasks();
  m_fastForward.Reset();

  // The next game runs at the same speed, which the user picked
  Request(false);
  RunTasks();
  EXPECT_EQ(m_player->speed, 16.0f);
}

TEST_F(TestGameClientFastForward, ShowsTheClientTheSpeedItWillGet)
{
  Request(true);
  EXPECT_EQ(ClientSpeed(), 16.0);
  RunTasks();
  EXPECT_EQ(ClientSpeed(), 16.0);

  // A load that ends and another that starts before either is applied
  Request(false);
  EXPECT_EQ(ClientSpeed(), 1.0);
  Request(true);
  EXPECT_EQ(ClientSpeed(), 16.0);
  RunTasks();
  EXPECT_EQ(ClientSpeed(), 16.0);
}

TEST_F(TestGameClientFastForward, ShowsTheUsersSpeedWhenItWontChange)
{
  m_player->speed = 0.0f;

  Request(true);
  EXPECT_EQ(ClientSpeed(), 0.0);
}

TEST_F(TestGameClientFastForward, ShowsThePlayersSpeedOnceTheGameCloses)
{
  Request(true);
  m_fastForward.Reset();
  EXPECT_EQ(ClientSpeed(), 1.0);
}

TEST_F(TestGameClientFastForward, ChangesNothingWhenItCantReachThePlayer)
{
  m_canPost = false;

  Request(true);
  EXPECT_EQ(ClientSpeed(), 1.0);
  EXPECT_EQ(m_player->speed, 1.0f);
}

TEST_F(TestGameClientFastForward, ChangesNothingWithoutAGamePlaying)
{
  m_player->playing = false;

  Request(true);
  RunTasks();
  EXPECT_EQ(m_player->speed, 1.0f);
}
