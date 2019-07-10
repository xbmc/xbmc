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

void CUnionDirtyRegionSolver::Solve(const CDirtyRegionList &input, CDirtyRegionList &output)
{
  CDirtyRegion unifiedRegion;
  for (unsigned int i = 0; i < input.size(); i++)
    unifiedRegion.Union(input[i]);

  if (!unifiedRegion.IsEmpty())
    output.push_back(unifiedRegion);
}

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

CGreedyDirtyRegionSolver::CGreedyDirtyRegionSolver()
{
  m_costNewRegion = 10.0f;
  m_costPerArea   = 0.01f;
}

void CGreedyDirtyRegionSolver::Solve(const CDirtyRegionList &input, CDirtyRegionList &output)
{
  for (unsigned int i = 0; i < input.size(); i++)
  {
    CDirtyRegion possibleUnionRegion;
    int   possibleUnionNbr = -1;
    float possibleUnionCost = 100000.0f;

    CDirtyRegion currentRegion = input[i];
    for (unsigned int j = 0; j < output.size(); j++)
    {
      CDirtyRegion temporaryUnion = output[j];
      temporaryUnion.Union(currentRegion);
      float temporaryCost = m_costPerArea * (temporaryUnion.Area() - output[j].Area());
      if (temporaryCost < possibleUnionCost)
      {
        //! @todo if the temporaryCost is 0 then we could skip checking the other regions since there exist no better solution
        possibleUnionRegion = temporaryUnion;
        possibleUnionNbr    = j;
        possibleUnionCost   = temporaryCost;
      }
    }

    float newRegionTotalCost = m_costPerArea * currentRegion.Area() + m_costNewRegion;

    if (possibleUnionNbr >= 0 && possibleUnionCost < newRegionTotalCost)
      output[possibleUnionNbr] = possibleUnionRegion;
    else
      output.push_back(currentRegion);
  }
}
