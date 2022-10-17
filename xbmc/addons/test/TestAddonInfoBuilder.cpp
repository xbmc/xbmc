/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "utils/XBMCTinyXML.h"

#include <set>

#include <gtest/gtest.h>

using namespace ADDON;

const std::string addonXML = R"xml(
<addon id="metadata.blablabla.org"
       name="The Bla Bla Bla Addon"
       version="1.2.3"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
    <import addon="metadata.common.imdb.com" minversion="2.9.2" version="2.9.2"/>
    <import addon="metadata.common.themoviedb.org" minversion="3.1.0" version="3.1.0"/>
    <import addon="plugin.video.youtube" minversion="4.4.0" version="4.4.10" optional="true"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies"
             language="en"
             library="blablabla.xml"/>
  <extension point="xbmc.python.module"
             library="lib.so"/>
  <extension point="kodi.addon.metadata">
    <summary lang="en">Summary bla bla bla</summary>
    <description lang="en">Description bla bla bla</description>
    <disclaimer lang="en">Disclaimer bla bla bla</disclaimer>
    <platform>all</platform>
    <language>marsian</language>
    <license>GPL v2.0</license>
    <forum>https://forum.kodi.tv</forum>
    <website>https://kodi.tv</website>
    <email>a@a.dummy</email>
    <source>https://github.com/xbmc/xbmc</source>
  </extension>
</addon>
)xml";

class TestAddonInfoBuilder : public ::testing::Test
{
protected:
  TestAddonInfoBuilder() = default;
};

TEST_F(TestAddonInfoBuilder, ShouldFailWhenIdIsNotSet)
{
  AddonInfoPtr addon = CAddonInfoBuilder::Generate("", AddonType::UNKNOWN);
  EXPECT_EQ(nullptr, addon);
}

TEST_F(TestAddonInfoBuilder, TestGenerate_Id_Type)
{
  AddonInfoPtr addon = CAddonInfoBuilder::Generate("foo.baz", AddonType::VISUALIZATION);
  EXPECT_NE(nullptr, addon);
  EXPECT_EQ(addon->ID(), "foo.baz");
  EXPECT_EQ(addon->MainType(), AddonType::VISUALIZATION);
  EXPECT_TRUE(addon->HasType(AddonType::VISUALIZATION));
  EXPECT_FALSE(addon->HasType(AddonType::SCREENSAVER));
}

TEST_F(TestAddonInfoBuilder, TestGenerate_Repo)
{
  CXBMCTinyXML doc;
  EXPECT_TRUE(doc.Parse(addonXML));
  ASSERT_NE(nullptr, doc.RootElement());

  RepositoryDirInfo repo;
  AddonInfoPtr addon = CAddonInfoBuilder::Generate(doc.RootElement(), repo);
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(addon->ID(), "metadata.blablabla.org");

  EXPECT_EQ(addon->MainType(), AddonType::SCRAPER_MOVIES);
  EXPECT_TRUE(addon->HasType(AddonType::SCRAPER_MOVIES));
  EXPECT_EQ(addon->Type(AddonType::SCRAPER_MOVIES)->LibName(), "blablabla.xml");
  EXPECT_EQ(addon->Type(AddonType::SCRAPER_MOVIES)->GetValue("@language").asString(), "en");

  EXPECT_TRUE(addon->HasType(AddonType::SCRIPT_MODULE));
  EXPECT_EQ(addon->Type(AddonType::SCRIPT_MODULE)->LibName(), "lib.so");
  EXPECT_FALSE(addon->HasType(AddonType::SCRAPER_ARTISTS));

  EXPECT_EQ(addon->Name(), "The Bla Bla Bla Addon");
  EXPECT_EQ(addon->Author(), "Team Kodi");
  EXPECT_EQ(addon->Version().asString(), "1.2.3");

  EXPECT_EQ(addon->Summary(), "Summary bla bla bla");
  EXPECT_EQ(addon->Description(), "Description bla bla bla");
  EXPECT_EQ(addon->Disclaimer(), "Disclaimer bla bla bla");
  EXPECT_EQ(addon->License(), "GPL v2.0");
  EXPECT_EQ(addon->Forum(), "https://forum.kodi.tv");
  EXPECT_EQ(addon->Website(), "https://kodi.tv");
  EXPECT_EQ(addon->EMail(), "a@a.dummy");
  EXPECT_EQ(addon->Source(), "https://github.com/xbmc/xbmc");

  const std::vector<DependencyInfo>& dependencies = addon->GetDependencies();
  ASSERT_EQ(dependencies.size(), (long unsigned int)4);
  EXPECT_EQ(dependencies[0].id, "xbmc.metadata");
  EXPECT_EQ(dependencies[0].optional, false);
  EXPECT_EQ(dependencies[0].versionMin.asString(), "2.1.0");
  EXPECT_EQ(dependencies[0].version.asString(), "2.1.0");
  EXPECT_EQ(dependencies[1].id, "metadata.common.imdb.com");
  EXPECT_EQ(dependencies[1].optional, false);
  EXPECT_EQ(dependencies[1].versionMin.asString(), "2.9.2");
  EXPECT_EQ(dependencies[1].version.asString(), "2.9.2");
  EXPECT_EQ(dependencies[2].id, "metadata.common.themoviedb.org");
  EXPECT_EQ(dependencies[2].optional, false);
  EXPECT_EQ(dependencies[2].versionMin.asString(), "3.1.0");
  EXPECT_EQ(dependencies[2].version.asString(), "3.1.0");
  EXPECT_EQ(dependencies[3].id, "plugin.video.youtube");
  EXPECT_EQ(dependencies[3].optional, true);
  EXPECT_EQ(dependencies[3].versionMin.asString(), "4.4.0");
  EXPECT_EQ(dependencies[3].version.asString(), "4.4.10");

  auto info = addon->ExtraInfo().find("language");
  ASSERT_NE(info, addon->ExtraInfo().end());
  EXPECT_EQ(info->second, "marsian");
}

