/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"
#include "addons/Resource.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace ADDON;

namespace
{
std::shared_ptr<CResource> MakeResource(AddonType type)
{
  CAddonInfoBuilderFromDB builder;
  builder.SetId("resource.test");
  CAddonType addonType(type);
  builder.SetExtensions(addonType);

  return std::dynamic_pointer_cast<CResource>(CAddonBuilder::Generate(builder.get(), type));
}
} // namespace

TEST(TestResource, ResolvesTheResourceRootForEveryType)
{
  for (const AddonType type :
       {AddonType::RESOURCE_LANGUAGE, AddonType::RESOURCE_UISOUNDS, AddonType::RESOURCE_IMAGES,
        AddonType::RESOURCE_SKIN, AddonType::RESOURCE_FONT, AddonType::RESOURCE_GAMES})
  {
    const std::shared_ptr<CResource> resource{MakeResource(type)};
    ASSERT_NE(resource, nullptr);

    EXPECT_TRUE(resource->CanResolve(""));
    EXPECT_TRUE(resource->CanResolve("subdirectory/"));
  }
}

TEST(TestResource, ResolvesOnlyTheFilesATypePublishes)
{
  const std::shared_ptr<CResource> language{MakeResource(AddonType::RESOURCE_LANGUAGE)};
  EXPECT_TRUE(language->CanResolve("strings.po"));
  EXPECT_TRUE(language->CanResolve("LANGINFO.XML"));
  EXPECT_FALSE(language->CanResolve("langinfo.txt"));
  EXPECT_FALSE(language->CanResolve("strings.po.bak"));

  const std::shared_ptr<CResource> sounds{MakeResource(AddonType::RESOURCE_UISOUNDS)};
  EXPECT_TRUE(sounds->CanResolve("sounds.xml"));
  EXPECT_TRUE(sounds->CanResolve("click.wav"));
  EXPECT_FALSE(sounds->CanResolve("click.mp3"));

  const std::shared_ptr<CResource> images{MakeResource(AddonType::RESOURCE_IMAGES)};
  EXPECT_TRUE(images->CanResolve("28.png"));
  EXPECT_FALSE(images->CanResolve("28.gif"));

  const std::shared_ptr<CResource> skin{MakeResource(AddonType::RESOURCE_SKIN)};
  EXPECT_TRUE(skin->CanResolve("Home.xml"));
  EXPECT_TRUE(skin->CanResolve("font.ttf"));
  EXPECT_FALSE(skin->CanResolve("script.py"));
}

TEST(TestResource, ResolvesEveryFileForATypeThatPublishesNoSet)
{
  const std::shared_ptr<CResource> font{MakeResource(AddonType::RESOURCE_FONT)};
  EXPECT_TRUE(font->CanResolve("anything.ttf"));
  EXPECT_TRUE(font->CanResolve("anything.at.all"));

  const std::shared_ptr<CResource> game{MakeResource(AddonType::RESOURCE_GAMES)};
  EXPECT_TRUE(game->CanResolve("anything.zip"));
}
