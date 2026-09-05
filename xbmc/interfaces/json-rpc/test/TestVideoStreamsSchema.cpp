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
#include "utils/StreamDetails.h"
#include "utils/Variant.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
std::set<std::string> DeclaredProperties(const std::string& kind)
{
  return Keys(ShippedType("Video.Streams")["properties"][kind]["items"]["properties"]);
}

std::set<std::string> SerializedProperties(const CStreamDetail& detail)
{
  CVariant value(CVariant::VariantTypeObject);
  detail.Serialize(value);
  return Keys(value);
}
} // namespace

TEST(TestVideoStreamsSchema, VideoDeclaresWhatTheSerializerEmits)
{
  EXPECT_EQ(SerializedProperties(CStreamDetailVideo{}), DeclaredProperties("video"));
}

TEST(TestVideoStreamsSchema, AudioDeclaresWhatTheSerializerEmits)
{
  EXPECT_EQ(SerializedProperties(CStreamDetailAudio{}), DeclaredProperties("audio"));
}

TEST(TestVideoStreamsSchema, SubtitleDeclaresWhatTheSerializerEmits)
{
  EXPECT_EQ(SerializedProperties(CStreamDetailSubtitle{}), DeclaredProperties("subtitle"));
}
