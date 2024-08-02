/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVREdl.h"

#include "FileItem.h"
#include "cores/EdlEdit.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "utils/log.h"

namespace PVR
{

std::vector<EDL::Edit> CPVREdl::GetEdits(const CFileItem& item)
{
  if (item.HasPVRRecordingInfoTag())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Reading EDL for recording: {}",
                item.GetPVRRecordingInfoTag()->m_strTitle);
    return item.GetPVRRecordingInfoTag()->GetEdl();
  }
  else if (item.HasEPGInfoTag())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Reading EDL for EPG tag: {}", item.GetEPGInfoTag()->Title());
    return item.GetEPGInfoTag()->GetEdl();
  }
  return {};
}

} // namespace PVR
