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

/*!
\file GraphicContext.h
\brief
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even whith optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif


#include <string>
#include <vector>
#include <stack>
#include <map>
#include "threads/CriticalSection.h"  // base class
#include "TransformMatrix.h"        // for the members m_guiTransform etc.
#include "Geometry.h"               // for CRect/CPoint
#include "gui3d.h"
#include "Resolution.h"
#include "utils/GlobalsHandling.h"
#include "DirtyRegion.h"
#include "settings/lib/ISettingCallback.h"
#include "rendering/RenderSystem.h"

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_INFO,
                 VIEW_TYPE_BIG_INFO,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };

enum AdjustRefreshRate
{
  ADJUST_REFRESHRATE_OFF          = 0,
  ADJUST_REFRESHRATE_ALWAYS,
  ADJUST_REFRESHRATE_ON_STARTSTOP
};

class CGraphicContext : public CCriticalSection,
                        public ISettingCallback
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);

  virtual void OnSettingChanged(const CSetting *setting);

  // the following two functions should wrap any
  // GL calls to maintain thread safety
  void BeginPaint(bool lock=true);
  void EndPaint(bool lock=true);

  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  float GetFPS() const;
  const std::string& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const std::string& strMediaDir);
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();

  void SetScissors(const CRect &rect);
  void ResetScissors();
  const CRect &GetScissors() const { return m_scissors; }

  const CRect GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  bool IsFullScreenRoot() const;
  bool ToggleFullScreenRoot();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION res, bool forceUpdate = false);
  RESOLUTION GetVideoResolution() const;
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetOverscan(RESOLUTION_INFO &resinfo);
  void ResetScreenParameters(RESOLUTION res);
  void Lock() { lock(); }
  void Unlock() { unlock(); }
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear(color_t color = 0);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res);

  // output scaling
  const RESOLUTION_INFO GetResInfo() const
  {
    return GetResInfo(m_Resolution);
  }
  const RESOLUTION_INFO GetResInfo(RESOLUTION res) const;
  void SetResInfo(RESOLUTION res, const RESOLUTION_INFO& info);

  /* \brief Get UI scaling information from a given resolution to the screen resolution.
   Takes account of overscan and UI zooming.
   \param res the resolution to scale from.
   \param scaleX [out] the scaling amount in the X direction.
   \param scaleY [out] the scaling amount in the Y direction.
   \param matrix [out] if non-NULL, a suitable transformation from res to screen resolution is set.
   */
  void GetGUIScaling(const RESOLUTION_INFO &res, float &scaleX, float &scaleY, TransformMatrix *matrix = NULL);

  void SetRenderingResolution(const RESOLUTION_INFO &res, bool needsScaling);  ///< Sets scaling up for rendering
  void SetScalingResolution(const RESOLUTION_INFO &res, bool needsScaling);    ///< Sets scaling up for skin loading etc.
  float GetScalingPixelRatio() const;
  void Flip(const CDirtyRegionList& dirty);
  void InvertFinalCoords(float &x, float &y) const;
  inline float ScaleFinalXCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.matrix.TransformXCoord(x, y, 0); }
  inline float ScaleFinalYCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.matrix.TransformYCoord(x, y, 0); }
  inline float ScaleFinalZCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.matrix.TransformZCoord(x, y, 0); }
  inline void ScaleFinalCoords(float &x, float &y, float &z) const XBMC_FORCE_INLINE { m_finalTransform.matrix.TransformPosition(x, y, z); }
  bool RectIsAngled(float x1, float y1, float x2, float y2) const;

  inline float GetGUIScaleX() const XBMC_FORCE_INLINE { return m_finalTransform.scaleX; }
  inline float GetGUIScaleY() const XBMC_FORCE_INLINE { return m_finalTransform.scaleY; }
  inline color_t MergeAlpha(color_t color) const XBMC_FORCE_INLINE
  {
    color_t alpha = m_finalTransform.matrix.TransformAlpha((color >> 24) & 0xff);
    if (alpha > 255) alpha = 255;
    return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
  }

  void SetOrigin(float x, float y);
  void RestoreOrigin();
  void SetCameraPosition(const CPoint &camera);
  void SetStereoView(RENDER_STEREO_VIEW view);
  RENDER_STEREO_VIEW GetStereoView()  { return m_stereoView; }
  void SetStereoMode(RENDER_STEREO_MODE mode) { m_nextStereoMode = mode; }
  RENDER_STEREO_MODE GetStereoMode()  { return m_stereoMode; }
  void RestoreCameraPosition();
  /*! \brief Set a region in which to clip all rendering
   Anything that is rendered after setting a clip region will be clipped so that no part renders
   outside of the clip region.  Successive calls to SetClipRegion intersect the clip region, which
   means the clip region may eventually become an empty set.  In this case SetClipRegion returns false
   to indicate that no rendering need be performed.

   This call must be matched with a RestoreClipRegion call unless SetClipRegion returns false.

   Usage should be of the form:

     if (SetClipRegion(x, y, w, h))
     {
       ...
       perform rendering
       ...
       RestoreClipRegion();
     }

   \param x the left-most coordinate of the clip region
   \param y the top-most coordinate of the clip region
   \param w the width of the clip region
   \param h the height of the clip region
   \returns true if the region is set and the result is non-empty. Returns false if the resulting region is empty.
   \sa RestoreClipRegion
   */
  bool SetClipRegion(float x, float y, float w, float h);

   /*! \brief Restore a clip region to the previous clip region (if any) prior to the last SetClipRegion call
    This function should be within an if (SetClipRegion(x,y,w,h)) block.
    \sa SetClipRegion
    */
  void RestoreClipRegion();
  void ApplyHardwareTransform();
  void RestoreHardwareTransform();
  void ClipRect(CRect &vertex, CRect &texture, CRect *diffuse = NULL);
  inline void AddGUITransform()
  {
    m_transforms.push(m_finalTransform);
    m_finalTransform = m_guiTransform;
  }
  inline TransformMatrix AddTransform(const TransformMatrix &matrix)
  {
    m_transforms.push(m_finalTransform);
    m_finalTransform.matrix *= matrix;
    return m_finalTransform.matrix;
  }
  inline void SetTransform(const TransformMatrix &matrix)
  {
   m_transforms.push(m_finalTransform);
   m_finalTransform.matrix = matrix;
  }
  inline void SetTransform(const TransformMatrix &matrix, float scaleX, float scaleY)
  {
    m_transforms.push(m_finalTransform);
    m_finalTransform.matrix = matrix;
    m_finalTransform.scaleX = scaleX;
    m_finalTransform.scaleY = scaleY;
  }
  inline void RemoveTransform()
  {
    if (!m_transforms.empty())
    {
      m_finalTransform = m_transforms.top();
      m_transforms.pop();
    }
  }

  /* modifies final coordinates according to stereo mode if needed */
  CRect StereoCorrection(const CRect &rect) const;
  CPoint StereoCorrection(const CPoint &point) const;

  CRect generateAABB(const CRect &rect) const;

