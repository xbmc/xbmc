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

// The provider stays reachable through the service broker after its own deinitialization, and a
// one-argument URIUtils::GetExtension reaches it for any path containing a dot. Deinitializing
// the provider the service broker holds would disturb every other test in this binary.
TEST(TestFileExtensionProvider, GettersAnswerEmptyAfterDeinitializationRatherThanFaulting)
{
  CFileExtensionProvider provider;
  provider.Initialize(CServiceBroker::GetAddonMgr());

  // A cold list would be built from settings that are gone; a warm one must not be handed back.
  const std::string warmed = provider.GetVideoExtensions();
  ASSERT_FALSE(warmed.empty()) << "the video extensions were not built while initialized";

  provider.Deinitialize();

  EXPECT_TRUE(provider.GetCompoundArchiveExtensions().empty());
  EXPECT_TRUE(provider.GetVideoExtensions().empty());
}

// Guards the test above: an empty answer there must be deinitialization, not an unbuildable list.
TEST(TestFileExtensionProvider, GettersAnswerTheBuiltListWhileInitialized)
{
  CFileExtensionProvider provider;
  provider.Initialize(CServiceBroker::GetAddonMgr());

  EXPECT_FALSE(provider.GetCompoundArchiveExtensions().empty());
  EXPECT_FALSE(provider.GetVideoExtensions().empty());
}
