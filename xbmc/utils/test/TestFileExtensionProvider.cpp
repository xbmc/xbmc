/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "utils/FileExtensionProvider.h"

#include <string>

#include <gtest/gtest.h>

TEST(TestFileExtensionProvider, GettersAnswerEmptyAfterDeinitialization)
{
  // Use local instances to avoid disturbing the global instance for other tests

  // Check non-empty content of list to prove "cold" deinitialization behavior without a doubt
  CFileExtensionProvider dummyProvider;
  dummyProvider.Initialize(CServiceBroker::GetAddonMgr());
  ASSERT_FALSE(dummyProvider.GetCompoundArchiveExtensions().empty())
      << "the test requires a non-empty list (compound archive extensions).";
  dummyProvider.Deinitialize();

  CFileExtensionProvider provider;
  provider.Initialize(CServiceBroker::GetAddonMgr());

  // One list warm, one cold, so both states are covered after deinitialization.
  const std::string warmed = provider.GetVideoExtensions();
  ASSERT_FALSE(warmed.empty()) << "the test requires a non-empty list (video extensions).";

  provider.Deinitialize();

  EXPECT_TRUE(provider.GetCompoundArchiveExtensions().empty()); // "cold" list
  EXPECT_TRUE(provider.GetVideoExtensions().empty());
}
