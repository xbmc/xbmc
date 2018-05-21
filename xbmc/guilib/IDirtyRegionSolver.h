#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
  virtual ~IDirtyRegionSolver() = default;

  // Takes a number of dirty regions which will become a number of needed rendering passes.
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) = 0;
};
