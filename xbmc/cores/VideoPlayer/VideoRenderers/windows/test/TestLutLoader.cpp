/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/VideoRenderers/windows/LutLoader.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
// a load that waits here gives up after a while, so that a loader which waits for its load fails
// the test instead of hanging it
constexpr auto LOAD_TIMEOUT = 10s;
// long enough for a worker to apply a result it should have dropped, once its load has returned
constexpr auto LATE_APPLY_GRACE = 50ms;

CLutLoader::Lut MakeLut(int size)
{
  CLutLoader::Lut lut;
  lut.size = size;
  lut.cmsToken = size;
  return lut;
}

// asks for the result as the render thread does every frame, until the worker has finished
bool ApplyWhenLoaded(CLutLoader& loader)
{
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (loader.IsLoading() && std::chrono::steady_clock::now() < deadline)
  {
    loader.ApplyFinishedLoad();
    std::this_thread::sleep_for(1ms);
  }
  return !loader.IsLoading();
}
} // namespace

TEST(TestLutLoader, ALoadRunsOnAWorkerAndIsAppliedByTheThreadThatAsksForIt)
{
  const std::thread::id consumer = std::this_thread::get_id();
  std::promise<void> release;
  const std::shared_future<void> released = release.get_future().share();
  // outlives the test, which a load still running when the test gives up does not
  const auto loadedOn = std::make_shared<std::thread::id>();
  std::vector<std::thread::id> appliedOn;
  CLutLoader::Lut applied;

  CLutLoader loader(
      [loadedOn, released](AVColorPrimaries)
      {
        released.wait_for(LOAD_TIMEOUT);
        *loadedOn = std::this_thread::get_id();
        return MakeLut(65);
      },
      [&](const CLutLoader::Lut& lut)
      {
        appliedOn.push_back(std::this_thread::get_id());
        applied = lut;
      });

  loader.Load(AVCOL_PRI_BT709);
  // the load is still running: neither starting it nor asking for its result waits for it
  loader.ApplyFinishedLoad();
  EXPECT_TRUE(loader.IsLoading());
  EXPECT_TRUE(appliedOn.empty());

  release.set_value();
  ASSERT_TRUE(ApplyWhenLoaded(loader));

  EXPECT_NE(consumer, *loadedOn);
  EXPECT_EQ(std::vector<std::thread::id>{consumer}, appliedOn);
  EXPECT_EQ(65, applied.size);
  EXPECT_EQ(65, applied.cmsToken);
}

TEST(TestLutLoader, ALoadReplacedByALaterOneIsNeverApplied)
{
  std::promise<void> release;
  const std::shared_future<void> released = release.get_future().share();
  const auto firstLoaded = std::make_shared<std::promise<void>>();
  std::future<void> isFirstLoaded = firstLoaded->get_future();
  std::vector<int> appliedSizes;

  CLutLoader loader(
      [released, firstLoaded](AVColorPrimaries primaries)
      {
        if (primaries != AVCOL_PRI_BT709)
          return MakeLut(65);

        released.wait_for(LOAD_TIMEOUT);
        firstLoaded->set_value();
        return MakeLut(33);
      },
      [&](const CLutLoader::Lut& lut) { appliedSizes.push_back(lut.size); });

  loader.Load(AVCOL_PRI_BT709);
  loader.Load(AVCOL_PRI_BT2020);
  ASSERT_TRUE(ApplyWhenLoaded(loader));

  release.set_value();
  ASSERT_EQ(std::future_status::ready, isFirstLoaded.wait_for(5s));
  std::this_thread::sleep_for(LATE_APPLY_GRACE);
  loader.ApplyFinishedLoad();

  EXPECT_EQ(std::vector<int>{65}, appliedSizes);
}

TEST(TestLutLoader, ALoadInFlightWhenTheLoaderIsDestroyedIsAbandoned)
{
  std::promise<void> release;
  const std::shared_future<void> released = release.get_future().share();
  const auto loaded = std::make_shared<std::promise<void>>();
  std::future<void> isLoaded = loaded->get_future();
  // outlives the loader, which the renderer the result is applied to does not
  const auto applied = std::make_shared<std::atomic<int>>(0);

  {
    CLutLoader loader(
        [released, loaded](AVColorPrimaries)
        {
          released.wait_for(LOAD_TIMEOUT);
          loaded->set_value();
          return MakeLut(65);
        },
        [applied](const CLutLoader::Lut&) { ++*applied; });

    loader.Load(AVCOL_PRI_BT709);
  }
  // destroying the loader did not wait for the load
  EXPECT_NE(std::future_status::ready, isLoaded.wait_for(0s));

  release.set_value();
  ASSERT_EQ(std::future_status::ready, isLoaded.wait_for(5s));
  std::this_thread::sleep_for(LATE_APPLY_GRACE);
  EXPECT_EQ(0, *applied);
}
