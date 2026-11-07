/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/VideoRenderers/windows/LutLoader.h"

#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using Microsoft::WRL::ComPtr;

TEST(TestLutLoader, TheLutIsLoadedAndAppliedOnTheCallingThread)
{
  const std::thread::id caller = std::this_thread::get_id();
  std::thread::id loadedOn;
  std::thread::id appliedOn;
  int appliedSize = 0;
  std::promise<void> applied;
  std::future<void> isApplied = applied.get_future();

  CLutLoader loader(
      [&](AVColorPrimaries, int& lutSize, ComPtr<ID3D11ShaderResourceView>&)
      {
        loadedOn = std::this_thread::get_id();
        lutSize = 65;
        return true;
      },
      [&](int lutSize, ID3D11ShaderResourceView*)
      {
        appliedOn = std::this_thread::get_id();
        appliedSize = lutSize;
        applied.set_value();
      });

  loader.Load(AVCOL_PRI_BT709);
  const bool appliedOnReturn = isApplied.wait_for(0s) == std::future_status::ready;
  // a load that went to another thread must finish before the loader goes out of scope
  ASSERT_EQ(std::future_status::ready, isApplied.wait_for(5s));

  EXPECT_TRUE(appliedOnReturn);
  EXPECT_EQ(caller, loadedOn);
  EXPECT_EQ(caller, appliedOn);
  EXPECT_EQ(65, appliedSize);
  EXPECT_EQ(65, loader.GetSize());
}

TEST(TestLutLoader, ALutThatFailsToLoadIsAppliedWithSizeZero)
{
  int appliedSize = -1;
  std::promise<void> applied;
  std::future<void> isApplied = applied.get_future();

  CLutLoader loader(
      [](AVColorPrimaries, int& lutSize, ComPtr<ID3D11ShaderResourceView>&)
      {
        lutSize = 65;
        return false;
      },
      [&](int lutSize, ID3D11ShaderResourceView*)
      {
        appliedSize = lutSize;
        applied.set_value();
      });

  loader.Load(AVCOL_PRI_BT2020);
  ASSERT_EQ(std::future_status::ready, isApplied.wait_for(5s));

  EXPECT_EQ(0, appliedSize);
  EXPECT_EQ(0, loader.GetSize());
}