protected:
  std::stack<CRect> m_viewStack;

  int m_iScreenHeight;
  int m_iScreenWidth;
  int m_iScreenId;
  std::string m_strMediaDir;
  CRect m_videoRect;
  bool m_bFullScreenRoot;
  bool m_bFullScreenVideo;
  bool m_bCalibrating;
  RESOLUTION m_Resolution;

private:
  class UITransform
  {
  public:
    UITransform() : matrix(), scaleX(1.0f), scaleY(1.0f) {};
    UITransform(const TransformMatrix &m, const float sX = 1.0f, const float sY = 1.0f) : matrix(m), scaleX(sX), scaleY(sY) { };
    void Reset() { matrix.Reset(); scaleX = scaleY = 1.0f; };

    TransformMatrix matrix;
    float scaleX;
    float scaleY;
  };
  void UpdateCameraPosition(const CPoint &camera);
  // this method is indirectly called by the public SetVideoResolution
  // it only works when called from mainthread (thats what SetVideoResolution ensures)
  void SetVideoResolutionInternal(RESOLUTION res, bool forceUpdate);
  RESOLUTION_INFO m_windowResolution;
  std::stack<CPoint> m_cameras;
  std::stack<CPoint> m_origins;
  std::stack<CRect>  m_clipRegions;

  UITransform m_guiTransform;
  UITransform m_finalTransform;
  std::stack<UITransform> m_transforms;
  RENDER_STEREO_VIEW m_stereoView;
  RENDER_STEREO_MODE m_stereoMode;
  RENDER_STEREO_MODE m_nextStereoMode;

  CRect m_scissors;
};

/*!
 \ingroup graphics
 \brief
 */

XBMC_GLOBAL(CGraphicContext,g_graphicsContext);

#endif
