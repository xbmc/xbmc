/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/RetroPlayerAutoSave.h"
#include "games/GameSettings.h"

#include <chrono>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::RETRO;
using namespace std::chrono_literals;

namespace
{
class CAutoSaveCallback : public IAutoSaveCallback
{
public:
  bool IsAutoSaveEnabled() const override { return false; }
  void RequestAutosave() override { ADD_FAILURE() << "Autosave is disabled"; }
};
} // namespace

TEST(TestRetroPlayerAutoSave, InitiallyIneligible)
{
  CAutoSaveCallback callback;
  GAME::CGameSettings settings;
  const auto beforeConstruction = std::chrono::steady_clock::now();
  CRetroPlayerAutoSave autosave(callback, settings);

  EXPECT_FALSE(autosave.HasInitialDelayElapsed(beforeConstruction));
}

TEST(TestRetroPlayerAutoSave, EligibleAtInitialIntervalWithoutAutosaveRequests)
{
  CAutoSaveCallback callback;
  GAME::CGameSettings settings;
  const std::chrono::steady_clock::time_point startTime{};
  CRetroPlayerAutoSave autosave(callback, settings, startTime);

  EXPECT_FALSE(autosave.HasInitialDelayElapsed(startTime));
  EXPECT_FALSE(autosave.HasInitialDelayElapsed(startTime + 10s - 1ns));
  EXPECT_TRUE(autosave.HasInitialDelayElapsed(startTime + 10s));
}

TEST(TestRetroPlayerAutoSave, RemainsEligibleAfterInitialInterval)
{
  CAutoSaveCallback callback;
  GAME::CGameSettings settings;
  const std::chrono::steady_clock::time_point startTime{};
  CRetroPlayerAutoSave autosave(callback, settings, startTime);

  EXPECT_TRUE(autosave.HasInitialDelayElapsed(startTime + 10s));
  EXPECT_TRUE(autosave.HasInitialDelayElapsed(startTime + 20s));
  EXPECT_TRUE(autosave.HasInitialDelayElapsed(startTime + 1h));
}

TEST(TestRetroPlayerAutoSave, DelayedStartKeepsOriginalDeadline)
{
  CAutoSaveCallback callback;
  GAME::CGameSettings settings;
  const auto delayedStart = std::chrono::steady_clock::now();
  const auto startTime = delayedStart - 1h;
  CRetroPlayerAutoSave autosave(callback, settings, startTime);

  EXPECT_FALSE(autosave.HasInitialDelayElapsed(startTime + 10s - 1ns));
  EXPECT_TRUE(autosave.HasInitialDelayElapsed(startTime + 10s));
  EXPECT_TRUE(autosave.HasInitialDelayElapsed(delayedStart));
}
