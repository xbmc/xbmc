/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PvrEdlParser.h"

#include "FileItem.h"
#include "pvr/PVREdl.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace EDL;

bool CPvrEdlParser::CanParse(const CFileItem& item) const
{
  return item.HasPVRRecordingInfoTag() || item.HasEPGInfoTag();
}

CEdlParserResult CPvrEdlParser::Parse(const CFileItem& item, float fps)
{
  CEdlParserResult result;

  const std::vector<Edit> editlist = PVR::CPVREdl::GetEdits(item);
  for (const auto& edit : editlist)
  {
    switch (edit.action)
    {
      case Action::CUT:
      case Action::MUTE:
      case Action::COMM_BREAK:
        CLog::LogF(LOGDEBUG, "Adding break [{} - {}] found in PVR item for: {}.",
                   StringUtils::MillisecondsToTimeString(edit.start),
                   StringUtils::MillisecondsToTimeString(edit.end),
                   CURL::GetRedacted(item.GetDynPath()));
        result.AddEdit(edit, EdlSourceLocation{"PVR"});
        break;

      case Action::SCENE:
        result.AddSceneMarker(edit.end, EdlSourceLocation{"PVR"});
        break;

      default:
        CLog::LogF(LOGINFO, "Ignoring entry of unknown edit action: {}",
                   static_cast<int>(edit.action));
        break;
    }
  }

  return result;
}
