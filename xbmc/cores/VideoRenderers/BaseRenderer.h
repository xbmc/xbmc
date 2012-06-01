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
#include "cores/VideoRenderers/RenderFlags.h"
#include "cores/VideoRenderers/RenderFormats.h"
#include "settings/VideoSettings.h"

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
  RENDERFEATURE_NONLINSTRETCH
};

struct DVDVideoPicture;

class CBaseRenderer
{
public:
  CBaseRenderer();
  virtual ~CBaseRenderer();

  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format);
  void SetViewMode(int viewMode);
  RESOLUTION GetResolution() const;
  void GetVideoRect(CRect &source, CRect &dest);
  float GetAspectRatio() const;

  virtual bool AddVideoPicture(DVDVideoPicture* picture) { return false; }
  virtual void Flush() {};

  virtual unsigned int GetProcessorSize() { return 0; }

  // Supported pixel formats, can be called before configure
  virtual std::vector<ERenderFormat> SupportedFormats() { return m_formats; }

  virtual bool IsConfigured() { return m_bConfigured; }
protected:
  void       ChooseBestResolution();
  bool       FindResolutionFromOverride(float fps, float& weight, bool fallback);
  void       FindResolutionFromFpsMatch(float fps, float& weight);
  RESOLUTION FindClosestResolution(float fps, float multiplier, RESOLUTION current, float& weight);
  float      RefreshWeight(float refresh, float fps);
  void       CalcNormalDisplayRect(float offsetX, float offsetY, float screenWidth, float screenHeight, float inputFrameRatio, float zoomAmount, float verticalShift);
  void       CalculateFrameAspectRatio();
  void       ManageDisplay();

  RESOLUTION                  m_resolution;    // the resolution we're running in
  unsigned int                m_sourceWidth;
  unsigned int                m_sourceHeight;
  unsigned int                m_desiredWidth;
  unsigned int                m_desiredHeight;
  float                       m_sourceFrameRatio;
  float                       m_fps;

  CRect                       m_destRect;
  CRect                       m_sourceRect;

  bool                        m_bConfigured;

  unsigned int                m_iFlags;
  unsigned int                m_extended_format;
  ERenderFormat               m_format;
  std::vector<ERenderFormat>  m_formats;

  ESCALINGMETHOD              m_scalingMethod;
  ESCALINGMETHOD              m_scalingMethodGui;
};
