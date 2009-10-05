#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Resolution.h"
#include "Geometry.h"

class CBaseRenderer
{
public:
  CBaseRenderer();
  virtual ~CBaseRenderer();

  void SetViewMode(int viewMode);
  RESOLUTION GetResolution() const;
  void GetVideoRect(CRect &source, CRect &dest);
  float GetAspectRatio() const;
  virtual void AutoCrop(bool bCrop) {};

protected:
  void ChooseBestResolution(float fps);
  void CalcNormalDisplayRect(float offsetX, float offsetY, float screenWidth, float screenHeight, float inputFrameRatio, float zoomAmount);
  void CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height);
  void CBaseRenderer::ManageDisplay();

  RESOLUTION m_resolution;    // the resolution we're running in
  unsigned int m_sourceWidth;
  unsigned int m_sourceHeight;
  float m_sourceFrameRatio;

  CRect m_destRect;
  CRect m_sourceRect;
};
