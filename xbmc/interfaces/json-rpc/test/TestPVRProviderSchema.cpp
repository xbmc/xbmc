/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"
#include "pvr/providers/PVRProvider.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace PVR;
using namespace JSONRPC;

namespace
{

//! \brief The property names a caller may request for the given Fields type
std::set<std::string> RequestableFields(const std::string& fieldsType)
{
  return EnumValues(ShippedType(fieldsType)["items"]);
}

} // unnamed namespace

/*!
 A caller may only ask for members of the Fields enum, so anything Serialize
 writes that the schema does not declare is silently discarded.
 */
TEST(TestPVRProviderSchema, EverySerializedValueIsRequestable)
{
  CPVRProvider typed{1, 2};
  typed.SetName("Example Provider");
  typed.SetType(PVR_PROVIDER_TYPE_IPTV);
  typed.SetIconPath("/logos/example.png");
  typed.SetCountries({"GB", "IE"});
  typed.SetLanguages({"en_GB"});

  // A provider whose type the client did not report takes a different arm of
  // the serializer, so it can write a name the typed one never reaches
  CPVRProvider untyped{3, 2};

  const std::set<std::string> fields{RequestableFields("PVR.Fields.Provider")};

  for (const CPVRProvider* provider : {&typed, &untyped})
  {
    CVariant serialized;
    provider->Serialize(serialized);

    for (auto value = serialized.begin_map(); value != serialized.end_map(); ++value)
    {
      // CPVROperations::FillProviderDetails answers the identifier itself, so
      // it is not one of the requestable fields
      if (value->first == "providerid")
        continue;

      EXPECT_TRUE(fields.contains(value->first))
          << "CPVRProvider::Serialize writes \"" << value->first
          << "\", which no caller can request";
    }
  }
}

/*!
 A provider whose type the client did not report still has to say so under the
 name the schema declares, rather than answering with no provider type at all.
 */
TEST(TestPVRProviderSchema, UnknownTypeIsReportedAsAProviderType)
{
  const CPVRProvider provider{1, 2};

  CVariant serialized;
  provider.Serialize(serialized);

  EXPECT_EQ("unknown", serialized["providertype"].asString());
}
