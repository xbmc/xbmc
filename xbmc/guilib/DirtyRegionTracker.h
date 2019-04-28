/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirtyRegionSolver.h"

#if defined(TARGET_DARWIN_EMBEDDED)
#define DEFAULT_BUFFERING 4
#else
#define DEFAULT_BUFFERING 3
#endif

class CDirtyRegionTracker
{
public:
  explicit CDirtyRegionTracker(int buffering = DEFAULT_BUFFERING);
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
