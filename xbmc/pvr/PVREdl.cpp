/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVREdl.h"

#include "FileItem.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_edl.h"
#include "cores/EdlEdit.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "utils/log.h"

namespace PVR
{

std::vector<EDL::Edit> CPVREdl::GetEdits(const CFileItem& item)
{
  std::vector<PVR_EDL_ENTRY> edl;

  if (item.HasPVRRecordingInfoTag())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Reading EDL for recording: {}",
                item.GetPVRRecordingInfoTag()->m_strTitle);
    edl = item.GetPVRRecordingInfoTag()->GetEdl();
  }
  else if (item.HasEPGInfoTag())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Reading EDL for EPG tag: {}", item.GetEPGInfoTag()->Title());
    edl = item.GetEPGInfoTag()->GetEdl();
  }

  std::vector<EDL::Edit> editlist;
  for (const auto& entry : edl)
  {
    EDL::Edit edit;
    edit.start = entry.start;
    edit.end = entry.end;

    switch (entry.type)
    {
    case PVR_EDL_TYPE_CUT:
      edit.action = EDL::Action::CUT;
      break;
    case PVR_EDL_TYPE_MUTE:
      edit.action = EDL::Action::MUTE;
      break;
    case PVR_EDL_TYPE_SCENE:
      edit.action = EDL::Action::SCENE;
      break;
    case PVR_EDL_TYPE_COMBREAK:
      edit.action = EDL::Action::COMM_BREAK;
      break;
    default:
      CLog::LogF(LOGWARNING, "Ignoring entry of unknown EDL type: {}", entry.type);
      continue;
    }

    editlist.emplace_back(edit);
  }
  return editlist;
}

} // namespace PVR
