/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRStreamUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "utils/URIUtils.h"

#include <memory>

namespace PVR::UTILS
{

bool ProvidesStreamForMetaDataExtraction(const CFileItem& item)
{
  if (URIUtils::IsPVRRecording(item.GetPath()))
  {
    std::shared_ptr<const CPVRRecording> recording{item.GetPVRRecordingInfoTag()};
    if (!recording)
      recording = CServiceBroker::GetPVRManager().Recordings()->GetByPath(item.GetPath());
    if (!recording)
      return false;
    if (recording->IsInProgress())
      return false; // We must wait with data extraction until recording has finished.

    const std::shared_ptr<const CPVRClient> client{
        CServiceBroker::GetPVRManager().GetClient(recording->ClientID())};
    if (client)
      return client->GetClientCapabilities().SupportsMultipleRecordedStreams();
  }

  return false;
}

} // namespace PVR::UTILS
