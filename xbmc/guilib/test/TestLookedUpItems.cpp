/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "guilib/guiinfo/GUIInfoHelper.h"

#include <memory>

#include <gtest/gtest.h>

using KODI::GUILIB::GUIINFO::CLookedUpItems;

TEST(TestLookedUpItems, AnItemIsLookedUpOnce)
{
  CLookedUpItems lookedUp;
  const auto item = std::make_shared<CFileItem>("/music/one.flac", false);

  EXPECT_TRUE(lookedUp.NeedsLookUp(1, item));
  lookedUp.Add(1, item);
  EXPECT_FALSE(lookedUp.NeedsLookUp(1, item));
}

TEST(TestLookedUpItems, AReplacedItemIsLookedUpAgain)
{
  CLookedUpItems lookedUp;
  const auto item = std::make_shared<CFileItem>("/music/one.flac", false);
  lookedUp.Add(1, item);

  const auto rebuilt = std::make_shared<CFileItem>("/music/one.flac", false);
  EXPECT_TRUE(lookedUp.NeedsLookUp(1, rebuilt)) << "the entry kept its id, not its details";
}
