/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef GUILIB_DIRTYREGIONTRACKER_H_INCLUDED
#define GUILIB_DIRTYREGIONTRACKER_H_INCLUDED
#include "DirtyRegionTracker.h"
#endif

#ifndef GUILIB_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define GUILIB_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif

#ifndef GUILIB_UTILS_LOG_H_INCLUDED
#define GUILIB_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

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

  switch (g_advancedSettings.m_guiAlgorithmDirtyRegions)
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
  int buffering = g_advancedSettings.m_guiVisualizeDirtyRegions ? 20 : m_buffering;
  int i = m_markedRegions.size() - 1;
  while (i >= 0)
	{
    if (m_markedRegions[i].UpdateAge() >= buffering)
      m_markedRegions.erase(m_markedRegions.begin() + i);

    i--;
  }
}
