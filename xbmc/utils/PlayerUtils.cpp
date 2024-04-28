/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerUtils.h"

#include "FileItem.h"
#include "application/ApplicationPlayer.h"
#include "music/MusicUtils.h"
#include "utils/Variant.h"
#include "video/guilib/VideoGUIUtils.h"

using namespace KODI;

bool CPlayerUtils::IsItemPlayable(const CFileItem& itemIn)
{
  const CFileItem item(itemIn.GetItemToPlay());

  // General
  if (item.IsParentFolder())
    return false;

  // Plugins
  if (item.IsPlugin() && item.GetProperty("isplayable").asBoolean())
    return true;

  // Music
  if (MUSIC_UTILS::IsItemPlayable(item))
    return true;

  // Movies / TV Shows / Music Videos
  if (VIDEO::UTILS::IsItemPlayable(item))
    return true;

  //! @todo add more types on demand.

  return false;
}

void CPlayerUtils::AdvanceTempoStep(std::shared_ptr<CApplicationPlayer> appPlayer,
                                    TempoStepChange change)
{
  const auto step = 0.1f;
  const auto currentTempo = appPlayer->GetPlayTempo();
  switch (change)
  {
    case TempoStepChange::INCREASE:
      appPlayer->SetTempo(currentTempo + step);
      break;
    case TempoStepChange::DECREASE:
      appPlayer->SetTempo(currentTempo - step);
      break;
  }
}
