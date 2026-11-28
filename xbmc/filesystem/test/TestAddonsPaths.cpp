/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "addons/AddonBuilder.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/AddonsPaths.h"
#include "utils/Variant.h"

#include <gtest/gtest.h>

using namespace KODI;
using ADDON::AddonType;
using ADDON::CAddonInfo;
using KODI::MEDIA::MediaSection;

TEST(TestAddonsPaths, EndpointIsTheHostName)
{
  for (const char* path :
       {ADDONS::ALL, ADDONS::USER, ADDONS::DEPENDENCIES, ADDONS::DISABLED, ADDONS::OUTDATED,
        ADDONS::RUNNING, ADDONS::REPOS, ADDONS::SOURCES, ADDONS::SEARCH, ADDONS::RECENTLY_UPDATED,
        ADDONS::DOWNLOADING, ADDONS::MORE, ADDONS::DEFAULT_BINARY_ADDONS_SOURCE})
    EXPECT_EQ(ADDONS::EndpointOf(path), CURL(path).GetHostName()) << path;
}

TEST(TestAddonsPaths, SourcesPathNamesTheContentASectionTakes)
{
  EXPECT_EQ(XFILE::CAddonsDirectory::SourcesPathOf(MediaSection::VIDEO), "addons://sources/video/");
  EXPECT_EQ(XFILE::CAddonsDirectory::SourcesPathOf(MediaSection::MUSIC), "addons://sources/audio/");
  EXPECT_EQ(XFILE::CAddonsDirectory::SourcesPathOf(MediaSection::PICTURES),
            "addons://sources/image/");
  EXPECT_EQ(XFILE::CAddonsDirectory::SourcesPathOf(MediaSection::PROGRAMS),
            "addons://sources/executable/");
  EXPECT_EQ(XFILE::CAddonsDirectory::SourcesPathOf(MediaSection::GAMES), "addons://sources/game/");
  EXPECT_EQ(XFILE::CAddonsDirectory::SourcesPathOf(MediaSection::FILES), "");
}

TEST(TestAddonsPaths, SubContentNamesRoundTrip)
{
  for (const AddonType type : {AddonType::AUDIO, AddonType::IMAGE, AddonType::EXECUTABLE,
                               AddonType::VIDEO, AddonType::GAME})
    EXPECT_EQ(CAddonInfo::TranslateSubContent(CAddonInfo::SubContentNameOf(type)), type);
  EXPECT_EQ(CAddonInfo::SubContentNameOf(AddonType::SKIN), "");
}

TEST(TestAddonsDirectory, AddonLanguageListsTheParsedTags)
{
  const auto itemFor = [](const std::string& language)
  {
    ADDON::CAddonInfoBuilderFromDB builder;
    builder.SetId("plugin.video.test");
    builder.SetExtrainfo({{"language", language}});
    return XFILE::CAddonsDirectory::FileItemFromAddon(
        ADDON::CAddonBuilder::Generate(builder.get(), AddonType::UNKNOWN), "addons://user/");
  };

  EXPECT_EQ(itemFor("en_GB de nolanguage")->GetProperty("Addon.Language").asString(), "en-GB de");
  EXPECT_FALSE(itemFor("nolanguage")->HasProperty("Addon.Language"));
}
