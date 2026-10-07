/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/BlurayDirectory.h"
#include "filesystem/BlurayDiscCache.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
const std::string DISC{"D:\\Movies\\Movie\\"};

void WriteIndex(const std::string& disc, const std::string& content)
{
  CFile file;
  ASSERT_TRUE(file.OpenForWrite(URIUtils::AddFileToFolder(disc, "BDMV", "index.bdmv"), true));
  ASSERT_EQ(file.Write(content.data(), content.size()), static_cast<ssize_t>(content.size()));
}
} // namespace

// A disc in a drive is probed for its name before it is ever listed, so the probe records which
// disc its name came from too. Eg. a removable drive holding a disc folder, replaced by another
// under the same drive letter.
TEST(TestBlurayDiscCache, ProbeDisc_DropsWhatAReplacedDiscCached)
{
  const auto previousCache{CServiceBroker::GetBlurayDiscCache()};
  const auto cache{std::make_shared<CBlurayDiscCache>()};
  CServiceBroker::RegisterBlurayDiscCache(cache);

  const std::string disc{CSpecialProtocol::TranslatePath("special://temp/TestBlurayDiscCache/")};
  ASSERT_TRUE(CDirectory::Create(disc));
  ASSERT_TRUE(CDirectory::Create(URIUtils::AddFileToFolder(disc, "BDMV")));
  WriteIndex(disc, "first disc");

  // Not a real disc, so the probe finds no name - but the disc it probed is recorded
  CBlurayDirectory::ProbeDisc(disc);

  // As a probe of a real disc would have cached it
  cache->SetDiscTitle(disc, "First Disc");
  EXPECT_EQ(CBlurayDirectory::ProbeDisc(disc).name, "First Disc");

  WriteIndex(disc, "a different disc");
  EXPECT_TRUE(CBlurayDirectory::ProbeDisc(disc).name.empty());

  CDirectory::RemoveRecursive(disc);
  if (previousCache)
    CServiceBroker::RegisterBlurayDiscCache(previousCache);
  else
    CServiceBroker::UnregisterBlurayDiscCache();
}

TEST(TestBlurayDiscCache, CheckDisc_KeepsTheSameDisc)
{
  CBlurayDiscCache cache;
  cache.CheckDisc(DISC, "1:100");
  cache.SetDiscTitle(DISC, "Movie");

  cache.CheckDisc(DISC, "1:100");

  std::string title;
  EXPECT_TRUE(cache.GetDiscTitle(DISC, title));
  EXPECT_EQ(title, "Movie");
}

TEST(TestBlurayDiscCache, CheckDisc_DropsAReplacedDisc)
{
  CBlurayDiscCache cache;
  cache.CheckDisc(DISC, "1:100");
  cache.SetDiscTitle(DISC, "Movie");

  cache.CheckDisc(DISC, "2:100");

  std::string title;
  EXPECT_FALSE(cache.GetDiscTitle(DISC, title));

  // The replacement is now the disc held
  cache.SetDiscTitle(DISC, "Other");
  cache.CheckDisc(DISC, "2:100");
  EXPECT_TRUE(cache.GetDiscTitle(DISC, title));
  EXPECT_EQ(title, "Other");
}
