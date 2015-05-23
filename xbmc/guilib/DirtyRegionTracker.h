#pragma once
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

#include "IDirtyRegionSolver.h"

#if defined(TARGET_DARWIN_IOS)
#define DEFAULT_BUFFERING 4
#else
#define DEFAULT_BUFFERING 3
#endif

class CDirtyRegionTracker
{
public:
  CDirtyRegionTracker(int buffering = DEFAULT_BUFFERING);
  ~CDirtyRegionTracker();
  void SelectAlgorithm();
  void MarkDirtyRegion(const CDirtyRegion &region);

  const CDirtyRegionList &GetMarkedRegions() const;
  CDirtyRegionList GetDirtyRegions();
  void CleanMarkedRegions();

private:
  CDirtyRegionList m_markedRegions;
  int m_buffering;
  IDirtyRegionSolver *m_solver;
};
