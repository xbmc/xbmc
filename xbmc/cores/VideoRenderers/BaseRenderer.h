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

#include "guilib/Resolution.h"
#include "guilib/Geometry.h"
#include "RenderFormats.h"

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
  unsigned bpp; /* bytes per pixel */
} YV12Image;

enum EFIELDSYNC
{
  FS_NONE,
  FS_TOP,
  FS_BOT
};

enum ERENDERFEATURE
{
  RENDERFEATURE_GAMMA,
  RENDERFEATURE_BRIGHTNESS,
  RENDERFEATURE_CONTRAST,
  RENDERFEATURE_NOISE,
  RENDERFEATURE_SHARPNESS,
  RENDERFEATURE_NONLINSTRETCH,
  RENDERFEATURE_ROTATION,
  RENDERFEATURE_STRETCH,
  RENDERFEATURE_CROP,
  RENDERFEATURE_ZOOM,
  RENDERFEATURE_VERTICAL_SHIFT,
  RENDERFEATURE_PIXEL_RATIO,
  RENDERFEATURE_POSTPROCESS
};

typedef void (*RenderUpdateCallBackFn)(const void *ctx, const CRect &SrcRect, const CRect &DestRect);

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

  virtual bool Supports(ERENDERFEATURE feature) { return false; }

  // Supported pixel formats, can be called before configure
  std::vector<ERenderFormat> SupportedFormats()  { return std::vector<ERenderFormat>(); }

  virtual void RegisterRenderUpdateCallBack(const void *ctx, RenderUpdateCallBackFn fn);

protected:
  void       ChooseBestResolution(float fps);
  bool       FindResolutionFromOverride(float fps, float& weight, bool fallback);
  void       FindResolutionFromFpsMatch(float fps, float& weight);
  RESOLUTION FindClosestResolution(float fps, float multiplier, RESOLUTION current, float& weight);
  float      RefreshWeight(float refresh, float fps);
  void       CalcNormalDisplayRect(float offsetX, float offsetY, float screenWidth, float screenHeight, float inputFrameRatio, float zoomAmount, float verticalShift);
  void       CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height);
  void       ManageDisplay();

  virtual void       ReorderDrawPoints();//might be overwritten (by egl e.x.)
  void       saveRotatedCoords();//saves the current state of m_rotatedDestCoords
  void       syncDestRectToRotatedPoints();//sync any changes of m_destRect to m_rotatedDestCoords
  void       restoreRotatedCoords();//restore the current state of m_rotatedDestCoords from saveRotatedCoords 
  void       MarkDirty();

  RESOLUTION m_resolution;    // the resolution we're running in
  unsigned int m_sourceWidth;
  unsigned int m_sourceHeight;
  float m_sourceFrameRatio;
  float m_fps;

  unsigned int m_renderOrientation; // orientation of the video in degress counter clockwise
  unsigned int m_oldRenderOrientation; // orientation of the previous frame
  // for drawing the texture with glVertex4f (holds all 4 corner points of the destination rect
  // with correct orientation based on m_renderOrientation
  // 0 - top left, 1 - top right, 2 - bottom right, 3 - bottom left
  CPoint m_rotatedDestCoords[4];
  CPoint m_savedRotatedDestCoords[4];//saved points from saveRotatedCoords call

  CRect m_destRect;
  CRect m_oldDestRect; // destrect of the previous frame
  CRect m_sourceRect;

  // rendering flags
  unsigned m_iFlags;

  const void* m_RenderUpdateCallBackCtx;
  RenderUpdateCallBackFn m_RenderUpdateCallBackFn;
};
