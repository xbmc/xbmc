/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"
#include "addons/LanguageResource.h"
#include "gtest/gtest.h"

using namespace ADDON;


class TestAddonBuilder : public ::testing::Test
{
protected:
  CAddonBuilder builder;

  void SetUp() override
  {
    builder.SetId("foo.bar");
    builder.SetVersion(AddonVersion("1.2.3"));
  }
};

TEST_F(TestAddonBuilder, ShouldFailWhenIdIsNotSet)
{
  CAddonBuilder builder;
  builder.SetId("");
  EXPECT_EQ(nullptr, builder.Build());
}

TEST_F(TestAddonBuilder, ShouldBuildDependencyAddons)
{
  std::vector<DependencyInfo> deps;
  deps.emplace_back("a", AddonVersion("1.0.0"), false);
  builder.SetDependencies(deps);
  builder.SetType(ADDON_UNKNOWN);
  builder.SetExtPoint(nullptr);
  auto addon = builder.Build();
  EXPECT_EQ(deps, addon->GetDependencies());
}

TEST_F(TestAddonBuilder, ShouldReturnDerivedType)
{
  builder.SetType(ADDON_RESOURCE_LANGUAGE);
  auto addon = std::dynamic_pointer_cast<CLanguageResource>(builder.Build());
  EXPECT_NE(nullptr, addon);
}
