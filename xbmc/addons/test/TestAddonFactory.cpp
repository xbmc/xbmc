/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

TEST_F(TestAddonFactory, ShouldPickFirstExtensionWhenNotRequestingSpecificType)
{
  cp_extension_t extensions[2] = {
      {&plugin, const_cast<char*>("xbmc.python.script"), nullptr, nullptr, nullptr, nullptr},
      {&plugin, const_cast<char*>("xbmc.python.service"), nullptr, nullptr, nullptr, nullptr},
  };
  plugin.extensions = extensions;
  plugin.num_extensions = 2;

  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(ADDON_SCRIPT, addon->Type());
}

TEST_F(TestAddonFactory, ShouldIgnoreMetadataExtension)
{
  cp_extension_t extensions[2] = {
      {&plugin, const_cast<char*>("kodi.addon.metadata"), nullptr, nullptr, nullptr, nullptr},
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
  cp_plugin_import_t import{const_cast<char*>("a.b"), const_cast<char*>("1.2.3"), 0};
  plugin.extensions = nullptr;
  plugin.num_extensions = 0;
  plugin.num_imports = 1;
  plugin.imports = &import;

  std::vector<DependencyInfo> expected = {{"a.b", AddonVersion{"1.2.3"}, false}};
  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  EXPECT_EQ(expected, addon->GetDependencies());
}


TEST_F(TestAddonFactory, ShouldAcceptUnversionedDependencies)
{
  cp_plugin_import_t import{const_cast<char*>("a.b"), nullptr, 0};
  plugin.extensions = nullptr;
  plugin.num_extensions = 0;
  plugin.num_imports = 1;
  plugin.imports = &import;

  std::vector<DependencyInfo> expected = {{"a.b", AddonVersion{"0.0.0"}, false}};
  auto addon = CAddonMgr::Factory(&plugin, ADDON_UNKNOWN);
  EXPECT_EQ(expected, addon->GetDependencies());
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
  icon.name = const_cast<char*>("icon");
  icon.value = const_cast<char*>("foo/bar.jpg");

  cp_cfg_element_t assets{0};
  assets.name = const_cast<char*>("assets");
  assets.num_children = 1;
  assets.children = &icon;

  cp_cfg_element_t root{0};
  root.name = const_cast<char*>("kodi.addon.metadata");
  root.num_children = 1;
  root.children = &assets;
  assets.parent = &root;
  icon.parent = &assets;

  cp_extension_t metadata = {&plugin, const_cast<char*>("kodi.addon.metadata"), nullptr, nullptr, nullptr, &root};

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
