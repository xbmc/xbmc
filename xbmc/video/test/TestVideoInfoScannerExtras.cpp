/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "language/LangInfo.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "video/VideoInfoScannerExtras.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using KODI::VIDEO::FindExtraOnAnotherDisc;

// The same extra on two discs of a movie is found by its name and length, and one playing all of a
// kind by its length alone, as each disc may code the kind differently
TEST(TestVideoInfoScannerExtras, AnExtraOnAnotherDiscIsFound)
{
  auto& strings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  ASSERT_TRUE(strings.Load(g_langInfo.GetLanguagePath(), "resource.language.en_gb"));

  const auto extra{[](const std::string& title, int seconds)
                   {
                     auto item{std::make_shared<CFileItem>("extra.mkv", false)};
                     item->GetVideoInfoTag()->GetAssetInfo().SetTitle(title);
                     item->GetVideoInfoTag()->SetDuration(seconds);
                     return item;
                   }};
  CFileItemList existing;
  existing.Add(extra("Gag Reel", 300));
  existing.Add(extra("DA (play all)", 1200));

  EXPECT_EQ(existing[0].get(), FindExtraOnAnotherDisc(existing, *extra("Gagreel", 301)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("Gag Reel", 310)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("Making Of", 300)));
  EXPECT_EQ(existing[1].get(), FindExtraOnAnotherDisc(existing, *extra("SE (play all)", 1201)));
  EXPECT_EQ(existing[1].get(), FindExtraOnAnotherDisc(existing, *extra("Play all", 1200)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("SE (play all)", 900)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("Deleted scenes", 1200)));

  strings.Clear();
}
