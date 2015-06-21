/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
  ADDONDEPS deps;
  deps.emplace("a", std::make_pair(AddonVersion("1.0.0"), false));
  builder.SetDependencies(deps);
  builder.SetType(ADDON_UNKNOWN);
  builder.SetExtPoint(nullptr);
  auto addon = builder.Build();
  EXPECT_EQ(deps, addon->GetDeps());
}

TEST_F(TestAddonBuilder, ShouldReturnDeivedType)
{
  builder.SetType(ADDON_RESOURCE_LANGUAGE);
  auto addon = std::dynamic_pointer_cast<CLanguageResource>(builder.Build());
  EXPECT_NE(nullptr, addon);
}
