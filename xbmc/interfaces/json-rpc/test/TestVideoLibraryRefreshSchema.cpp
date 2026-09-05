/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

//! \brief The identifiers VideoLibrary.Refresh accepts, one per kind of library item
constexpr std::array<const char*, 6> IDENTIFIERS{
    "movieid", "setid", "tvshowid", "seasonid", "episodeid", "musicvideoid",
};

} // unnamed namespace

//! \brief Drives the shipped schema, so what is validated is what a client reaches
class TestVideoLibraryRefreshSchema : public JSONServiceDescriptionTestBase
{
public:
  void SetUp() override
  {
    JSONServiceDescriptionTestBase::SetUp();

    AddShippedServiceDescription();
  }
};

TEST_F(TestVideoLibraryRefreshSchema, EveryKindOfLibraryItemCanBeNamed)
{
  for (const char* identifier : IDENTIFIERS)
  {
    CVariant output;
    const std::string params{R"({"item": {")" + std::string(identifier) + R"(": 7}})"};

    EXPECT_EQ(OK, Call("VideoLibrary.Refresh", params, output)) << identifier;
    EXPECT_EQ(7, output["item"][identifier].asInteger()) << identifier;
  }
}

/*!
 The four deprecated methods each named one kind, so a caller sent off one of
 them has to find its own kind here or the deprecation strands it.
 */
TEST_F(TestVideoLibraryRefreshSchema, TheDeprecatedMethodsStillTakeWhatTheyAlwaysDid)
{
  CVariant output;
  EXPECT_EQ(OK, Call("VideoLibrary.RefreshMovie", R"({"movieid": 7})", output));
  EXPECT_EQ(OK, Call("VideoLibrary.RefreshTVShow", R"({"tvshowid": 7})", output));
  EXPECT_EQ(OK, Call("VideoLibrary.RefreshEpisode", R"({"episodeid": 7})", output));
  EXPECT_EQ(OK, Call("VideoLibrary.RefreshMusicVideo", R"({"musicvideoid": 7})", output));
}

TEST_F(TestVideoLibraryRefreshSchema, AnItemNamesOneKindAndNamesItProperly)
{
  CVariant output;
  EXPECT_EQ(InvalidParams, Call("VideoLibrary.Refresh", R"({})", output));
  EXPECT_EQ(InvalidParams, Call("VideoLibrary.Refresh", R"({"item": {}})", output));
  EXPECT_EQ(InvalidParams,
            Call("VideoLibrary.Refresh", R"({"item": {"movieid": 7, "tvshowid": 7}})", output));

  // Not a library item this method can refresh, however well formed
  EXPECT_EQ(InvalidParams, Call("VideoLibrary.Refresh", R"({"item": {"albumid": 7}})", output));

  // A library id starts at 1, so the id no row can have is refused rather than looked up
  EXPECT_EQ(InvalidParams, Call("VideoLibrary.Refresh", R"({"item": {"movieid": 0}})", output));
}

TEST_F(TestVideoLibraryRefreshSchema, TheRefreshOptionsCarryOverWithTheirDefaults)
{
  CVariant output;
  ASSERT_EQ(OK, Call("VideoLibrary.Refresh", R"({"item": {"tvshowid": 7}})", output));
  EXPECT_FALSE(output["ignorenfo"].asBoolean());
  EXPECT_FALSE(output["refreshepisodes"].asBoolean());
  EXPECT_EQ("", output["title"].asString());

  ASSERT_EQ(OK, Call("VideoLibrary.Refresh",
                     R"({"item": {"tvshowid": 7}, "ignorenfo": true, "refreshepisodes": true,
                         "title": "Planetes"})",
                     output));
  EXPECT_TRUE(output["ignorenfo"].asBoolean());
  EXPECT_TRUE(output["refreshepisodes"].asBoolean());
  EXPECT_EQ("Planetes", output["title"].asString());
}
