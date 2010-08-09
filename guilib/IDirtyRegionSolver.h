#pragma once
#include "DirtyRegion.h"

#define DIRTYREGION_SOLVER_NONE 0
#define DIRTYREGION_SOLVER_UNION 1
#define DIRTYREGION_SOLVER_COST_REDUCTION 2

class IDirtyRegionSolver
{
public:
  virtual ~IDirtyRegionSolver() { }

  // Takes a number of dirty regions which will become a number of needed rendering passes.
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) = 0;
};
