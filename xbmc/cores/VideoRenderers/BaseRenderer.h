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

#include "guilib/Resolution.h"
#include "guilib/Geometry.h"

#define MAX_PLANES 3
#define MAX_FIELDS 3

typedef struct YV12Image
{
  BYTE *   plane[MAX_PLANES];
  int      planesize[MAX_PLANES];
  unsigned stride[MAX_PLANES];
  unsigned width;
  unsigned height;
  unsigned flags;

  unsigned cshift_x; /* this is the chroma shift used */
  unsigned cshift_y;
} YV12Image;

enum ERENDERFEATURE
{
  RENDERFEATURE_GAMMA,
  RENDERFEATURE_BRIGHTNESS,
  RENDERFEATURE_CONTRAST,
  RENDERFEATURE_NOISE,
  RENDERFEATURE_SHARPNESS,
  RENDERFEATURE_NONLINSTRETCH
};

struct DVDVideoPicture;

class CBaseRenderer
{
public:
  CBaseRenderer();
  virtual ~CBaseRenderer();

  void SetViewMode(int viewMode);
  RESOLUTION GetResolution() const;
  void GetVideoRect(CRect &source, CRect &dest);
  float GetAspectRatio() const;

  virtual bool AddVideoPicture(DVDVideoPicture* picture) { return false; }
  virtual void Flush() {};

  virtual unsigned int GetProcessorSize() { return 0; }

protected:
  void       ChooseBestResolution(float fps);
  bool       FindResolutionFromOverride(float fps, float& weight, bool fallback);
  void       FindResolutionFromFpsMatch(float fps, float& weight);
  RESOLUTION FindClosestResolution(float fps, float multiplier, RESOLUTION current, float& weight);
  float      RefreshWeight(float refresh, float fps);
  void       CalcNormalDisplayRect(float offsetX, float offsetY, float screenWidth, float screenHeight, float inputFrameRatio, float zoomAmount, float verticalShift);
  void       CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height);
  void       ManageDisplay();

  RESOLUTION m_resolution;    // the resolution we're running in
  unsigned int m_sourceWidth;
  unsigned int m_sourceHeight;
  float m_sourceFrameRatio;
  float m_fps;

  CRect m_destRect;
  CRect m_sourceRect;
};
