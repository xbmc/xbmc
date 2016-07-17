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
#include "addons/AddonManager.h"
#include "gtest/gtest.h"

using namespace ADDON;


struct TestAddonFactory : public ::testing::Test
{
  cp_plugin_info_t plugin;
  cp_extension_t scriptExtension;

  void SetUp() override
  {
    plugin = {0};
    plugin.identifier = strdup("foo.bar");
    plugin.version = strdup("1.2.3");

    scriptExtension = {0};
    scriptExtension.plugin = &plugin;
    scriptExtension.ext_point_id = strdup("xbmc.python.script");
  }

  void TearDown() override
  {
    free(plugin.identifier);
    free(plugin.version);
    free(scriptExtension.ext_point_id);
  }
};

TEST_F(TestAddonFactory, ShouldFailWhenAddonDoesNotHaveRequestedType)
{
  plugin.extensions = &scriptExtension;
  plugin.num_extensions = 1;

  auto addon = CAddonMgr::Factory(&plugin, ADDON_SERVICE);
  EXPECT_EQ(nullptr, addon);
}

TEST_F(TestAddonFactory, ShouldPickFirstExtenstionWhenNotRequestingSpecificType)
{
  cp_extension_t extensions[2] = {
      {&plugin, (char*)"xbmc.python.script", nullptr, nullptr, nullptr, nullptr},
      {&plugin, (char*)"xbmc.python.service", nullptr, nullptr, nullptr, nullptr},
  };
  plugin.extensions = extensions;
  plugin.num_extensions = 2;

  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(ADDON_SCRIPT, addon->Type());
}

TEST_F(TestAddonFactory, ShouldIgnoreMetadataExtenstion)
{
  cp_extension_t extensions[2] = {
      {&plugin, (char*)"kodi.addon.metadata", nullptr, nullptr, nullptr, nullptr},
      scriptExtension,
  };
  plugin.extensions = extensions;
  plugin.num_extensions = 2;

  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(ADDON_SCRIPT, addon->Type());
}


TEST_F(TestAddonFactory, ShouldReturnDependencyInfoWhenNoExtensions)
{
  cp_plugin_import_t import{(char*)"a.b", (char*)"1.2.3", 0};
  plugin.extensions = nullptr;
  plugin.num_extensions = 0;
  plugin.num_imports = 1;
  plugin.imports = &import;

  ADDONDEPS expected = {{"a.b", {AddonVersion{"1.2.3"}, false}}};
  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  EXPECT_EQ(expected, addon->GetDeps());
}


TEST_F(TestAddonFactory, ShouldAcceptUnversionedDependencies)
{
  cp_plugin_import_t import{(char*)"a.b", nullptr, 0};
  plugin.extensions = nullptr;
  plugin.num_extensions = 0;
  plugin.num_imports = 1;
  plugin.imports = &import;

  ADDONDEPS expected = {{"a.b", {AddonVersion{"0.0.0"}, false}}};
  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  EXPECT_EQ(expected, addon->GetDeps());
}


TEST_F(TestAddonFactory, IconPathShouldBeBuiltFromPluginPath)
{
  plugin.plugin_path = strdup("a/b");
  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  EXPECT_EQ("a/b", addon->Path());
  EXPECT_EQ("a/b/icon.png", addon->Icon());
  free(plugin.plugin_path);
}


TEST_F(TestAddonFactory, AssetsElementShouldOverrideImplicitArt)
{
  cp_cfg_element_t icon{0};
  icon.name = (char*)"icon";
  icon.value = (char*)"foo/bar.jpg";

  cp_cfg_element_t assets{0};
  assets.name = (char*)"assets";
  assets.num_children = 1;
  assets.children = &icon;

  cp_cfg_element_t root{0};
  root.name = (char*)"kodi.addon.metadata";
  root.num_children = 1;
  root.children = &assets;
  assets.parent = &root;
  icon.parent = &assets;

  cp_extension_t metadata = {&plugin, (char*)"kodi.addon.metadata", nullptr, nullptr, nullptr, &root};

  cp_extension_t extensions[1] = {metadata};
  plugin.extensions = extensions;
  plugin.num_extensions = 1;
  plugin.plugin_path = strdup("a/b");

  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  EXPECT_EQ("a/b", addon->Path());
  EXPECT_EQ("a/b/foo/bar.jpg", addon->Icon());
  EXPECT_EQ("", addon->FanArt());
  free(plugin.plugin_path);
}
