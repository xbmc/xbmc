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

// The provider outlives its own deinitialization: it stays reachable through the service broker
// until the process ends, and one-argument URIUtils::GetExtension consults the compound archive
// extensions for any path containing a dot. Deinitialization drops the advanced settings that
// the lazily built lists read, so a list not yet built was built against nothing.
//
// A local instance is used rather than the one the service broker holds, so deinitializing it
// cannot disturb another test in this binary.
TEST(TestFileExtensionProvider, GettersAnswerEmptyAfterDeinitializationRatherThanFaulting)
{
  CFileExtensionProvider provider;
  provider.Initialize(CServiceBroker::GetAddonMgr());

  // Build one list and leave another cold, so both states are covered after deinitialization.
  const std::string warmed = provider.GetVideoExtensions();
  ASSERT_FALSE(warmed.empty()) << "the video extensions were not built while initialized, so the "
                                  "warm case below would prove nothing";

  provider.Deinitialize();

  // The cold list is the case that faulted: nothing is cached, so it would be built, and the
  // settings it reads are gone.
  EXPECT_TRUE(provider.GetCompoundArchiveExtensions().empty());

  // Deinitialize releases the warm list too, so it answers the same way rather than handing back
  // a list built against settings that no longer exist.
  EXPECT_TRUE(provider.GetVideoExtensions().empty());
}
