/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGameClientDiscModel CGameClientDiscMergeUtils::ReconcileModels(
    const CGameClientDiscModel& frontendDiscs, const CGameClientDiscModel& coreDiscs)
{
  CGameClientDiscModel mergedDiscs;

  // Set ejected
  mergedDiscs.SetEjected(coreDiscs.IsEjected());

  // Set discs
  for (size_t discIndex = 0; discIndex < coreDiscs.Size(); ++discIndex)
  {
    const GameClientDiscEntry& coreDisc = coreDiscs.GetDiscs().at(discIndex);

    // Some cores don't compact indexes when removing discs, they simply mark
    // the index as an empty slot. In this case, check if the slot was
    // previously removed.
    if (frontendDiscs.Size() <= coreDiscs.Size() && coreDisc.path.empty())
    {
      // Check if it's a removed disc in the frontend
      if (frontendDiscs.IsRemovedSlotByIndex(discIndex))
      {
        mergedDiscs.AddRemovedSlot();
        continue;
      }
    }

    mergedDiscs.AddDisc(coreDisc.path, coreDisc.cachedLabel);
  }

  // Set selected
  if (!coreDiscs.IsSelectedNoDisc())
    mergedDiscs.SetSelectedDiscByIndex(*coreDiscs.GetSelectedDiscIndex());
  else
    mergedDiscs.SetSelectedNoDisc();

  return mergedDiscs;
}
