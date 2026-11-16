/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"
#include "interfaces/json-rpc/PVREpgFields.h"
#include "utils/Variant.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

TEST(TestPVREpgFields, TranslateEpgCastGivesEveryEntryARoleAndAnOrder)
{
  // Video.Cast requires name, role and order on every entry, so a name on its own does not
  // satisfy the schema the generic list types declare.
  const CVariant cast{TranslateEpgCast("First Actor,Second Actor")};

  ASSERT_TRUE(cast.isArray());
  ASSERT_EQ(2U, cast.size());

  EXPECT_EQ("First Actor", cast[0]["name"].asString());
  EXPECT_TRUE(cast[0].isMember("role"));
  EXPECT_EQ("", cast[0]["role"].asString());
  EXPECT_TRUE(cast[0]["order"].isInteger());
  EXPECT_EQ(0, cast[0]["order"].asInteger());

  EXPECT_EQ("Second Actor", cast[1]["name"].asString());
  EXPECT_TRUE(cast[1].isMember("role"));
  EXPECT_EQ("", cast[1]["role"].asString());
  EXPECT_TRUE(cast[1]["order"].isInteger());
  EXPECT_EQ(1, cast[1]["order"].asInteger());
}

TEST(TestPVREpgFields, TranslateEpgCastKeepsAnOnlyEntryAtOrderZero)
{
  const CVariant cast{TranslateEpgCast("Only Actor")};

  ASSERT_EQ(1U, cast.size());
  EXPECT_EQ("Only Actor", cast[0]["name"].asString());
  EXPECT_EQ("", cast[0]["role"].asString());
  EXPECT_EQ(0, cast[0]["order"].asInteger());
}

TEST(TestPVREpgFields, TranslateEpgCastOfNothingIsAnEmptyArray)
{
  // The field is only asked for when it was requested, so an EPG tag with no cast has to answer
  // with an array rather than a one-entry array holding an empty name.
  const CVariant cast{TranslateEpgCast("")};

  ASSERT_TRUE(cast.isArray());
  EXPECT_EQ(0U, cast.size());
}

TEST(TestPVREpgFields, TranslateEpgCastSkipsEmptyNames)
{
  const CVariant cast{TranslateEpgCast("First Actor,,Second Actor")};

  ASSERT_EQ(2U, cast.size());
  EXPECT_EQ("First Actor", cast[0]["name"].asString());
  EXPECT_EQ(0, cast[0]["order"].asInteger());
  EXPECT_EQ("Second Actor", cast[1]["name"].asString());
  EXPECT_EQ(1, cast[1]["order"].asInteger());
}

namespace
{

std::set<std::string> ShippedEnum(const std::string& type)
{
  return EnumValues(ShippedType(type)["items"]);
}

class TestPVRBroadcastFields : public JSONServiceDescriptionTestBase
{
protected:
  void SetUp() override
  {
    JSONServiceDescriptionTestBase::SetUp();

    // PVR.Fields.Broadcast extends Item.Fields.Base, which has to be registered first
    ASSERT_TRUE(CJSONServiceDescription::AddType(ShippedDefinition("Item.Fields.Base")));
    ASSERT_TRUE(CJSONServiceDescription::AddType(ShippedDefinition("PVR.Fields.Broadcast")));
  }
};

} // unnamed namespace

/*!
 The schema enum and BroadcastFields() must agree exactly; an empty set on either side is a
 lookup failure, not agreement.
 */
TEST_F(TestPVRBroadcastFields, EveryFieldTheSchemaOffersIsAnswered)
{
  const std::set<std::string> offered{ShippedEnum("PVR.Fields.Broadcast")};

  ASSERT_FALSE(offered.empty()) << "the two sides agreeing on nothing is not agreement";
  EXPECT_EQ(offered, BroadcastFields());
}
