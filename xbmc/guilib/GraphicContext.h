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


#include <vector>
#include <stack>
#include <map>
#include "threads/CriticalSection.h"  // base class
#include "TransformMatrix.h"        // for the members m_guiTransform etc.
#include "Geometry.h"               // for CRect/CPoint
#include "gui3d.h"
#include "utils/StdString.h"
#include "Resolution.h"
#include "utils/GlobalsHandling.h"
#include "DirtyRegion.h"

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


class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);

  // the following two functions should wrap any
  // GL calls to maintain thread safety
  void BeginPaint(bool lock=true);
  void EndPaint(bool lock=true);

  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  float GetFPS() const;
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir);
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
  float GetPixelRatio(RESOLUTION iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear(color_t color = 0);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res);

  // output scaling
  const RESOLUTION_INFO &GetResInfo() const;
  void SetRenderingResolution(const RESOLUTION_INFO &res, bool needsScaling);  ///< Sets scaling up for rendering
  void SetScalingResolution(const RESOLUTION_INFO &res, bool needsScaling);    ///< Sets scaling up for skin loading etc.
  float GetScalingPixelRatio() const;
  void Flip(const CDirtyRegionList& dirty);
  void InvertFinalCoords(float &x, float &y) const;
  inline float ScaleFinalXCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformXCoord(x, y, 0); }
  inline float ScaleFinalYCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformYCoord(x, y, 0); }
  inline float ScaleFinalZCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformZCoord(x, y, 0); }
  inline void ScaleFinalCoords(float &x, float &y, float &z) const XBMC_FORCE_INLINE { m_finalTransform.TransformPosition(x, y, z); }
  bool RectIsAngled(float x1, float y1, float x2, float y2) const;

  inline float GetGUIScaleX() const XBMC_FORCE_INLINE { return m_guiScaleX; }
  inline float GetGUIScaleY() const XBMC_FORCE_INLINE { return m_guiScaleY; }
  inline color_t MergeAlpha(color_t color) const XBMC_FORCE_INLINE
  {
    color_t alpha = m_finalTransform.TransformAlpha((color >> 24) & 0xff);
    if (alpha > 255) alpha = 255;
    return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
  }

  void SetOrigin(float x, float y);
  void RestoreOrigin();
  void SetCameraPosition(const CPoint &camera);
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
  inline unsigned int AddGUITransform()
  {
    unsigned int size = m_groupTransform.size();
    m_groupTransform.push(m_guiTransform);
    UpdateFinalTransform(m_groupTransform.top());
    return size;
  }
  inline TransformMatrix AddTransform(const TransformMatrix &matrix)
  {
    ASSERT(m_groupTransform.size());
    TransformMatrix absoluteMatrix = m_groupTransform.size() ? m_groupTransform.top() * matrix : matrix;
    m_groupTransform.push(absoluteMatrix);
    UpdateFinalTransform(absoluteMatrix);
    return absoluteMatrix;
  }
  inline void SetTransform(const TransformMatrix &matrix)
  {
    // TODO: We only need to add it to the group transform as other transforms may be added on top of this one later on
    //       Once all transforms are cached then this can be removed and UpdateFinalTransform can be called directly
    ASSERT(m_groupTransform.size());
    m_groupTransform.push(matrix);
    UpdateFinalTransform(m_groupTransform.top());
  }
  inline unsigned int RemoveTransform()
  {
    ASSERT(m_groupTransform.size());
    if (m_groupTransform.size())
      m_groupTransform.pop();
    if (m_groupTransform.size())
      UpdateFinalTransform(m_groupTransform.top());
    else
      UpdateFinalTransform(TransformMatrix());
    return m_groupTransform.size();
  }

  CRect generateAABB(const CRect &rect) const;

protected:
  std::stack<CRect> m_viewStack;

  int m_iScreenHeight;
  int m_iScreenWidth;
  int m_iScreenId;
  CStdString m_strMediaDir;
  CRect m_videoRect;
  bool m_bFullScreenRoot;
  bool m_bFullScreenVideo;
  bool m_bCalibrating;
  RESOLUTION m_Resolution;

private:
  void UpdateCameraPosition(const CPoint &camera);
  void UpdateFinalTransform(const TransformMatrix &matrix);
  RESOLUTION_INFO m_windowResolution;
  float m_guiScaleX;
  float m_guiScaleY;
  std::stack<CPoint> m_cameras;
  std::stack<CPoint> m_origins;
  std::stack<CRect>  m_clipRegions;

  TransformMatrix m_guiTransform;
  TransformMatrix m_finalTransform;
  std::stack<TransformMatrix> m_groupTransform;

  CRect m_scissors;
};

/*!
 \ingroup graphics
 \brief
 */

XBMC_GLOBAL(CGraphicContext,g_graphicsContext);

#endif
