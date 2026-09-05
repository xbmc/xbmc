/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "utils/Variant.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

TEST(TestAudioLibrarySetInfoProviderSchema, IsAWriteLikeItsVideoSibling)
{
  const CVariant method = ShippedMethod("AudioLibrary.SetInfoProvider");
  ASSERT_FALSE(method.isNull());
  EXPECT_EQ("UpdateData", method["permission"].asString());
  EXPECT_EQ("UpdateData", ShippedMethod("VideoLibrary.SetSourceContent")["permission"].asString());
}

TEST(TestAudioLibrarySetInfoProviderSchema, OffersTheThreeScopesOfTheDialog)
{
  const CVariant method = ShippedMethod("AudioLibrary.SetInfoProvider");
  const CVariant* applyTo = Param(method, "applyto");
  ASSERT_NE(nullptr, applyTo);
  ASSERT_TRUE((*applyTo)["required"].asBoolean());

  std::set<std::string> scopes;
  for (unsigned int index = 0; index < (*applyTo)["schema"]["enum"].size(); index++)
    scopes.insert((*applyTo)["schema"]["enum"][index].asString());
  EXPECT_EQ((std::set<std::string>{"item", "view", "default"}), scopes);
}

TEST(TestAudioLibrarySetInfoProviderSchema, ScraperIdIsOptionalSoABindingCanBeCleared)
{
  const CVariant method = ShippedMethod("AudioLibrary.SetInfoProvider");
  const CVariant* scraperId = Param(method, "scraperid");
  ASSERT_NE(nullptr, scraperId);
  EXPECT_FALSE((*scraperId)["required"].asBoolean(false));
  EXPECT_EQ("", (*scraperId)["schema"]["default"].asString());
}
