#include "DirtyRegionTracker.h"

#include <stdio.h>

CDirtyRegionTracker::CDirtyRegionTracker(int buffering)
{
  m_buffering = buffering;
  m_solver = new CUnionDirtyRegionSolver();
}

CDirtyRegionTracker::~CDirtyRegionTracker()
{
  delete m_solver;
}

void CDirtyRegionTracker::MarkDirtyRegion(const CDirtyRegion &region)
{
  if (!region.IsEmpty())
    m_markedRegions.push_back(region);
}

CDirtyRegionList CDirtyRegionTracker::GetDirtyRegions()
{
  CDirtyRegionList output;

  if (m_markedRegions.size() > 0)
    m_solver->Solve(m_markedRegions, output);

  return output;
}

void CDirtyRegionTracker::TimingInformation(float time)
{
  m_solver->TimingInformation(time);
}

void CDirtyRegionTracker::CleanMarkedRegions()
{
  int i = m_markedRegions.size() - 1;
  while (i >= 0)
	{
    if (m_markedRegions[i].UpdateAge() >= m_buffering)
      m_markedRegions.erase(m_markedRegions.begin() + i);

    i--;
  }
}