TEST_F(TestAddonInfoBuilder, TestGenerate_DBEntry)
{
  CAddonInfoBuilderFromDB builder;
  builder.SetId("video.blablabla.org");
  builder.SetVersion(CAddonVersion("1.2.3"));
  CAddonType addonType(AddonType::PLUGIN);
  addonType.Insert("provides", "video audio");
  builder.SetExtensions(addonType);
  builder.SetName("The Bla Bla Bla Addon");
  builder.SetAuthor("Team Kodi");
  builder.SetSummary("Summary bla bla bla");
  builder.SetDescription("Description bla bla bla");
  builder.SetDisclaimer("Disclaimer bla bla bla");
  builder.SetLicense("GPL v2.0");
  builder.SetForum("https://forum.kodi.tv");
  builder.SetWebsite("https://kodi.tv");
  builder.SetEMail("a@a.dummy");
  builder.SetSource("https://github.com/xbmc/xbmc");
  InfoMap extrainfo;
  extrainfo["language"] = "marsian";
  builder.SetExtrainfo(extrainfo);

  AddonInfoPtr addon = builder.get();
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(addon->ID(), "video.blablabla.org");

  EXPECT_EQ(addon->MainType(), AddonType::PLUGIN);
  EXPECT_TRUE(addon->HasType(AddonType::PLUGIN));
  EXPECT_TRUE(addon->HasType(AddonType::VIDEO));
  EXPECT_TRUE(addon->HasType(AddonType::AUDIO));
  EXPECT_FALSE(addon->HasType(AddonType::GAME));

  EXPECT_EQ(addon->Name(), "The Bla Bla Bla Addon");
  EXPECT_EQ(addon->Author(), "Team Kodi");
  EXPECT_EQ(addon->Version().asString(), "1.2.3");

  EXPECT_EQ(addon->Summary(), "Summary bla bla bla");
  EXPECT_EQ(addon->Description(), "Description bla bla bla");
  EXPECT_EQ(addon->Disclaimer(), "Disclaimer bla bla bla");
  EXPECT_EQ(addon->License(), "GPL v2.0");
  EXPECT_EQ(addon->Forum(), "https://forum.kodi.tv");
  EXPECT_EQ(addon->Website(), "https://kodi.tv");
  EXPECT_EQ(addon->EMail(), "a@a.dummy");
  EXPECT_EQ(addon->Source(), "https://github.com/xbmc/xbmc");

  auto info = addon->ExtraInfo().find("language");
  ASSERT_NE(info, addon->ExtraInfo().end());
  EXPECT_EQ(info->second, "marsian");
}
