/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"
#include "addons/LanguageResource.h"
#include "addons/addoninfo/AddonInfoBuilder.h"

#include <gtest/gtest.h>

using namespace ADDON;


class TestAddonBuilder : public ::testing::Test
{
protected:
  TestAddonBuilder() = default;
};

TEST_F(TestAddonBuilder, ShouldFailWhenEmpty)
{
  EXPECT_EQ(nullptr, CAddonBuilder::Generate(nullptr, ADDON_UNKNOWN));
}

TEST_F(TestAddonBuilder, ShouldBuildDependencyAddons)
{
  std::vector<DependencyInfo> deps;
  deps.emplace_back("a", AddonVersion("1.0.0"), AddonVersion("1.0.10"), false);

  CAddonInfoBuilder::CFromDB builder;
  builder.SetId("aa");
  builder.SetDependencies(deps);
  builder.SetType(ADDON_UNKNOWN);
  AddonPtr addon = CAddonBuilder::Generate(builder.get(), ADDON_UNKNOWN);
  EXPECT_EQ(deps, addon->GetDependencies());
}

TEST_F(TestAddonBuilder, ShouldReturnDerivedType)
{
  CAddonInfoBuilder::CFromDB builder;
  builder.SetId("aa");
  builder.SetType(ADDON_RESOURCE_LANGUAGE);
  auto addon = std::dynamic_pointer_cast<CLanguageResource>(CAddonBuilder::Generate(builder.get(), ADDON_UNKNOWN));
  EXPECT_NE(nullptr, addon);
}
