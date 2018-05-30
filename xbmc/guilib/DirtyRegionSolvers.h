/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "IDirtyRegionSolver.h"

class CUnionDirtyRegionSolver : public IDirtyRegionSolver
{
public:
  void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) override;
};

class CFillViewportAlwaysRegionSolver : public IDirtyRegionSolver
{
public:
  void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) override;
};

class CFillViewportOnChangeRegionSolver : public IDirtyRegionSolver
{
public:
  void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) override;
};

class CGreedyDirtyRegionSolver : public IDirtyRegionSolver
{
public:
  CGreedyDirtyRegionSolver();
  void Solve(const CDirtyRegionList &input, CDirtyRegionList &output) override;
private:
  float m_costNewRegion;
  float m_costPerArea;
};
