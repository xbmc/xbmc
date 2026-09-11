/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
#include "ServiceDescription.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h"
#include "pvr/recordings/PVRRecording.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace PVR;
using namespace JSONRPC;

namespace
{

std::set<std::string> RequestableFields()
{
  return EnumValues(ShippedType("PVR.Fields.Recording")["items"]);
}

std::set<std::string> DeclaredProperties()
{
  return Keys(ShippedType("PVR.Details.Recording")["properties"]);
}

/*!
 \brief A recording as a client reports one.

 The channel type is stated so that the recording does not have to consult the
 PVR manager for it, which no test has running.
 */
CPVRRecording ClientRecording()
{
  PVR_RECORDING recording{};
  recording.strRecordingId = "0815";
  recording.strTitle = "Heroes";
  recording.strEpisodeName = "Genesis";
  recording.strTitleExtraInfo = "Drama, USA, 2006";
  recording.strChannelName = "Example Channel";
  recording.channelType = PVR_RECORDING_CHANNEL_TYPE_TV;
  recording.iGenreType = EPG_GENRE_USE_STRING;
  recording.strGenreDescription = "Drama";
  recording.iSeriesNumber = 1;
  recording.iEpisodeNumber = 1;
  recording.iEpisodePartNumber = 2;
  return {recording, 1};
}

} // unnamed namespace

TEST(TestPVRRecordingSchema, EveryValueTheRecordingAddsIsRequestable)
{
  // CPVRRecording::Serialize writes the whole CVideoInfoTag surface, whose keys answer to the
  // video library's Fields types; only what the recording adds has to be reachable here.
  CVariant base;
  CVideoInfoTag{}.Serialize(base);

  const CPVRRecording recording{ClientRecording()};

  CVariant serialized;
  recording.Serialize(serialized);

  const std::set<std::string> fields{RequestableFields()};

  for (auto value = serialized.begin_map(); value != serialized.end_map(); ++value)
  {
    if (base.isMember(value->first))
      continue;

    // CPVROperations answers the identifier itself, so it is not one of the
    // requestable fields
    if (value->first == "recordingid")
      continue;

    EXPECT_TRUE(fields.contains(value->first)) << "CPVRRecording::Serialize writes \""
                                               << value->first << "\", which no caller can request";
  }
}

TEST(TestPVRRecordingSchema, EveryRequestableFieldIsDeclared)
{
  const std::set<std::string> declared{DeclaredProperties()};

  for (const std::string& field : RequestableFields())
  {
    EXPECT_TRUE(declared.contains(field)) << "PVR.Fields.Recording offers \"" << field
                                          << "\", which PVR.Details.Recording does not declare";
  }
}
