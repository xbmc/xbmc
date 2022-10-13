/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"

namespace PVR
{
bool CPVRGUIActionsUtils::OnInfo(const CFileItem& item)
{
  if (item.HasPVRRecordingInfoTag())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().ShowRecordingInfo(item);
  }
  else if (item.HasPVRChannelInfoTag() || item.HasPVRTimerInfoTag())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowEPGInfo(item);
  }
  else if (item.HasEPGSearchFilter())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().EditSavedSearch(item);
  }
  return false;
}

} // namespace PVR
