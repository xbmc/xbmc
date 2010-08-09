#pragma once
#include "DirtyRegion.h"

class IDirtyRegionSolver
{
public:
  virtual ~IDirtyRegionSolver() { }

  // Takes a number of dirty regions which will become a number of needed rendering passes.
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) = 0;

  // State the time taken for said region. This method allows the solver to optimize forthcomming results
  virtual void TimingInformation(float time) { }
};
