/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "messaging/ApplicationMessenger.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::MESSAGING;

TEST(TestApplicationMessenger, PostCallbackExecutesOnceAndReleasesCaptures)
{
  CApplicationMessenger messenger;
  auto value = std::make_shared<int>(10);
  std::weak_ptr<int> lifetime = value;
  int result = 0;
  messenger.PostCallback([value, &result] { result += *value; });
  value.reset();

  EXPECT_EQ(result, 0);
  EXPECT_FALSE(lifetime.expired());
  messenger.ProcessMessages();
  EXPECT_EQ(result, 10);
  EXPECT_TRUE(lifetime.expired());
  messenger.ProcessMessages();
  EXPECT_EQ(result, 10);
}

TEST(TestApplicationMessenger, PostCallbackAfterStopReleasesCaptures)
{
  CApplicationMessenger messenger;
  messenger.Stop();
  auto called = std::make_shared<bool>(false);
  std::weak_ptr<bool> lifetime = called;
  messenger.PostCallback([called] { *called = true; });

  EXPECT_FALSE(*called);
  called.reset();
  EXPECT_TRUE(lifetime.expired());
  messenger.ProcessMessages();
}

TEST(TestApplicationMessenger, CleanupDiscardsCallbackAndReleasesCaptures)
{
  CApplicationMessenger messenger;
  auto called = std::make_shared<bool>(false);
  std::weak_ptr<bool> lifetime = called;
  messenger.PostCallback([called] { *called = true; });

  messenger.Stop();
  messenger.Cleanup();
  EXPECT_FALSE(*called);
  called.reset();
  EXPECT_TRUE(lifetime.expired());
  messenger.ProcessMessages();
}

TEST(TestApplicationMessenger, DestructionReleasesUnprocessedCallback)
{
  auto called = std::make_shared<bool>(false);
  std::weak_ptr<bool> lifetime = called;
  {
    CApplicationMessenger messenger;
    messenger.PostCallback([called] { *called = true; });
  }
  EXPECT_FALSE(*called);
  called.reset();
  EXPECT_TRUE(lifetime.expired());
}

TEST(TestApplicationMessenger, EmptyCallbackIsIgnored)
{
  CApplicationMessenger messenger;
  messenger.PostCallback({});
  messenger.ProcessMessages();
}

TEST(TestApplicationMessenger, LegacyCallbackStillExecutes)
{
  CApplicationMessenger messenger;
  bool called = false;
  ThreadMessageCallback callback{[](void* user) { *static_cast<bool*>(user) = true; }, &called};
  messenger.PostMsg(TMSG_CALLBACK, -1, -1, &callback);
  EXPECT_FALSE(called);
  messenger.ProcessMessages();
  EXPECT_TRUE(called);
}
