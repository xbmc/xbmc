#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DirtyRegion.h"

#define DIRTYREGION_SOLVER_FILL_VIEWPORT_ALWAYS 0
#define DIRTYREGION_SOLVER_UNION 1
#define DIRTYREGION_SOLVER_COST_REDUCTION 2
#define DIRTYREGION_SOLVER_FILL_VIEWPORT_ON_CHANGE 3

class IDirtyRegionSolver
{
public:
  virtual ~IDirtyRegionSolver() { }

  // Takes a number of dirty regions which will become a number of needed rendering passes.
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) = 0;
};
