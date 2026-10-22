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
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "filesystem/File.h"
#include "interfaces/json-rpc/PlayerOperations.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/Variant.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
class CTestPlayerOperations : public CPlayerOperations
{
public:
  using CPlayerOperations::GetPlaylistPlayerName;
};
} // namespace

class TestPlaylistPlayerName : public testing::Test
{
protected:
  void SetUp() override
  {
    m_factory = std::make_unique<CPlayerCoreFactory>(
        *CServiceBroker::GetSettingsComponent()->GetProfileManager());
    m_factory->OnSettingsLoaded();
    m_items.Add(std::make_shared<CFileItem>("/music/start.wav", false));
    m_items.Add(std::make_shared<CFileItem>("/video/next.mp4", false));
    // The unit environment has no game service, so classify these fixtures with media tags.
    m_items[0]->GetMusicInfoTag();
    m_items[1]->GetVideoInfoTag();
  }

  JSONRPC_STATUS Resolve(const CVariant& option, int position = 0)
  {
    return CTestPlayerOperations::GetPlaylistPlayerName(option, m_items, position, *m_factory,
                                                        m_name);
  }

  CFileItemList m_items;
  std::unique_ptr<CPlayerCoreFactory> m_factory;
  std::string m_name{"previous player"};
};

TEST_F(TestPlaylistPlayerName, ValidatesExpandedStartingItemWithoutChangingTheQueue)
{
  const auto deleteTempFile = [](XFILE::CFile* file) { XBMC_DELETETEMPFILE(file); };
  std::vector<std::unique_ptr<XFILE::CFile, decltype(deleteTempFile)>> files;
  std::string target = "/video/next.mp4";
  for (int depth = 1; depth <= 5; ++depth)
  {
    SCOPED_TRACE(depth);
    files.emplace_back(XBMC_CREATETEMPFILE(".m3u"), deleteTempFile);
    ASSERT_NE(nullptr, files.back());
    const std::string contents = target + "\n";
    ASSERT_EQ(static_cast<ssize_t>(contents.size()),
              files.back()->Write(contents.data(), contents.size()));
    files.back()->Close();
    target = XBMC_TEMPFILEPATH(files.back().get());

    m_items[0]->SetPath(target);
    EXPECT_EQ(InvalidParams, Resolve(CVariant("PAPlayer")));
    EXPECT_TRUE(m_name.empty());
    EXPECT_EQ(OK, Resolve(CVariant("vIdEoPlAyEr")));
    EXPECT_EQ("VideoPlayer", m_name);
    EXPECT_EQ(2, m_items.Size());
    EXPECT_EQ(target, m_items[0]->GetPath());
    EXPECT_FALSE(m_items[0]->HasDynPath());
  }
  files.clear();
  EXPECT_FALSE(XFILE::CFile::Exists(target));
}

TEST_F(TestPlaylistPlayerName, MissingAndNullNamesSelectTheDefault)
{
  const CVariant options(CVariant::VariantTypeObject);
  EXPECT_EQ(OK, Resolve(options["playername"]));
  EXPECT_TRUE(m_name.empty());
  m_name = "VideoPlayer";
  EXPECT_EQ(OK, Resolve(CVariant()));
  EXPECT_TRUE(m_name.empty());
}

TEST_F(TestPlaylistPlayerName, DefaultSentinelIsNotAPlayerName)
{
  EXPECT_EQ(OK, Resolve(CVariant("default")));
  EXPECT_TRUE(m_name.empty());
  EXPECT_EQ(InvalidParams, Resolve(CVariant("DEFAULT")));
}

TEST_F(TestPlaylistPlayerName, RejectsMalformedAndUnknownNames)
{
  EXPECT_EQ(InvalidParams, Resolve(CVariant(42)));
  EXPECT_EQ(InvalidParams, Resolve(CVariant("")));
  EXPECT_EQ(InvalidParams, Resolve(CVariant("MissingPlayer")));
  EXPECT_TRUE(m_name.empty());
}

TEST_F(TestPlaylistPlayerName, CanonicalizesCompatibleNames)
{
  EXPECT_EQ(OK, Resolve(CVariant("vIdEoPlAyEr")));
  EXPECT_EQ("VideoPlayer", m_name);
  EXPECT_EQ(OK, Resolve(CVariant("pApLaYeR")));
  EXPECT_EQ("PAPlayer", m_name);
}

TEST_F(TestPlaylistPlayerName, ValidatesTheRequestedStartingItem)
{
  EXPECT_EQ(OK, Resolve(CVariant("PAPlayer"), 0));
  EXPECT_EQ(InvalidParams, Resolve(CVariant("PAPlayer"), 1));
  EXPECT_TRUE(m_name.empty());
  EXPECT_EQ(OK, Resolve(CVariant("VideoPlayer"), 1));
  EXPECT_EQ("VideoPlayer", m_name);
}

TEST_F(TestPlaylistPlayerName, MatchesPlaybackPositionClamping)
{
  EXPECT_EQ(OK, Resolve(CVariant("PAPlayer"), -1));
  EXPECT_EQ(InvalidParams, Resolve(CVariant("PAPlayer"), 999));
  EXPECT_EQ(OK, Resolve(CVariant("VideoPlayer"), 999));
}

TEST_F(TestPlaylistPlayerName, UsesCurrentOrderInsteadOfOriginalOrder)
{
  m_items.Swap(0, 1);
  EXPECT_EQ(InvalidParams, Resolve(CVariant("PAPlayer"), 0));
  EXPECT_EQ(OK, Resolve(CVariant("PAPlayer"), 1));
}

TEST_F(TestPlaylistPlayerName, EmptyPlaylistsStillValidateExistence)
{
  m_items.Clear();
  EXPECT_EQ(OK, Resolve(CVariant()));
  EXPECT_TRUE(m_name.empty());
  EXPECT_EQ(OK, Resolve(CVariant("default")));
  EXPECT_TRUE(m_name.empty());
  EXPECT_EQ(OK, Resolve(CVariant("vIdEoPlAyEr"), 999));
  EXPECT_EQ("VideoPlayer", m_name);
  EXPECT_EQ(InvalidParams, Resolve(CVariant("MissingPlayer")));
}
