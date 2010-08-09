#pragma once
#include "IDirtyRegionSolver.h"

class CUnionDirtyRegionSolver : public IDirtyRegionSolver
{
public:
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output);
};
