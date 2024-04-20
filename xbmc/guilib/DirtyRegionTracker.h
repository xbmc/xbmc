/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirtyRegionSolver.h"

class CDirtyRegionTracker
{
public:
  CDirtyRegionTracker();
  ~CDirtyRegionTracker();
  void SelectAlgorithm();
  void MarkDirtyRegion(const CDirtyRegion &region);

  const CDirtyRegionList &GetMarkedRegions() const;
  CDirtyRegionList GetDirtyRegions();
  void CleanMarkedRegions(int bufferAge);

private:
  CDirtyRegionList m_markedRegions;
  IDirtyRegionSolver *m_solver;
};
