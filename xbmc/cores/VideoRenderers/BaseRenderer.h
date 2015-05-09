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

#include <vector>

#include "guilib/Resolution.h"
#include "guilib/Geometry.h"
#include "RenderFormats.h"
#include "RenderFeatures.h"

#define MAX_PLANES 3
#define MAX_FIELDS 3
#define NUM_BUFFERS 6

class CSetting;

typedef struct YV12Image
{
  uint8_t* plane[MAX_PLANES];
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

// Render Methods
enum RenderMethods
{
  RENDER_METHOD_AUTO     = 0,
  RENDER_METHOD_ARB,
  RENDER_METHOD_GLSL,
  RENDER_METHOD_SOFTWARE,
  RENDER_METHOD_D3D_PS,
  RENDER_METHOD_DXVA,
  RENDER_METHOD_DXVAHD,
  RENDER_OVERLAYS        = 99   // to retain compatibility
};

typedef void (*RenderUpdateCallBackFn)(const void *ctx, const CRect &SrcRect, const CRect &DestRect);
typedef void (*RenderFeaturesCallBackFn)(const void *ctx, Features &renderFeatures);

struct DVDVideoPicture;

class CBaseRenderer
{
public:
  CBaseRenderer();
  virtual ~CBaseRenderer();

  void SetViewMode(int viewMode);
  RESOLUTION GetResolution() const;

  /*! \brief Get video rectangle and view window
  \param source is original size of the video
  \param dest is the target rendering area honoring aspect ratio of source
  \param view is the entire target rendering area for the video (including black bars)
  */
  void GetVideoRect(CRect &source, CRect &dest, CRect &view);
  float GetAspectRatio() const;

  virtual bool AddVideoPicture(DVDVideoPicture* picture, int index) { return false; }
  virtual void Flush() {};

  /**
   * Returns number of references a single buffer can retain when rendering a single frame
   */
  virtual void         SetBufferSize(int numBuffers) { }
  virtual void         ReleaseBuffer(int idx) { }
  virtual bool         NeedBufferForRef(int idx) { return false; }
  virtual bool         IsGuiLayer() { return true; }

  virtual bool Supports(ERENDERFEATURE feature) { return false; }

  // Render info, can be called before configure
  virtual CRenderInfo GetRenderInfo() { return CRenderInfo(); }

  virtual void RegisterRenderUpdateCallBack(const void *ctx, RenderUpdateCallBackFn fn);
  virtual void RegisterRenderFeaturesCallBack(const void *ctx, RenderFeaturesCallBackFn fn);

  static void SettingOptionsRenderMethodsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

protected:
  void       ChooseBestResolution(float fps);
  bool       FindResolutionFromOverride(float fps, float& weight, bool fallback);
  void       FindResolutionFromFpsMatch(float fps, float& weight);
  RESOLUTION FindClosestResolution(float fps, float multiplier, RESOLUTION current, float& weight);
  static float      RefreshWeight(float refresh, float fps);
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
  CRect m_viewRect;

  // rendering flags
  unsigned m_iFlags;
  ERenderFormat m_format;

  const void* m_RenderUpdateCallBackCtx;
  RenderUpdateCallBackFn m_RenderUpdateCallBackFn;

  const void* m_RenderFeaturesCallBackCtx;
  RenderFeaturesCallBackFn m_RenderFeaturesCallBackFn;
};
