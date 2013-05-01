/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DirtyRegionSolvers.h"
#include "GraphicContext.h"
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
  CRect screen(0,0,(float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
  output.push_back(CDirtyRegion(screen));
}

void CFillViewportOnChangeRegionSolver::Solve(const CDirtyRegionList &input, CDirtyRegionList &output)
{
  CRect screen(0,0,(float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
  if (input.size() > 0)
    output.assign(1,CDirtyRegion(screen));
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
        // TODO if the temporaryCost is 0 then we could skip checking the other regions since there exist no better solution
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
