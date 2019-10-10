/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVREdl.h"

#include "FileItem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "cores/Cut.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "utils/log.h"

namespace PVR
{

std::vector<EDL::Cut> CPVREdl::GetCuts(const CFileItem& item)
{
  std::vector<PVR_EDL_ENTRY> edl;

  if (item.HasPVRRecordingInfoTag())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Reading EDL for recording: %s", item.GetPVRRecordingInfoTag()->m_strTitle.c_str());
    edl = item.GetPVRRecordingInfoTag()->GetEdl();
  }
  else if (item.HasEPGInfoTag())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Reading EDL for EPG tag: %s", item.GetEPGInfoTag()->Title().c_str());
    edl = item.GetEPGInfoTag()->GetEdl();
  }

  std::vector<EDL::Cut> cutlist;
  for (const auto& entry : edl)
  {
    EDL::Cut cut;
    cut.start = entry.start;
    cut.end = entry.end;

    switch (entry.type)
    {
    case PVR_EDL_TYPE_CUT:
      cut.action = EDL::Action::CUT;
      break;
    case PVR_EDL_TYPE_MUTE:
      cut.action = EDL::Action::MUTE;
      break;
    case PVR_EDL_TYPE_SCENE:
      cut.action = EDL::Action::SCENE;
      break;
    case PVR_EDL_TYPE_COMBREAK:
      cut.action = EDL::Action::COMM_BREAK;
      break;
    default:
      CLog::LogF(LOGWARNING, "Ignoring entry of unknown EDL type: %d", entry.type);
      continue;
    }

    cutlist.emplace_back(cut);
  }
  return cutlist;
}

} // namespace PVR
