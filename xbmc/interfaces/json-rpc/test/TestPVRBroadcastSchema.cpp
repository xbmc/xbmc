/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

std::set<std::string> RequestableFields()
{
  return EnumValues(ShippedType("PVR.Fields.Broadcast")["items"]);
}

std::set<std::string> DeclaredProperties()
{
  return Keys(ShippedType("PVR.Details.Broadcast")["properties"]);
}

} // unnamed namespace

/*!
 A field a caller may ask for that the details type does not declare arrives
 with no documented type or meaning. Half a field addition landing on its own
 says nothing at runtime, in either direction.
 */
TEST(TestPVRBroadcastSchema, EveryRequestableFieldIsDeclared)
{
  const std::set<std::string> properties{DeclaredProperties()};

  for (const std::string& field : RequestableFields())
  {
    EXPECT_TRUE(properties.contains(field)) << "PVR.Fields.Broadcast offers \"" << field
                                            << "\", which PVR.Details.Broadcast does not declare";
  }
}

TEST(TestPVRBroadcastSchema, EveryDeclaredPropertyIsRequestable)
{
  const std::set<std::string> fields{RequestableFields()};

  for (const std::string& property : DeclaredProperties())
  {
    // CFileItemHandler::HandleFileItem answers the identifier itself, so it is
    // not one of the requestable fields
    if (property == "broadcastid")
    {
      continue;
    }

    EXPECT_TRUE(fields.contains(property))
        << "PVR.Details.Broadcast declares \"" << property << "\", which no caller can request";
  }
}

/*!
 A recording is reachable from its broadcast as a library item, so PVR.GetRecordingDetails
 can follow it.
 */
TEST(TestPVRBroadcastSchema, TheRecordingIsAddressableByItsIdentifier)
{
  EXPECT_TRUE(RequestableFields().contains("recordingid"));
  EXPECT_TRUE(DeclaredProperties().contains("recordingid"));
}
