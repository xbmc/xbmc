#pragma once
#include "IDirtyRegionSolver.h"
#include "DirtyRegionSolvers.h"

class CDirtyRegionTracker
{
public:
  CDirtyRegionTracker(int buffering = 2);
  ~CDirtyRegionTracker();
  void SelectAlgorithm();
  void MarkDirtyRegion(const CDirtyRegion &region);

  CDirtyRegionList GetDirtyRegions();
  void CleanMarkedRegions();

private:
  CDirtyRegionList m_markedRegions;
  int m_buffering;
  IDirtyRegionSolver *m_solver;
};
