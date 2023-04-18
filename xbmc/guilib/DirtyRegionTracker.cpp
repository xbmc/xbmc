/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirtyRegionTracker.h"

#include "DirtyRegionSolvers.h"
#include "ServiceBroker.h"
#include "guilib/DirtyRegion.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <algorithm>
#include <stdio.h>

CDirtyRegionTracker::CDirtyRegionTracker(int buffering)
{
  m_buffering = buffering;
  m_solver = NULL;
}

CDirtyRegionTracker::~CDirtyRegionTracker()
{
  delete m_solver;
}

void CDirtyRegionTracker::SelectAlgorithm()
{
  delete m_solver;

  switch (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAlgorithmDirtyRegions)
  {
    case DIRTYREGION_SOLVER_FILL_VIEWPORT_ON_CHANGE:
      CLog::Log(LOGDEBUG, "guilib: Fill viewport on change for solving rendering passes");
      m_solver = new CFillViewportOnChangeRegionSolver();
      break;
    case DIRTYREGION_SOLVER_COST_REDUCTION:
      CLog::Log(LOGDEBUG, "guilib: Cost reduction as algorithm for solving rendering passes");
      m_solver = new CGreedyDirtyRegionSolver();
      break;
    case DIRTYREGION_SOLVER_UNION:
      m_solver = new CUnionDirtyRegionSolver();
      CLog::Log(LOGDEBUG, "guilib: Union as algorithm for solving rendering passes");
      break;
    case DIRTYREGION_SOLVER_FILL_VIEWPORT_ALWAYS:
    default:
      CLog::Log(LOGDEBUG, "guilib: Fill viewport always for solving rendering passes");
      m_solver = new CFillViewportAlwaysRegionSolver();
      break;
  }
}

void CDirtyRegionTracker::MarkDirtyRegion(const CDirtyRegion &region)
{
  if (!region.IsEmpty())
    m_markedRegions.push_back(region);
}

const CDirtyRegionList &CDirtyRegionTracker::GetMarkedRegions() const
{
  return m_markedRegions;
}

CDirtyRegionList CDirtyRegionTracker::GetDirtyRegions()
{
  CDirtyRegionList output;

  if (m_solver)
    m_solver->Solve(m_markedRegions, output);

  return output;
}

void CDirtyRegionTracker::CleanMarkedRegions()
{
  int buffering = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiVisualizeDirtyRegions ? 20 : m_buffering;
  m_markedRegions.erase(
      std::remove_if(m_markedRegions.begin(), m_markedRegions.end(),
                     [buffering](CDirtyRegion& r) { return r.UpdateAge() >= buffering; }),
      m_markedRegions.end());
}
