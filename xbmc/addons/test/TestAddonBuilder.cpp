/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"
#include "addons/LanguageResource.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"

#include <gtest/gtest.h>

using namespace ADDON;


class TestAddonBuilder : public ::testing::Test
{
protected:
  TestAddonBuilder() = default;
};

TEST_F(TestAddonBuilder, ShouldFailWhenEmpty)
{
  EXPECT_EQ(nullptr, CAddonBuilder::Generate(nullptr, AddonType::ADDON_UNKNOWN));
}

TEST_F(TestAddonBuilder, ShouldBuildDependencyAddons)
{
  std::vector<DependencyInfo> deps;
  deps.emplace_back("a", CAddonVersion("1.0.0"), CAddonVersion("1.0.10"), false);

  CAddonInfoBuilder::CFromDB builder;
  builder.SetId("aa");
  builder.SetDependencies(deps);
  CAddonType addonType(AddonType::ADDON_UNKNOWN);
  builder.SetExtensions(addonType);
  AddonPtr addon = CAddonBuilder::Generate(builder.get(), AddonType::ADDON_UNKNOWN);
  EXPECT_EQ(deps, addon->GetDependencies());
}

TEST_F(TestAddonBuilder, ShouldReturnDerivedType)
{
  CAddonInfoBuilder::CFromDB builder;
  builder.SetId("aa");
  CAddonType addonType(AddonType::ADDON_RESOURCE_LANGUAGE);
  builder.SetExtensions(addonType);
  auto addon = std::dynamic_pointer_cast<CLanguageResource>(
      CAddonBuilder::Generate(builder.get(), AddonType::ADDON_UNKNOWN));
  EXPECT_NE(nullptr, addon);
}
