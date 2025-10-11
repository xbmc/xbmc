/*
 *  Copyright (C) 2010-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/InitRetry.h"

#include <gtest/gtest.h>

#include <atomic>

using namespace std::chrono_literals;

TEST(TestInitRetry, ExecutesUntilSuccess)
{
  std::atomic<int> attempts{0};

  auto attempt = [&]() {
    return ++attempts >= 2;
  };

  RetryExecutionResult result = CInitRetry::Execute("test_component_success", attempt, {}, 3, 0ms);

  EXPECT_TRUE(result.succeeded);
  EXPECT_FALSE(result.fallbackInvoked);
  EXPECT_FALSE(result.fallbackSucceeded);
  EXPECT_EQ(2, result.attemptsMade);
  EXPECT_EQ(3, result.maxRetries);

  auto statuses = CInitStatusTracker::GetStatuses();
  auto it = statuses.find("test_component_success");
  ASSERT_NE(it, statuses.end());
  EXPECT_EQ(InitComponentState::Succeeded, it->second.state);
  EXPECT_TRUE(it->second.message.empty());
  EXPECT_EQ(2, it->second.attempts);
  EXPECT_EQ(3, it->second.maxAttempts);
}

TEST(TestInitRetry, InvokesFallbackWhenAttemptsFail)
{
  std::atomic<int> attempts{0};
  std::atomic<bool> fallbackCalled{false};

  auto attempt = [&]() {
    ++attempts;
    return false;
  };

  auto fallback = [&]() {
    fallbackCalled.store(true);
    return true;
  };

  RetryExecutionResult result =
      CInitRetry::Execute("test_component_fallback", attempt, fallback, 2, 0ms);

  EXPECT_FALSE(result.succeeded);
  EXPECT_TRUE(result.fallbackInvoked);
  EXPECT_TRUE(result.fallbackSucceeded);
  EXPECT_EQ(2, result.attemptsMade);
  EXPECT_EQ(2, result.maxRetries);
  EXPECT_EQ(2, attempts.load());
  EXPECT_TRUE(fallbackCalled.load());

  auto statuses = CInitStatusTracker::GetStatuses();
  auto it = statuses.find("test_component_fallback");
  ASSERT_NE(it, statuses.end());
  EXPECT_EQ(InitComponentState::Fallback, it->second.state);
  EXPECT_EQ("fallback active", it->second.message);
  EXPECT_EQ(2, it->second.attempts);
  EXPECT_EQ(2, it->second.maxAttempts);
}
