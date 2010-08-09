#include "DirtyRegionSolvers.h"
#include <stdio.h>

void CUnionDirtyRegionSolver::Solve(const CDirtyRegionList &input, CDirtyRegionList &output)
{
  CDirtyRegion unifiedRegion;
  for (unsigned int i = 0; i < input.size(); i++)
    unifiedRegion.Union(input[i]);

  if (!unifiedRegion.IsEmpty())
    output.push_back(unifiedRegion);
}
