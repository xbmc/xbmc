/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"
#include "utils/Variant.h"

#include <gtest/gtest.h>

using namespace JSONRPC;

//! \brief A property a caller can name has a declared value, and every declared value can be named
TEST(TestGUIPropertySchema, TheNamesAndTheValuesAgree)
{
  EXPECT_EQ(EnumValues(ShippedType("GUI.Property.Name")),
            Keys(ShippedType("GUI.Property.Value")["properties"]));
}

TEST(TestGUIPropertySchema, ReadyIsABoolean)
{
  EXPECT_EQ("boolean", ShippedType("GUI.Property.Value")["properties"]["ready"]["type"].asString());
}
