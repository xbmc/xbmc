/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirtyRegionSolvers.h"
#include "windowing/GraphicContext.h"
#include <stdio.h>

void CFillViewportAlwaysRegionSolver::Solve(const CDirtyRegionList &input, CDirtyRegionList &output)
{
  CDirtyRegion unifiedRegion(CServiceBroker::GetWinSystem()->GetGfxContext().GetViewWindow());
  output.push_back(unifiedRegion);
}

void CFillViewportOnChangeRegionSolver::Solve(const CDirtyRegionList &input, CDirtyRegionList &output)
{
  if (!input.empty())
    output.assign(1,CDirtyRegion(CServiceBroker::GetWinSystem()->GetGfxContext().GetViewWindow()));
}
