/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>
#include <stack>
#include <map>
#include "threads/CriticalSection.h"
#include "utils/TransformMatrix.h"        // for the members m_guiTransform etc.
#include "utils/Geometry.h"               // for CRect/CPoint
#include "Resolution.h"
#include "rendering/RenderSystem.h"
#include "utils/Color.h"

// required by clients
#include "ServiceBroker.h"
#include "WinSystem.h"

#define D3DPRESENTFLAG_INTERLACED   1
#define D3DPRESENTFLAG_WIDESCREEN   2
#define D3DPRESENTFLAG_PROGRESSIVE  4
#define D3DPRESENTFLAG_MODE3DSBS    8
#define D3DPRESENTFLAG_MODE3DTB    16

/* what types are important for mode setting */
#define D3DPRESENTFLAG_MODEMASK ( D3DPRESENTFLAG_INTERLACED \
                                  | D3DPRESENTFLAG_MODE3DSBS  \
                                  | D3DPRESENTFLAG_MODE3DTB   )

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
  ADJUST_REFRESHRATE_OFF = 0,
  ADJUST_REFRESHRATE_ALWAYS,
  ADJUST_REFRESHRATE_ON_STARTSTOP,
  ADJUST_REFRESHRATE_ON_START,
};

class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext();

  // methods related to windowing
  float GetFPS() const;
  void SetFPS(float fps);
  float GetDisplayLatency() const;
  bool IsFullScreenRoot() const;
  void ToggleFullScreen();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION res, bool forceUpdate);
  void ApplyModeChange(RESOLUTION res);
  void ApplyWindowResize(int newWidth, int newHeight);
  RESOLUTION GetVideoResolution() const;
  const RESOLUTION_INFO GetResInfo() const;
  const RESOLUTION_INFO GetResInfo(RESOLUTION res) const;
  void SetResInfo(RESOLUTION res, const RESOLUTION_INFO& info);

  void Flip(bool rendered, bool videoLayer);

  // gfx contect interface
  int GetWidth() const;
  int GetHeight() const;
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();
  void SetScissors(const CRect &rect);
  void ResetScissors();
  const CRect &GetScissors() const;
  const CRect GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetOverscan(RESOLUTION_INFO &resinfo);
  void ResetScreenParameters(RESOLUTION res);
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear(UTILS::Color color = 0);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res);

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
  void InvertFinalCoords(float &x, float &y) const;
  float ScaleFinalXCoord(float x, float y) const;
  float ScaleFinalYCoord(float x, float y) const;
  float ScaleFinalZCoord(float x, float y) const;
  void ScaleFinalCoords(float &x, float &y, float &z) const;
  bool RectIsAngled(float x1, float y1, float x2, float y2) const;
  const TransformMatrix &GetGUIMatrix() const;
  float GetGUIScaleX() const;
  float GetGUIScaleY() const;
  UTILS::Color MergeAlpha(UTILS::Color color) const;
  void SetOrigin(float x, float y);
  void RestoreOrigin();
  void SetCameraPosition(const CPoint &camera);
  void SetStereoView(RENDER_STEREO_VIEW view);
  RENDER_STEREO_VIEW GetStereoView()  { return m_stereoView; }
  void SetStereoMode(RENDER_STEREO_MODE mode) { m_nextStereoMode = mode; }
  RENDER_STEREO_MODE GetStereoMode()  { return m_stereoMode; }
  void RestoreCameraPosition();
  void SetStereoFactor(float factor);
  void RestoreStereoFactor();
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
  void RestoreClipRegion();
  void ClipRect(CRect &vertex, CRect &texture, CRect *diffuse = NULL);
  CRect GetClipRegion();
  void AddGUITransform();
  TransformMatrix AddTransform(const TransformMatrix &matrix);
  void SetTransform(const TransformMatrix &matrix);
  void SetTransform(const TransformMatrix &matrix, float scaleX, float scaleY);
  void RemoveTransform();

  /* modifies final coordinates according to stereo mode if needed */
  CRect StereoCorrection(const CRect &rect) const;
  CPoint StereoCorrection(const CPoint &point) const;

  CRect GenerateAABB(const CRect &rect) const;

  //@todo move those somewhere else
  const std::string& GetMediaDir() const;
  void SetMediaDir(const std::string& strMediaDir);

protected:

  void UpdateCameraPosition(const CPoint &camera, const float &factor);
  void SetVideoResolutionInternal(RESOLUTION res, bool forceUpdate);
  void ApplyVideoResolution(RESOLUTION res);
  void UpdateInternalStateWithResolution(RESOLUTION res);

  int m_iScreenHeight = 576;
  int m_iScreenWidth = 720;
  std::string m_strMediaDir;
  CRect m_videoRect;
  bool m_bFullScreenRoot = true;
  bool m_bFullScreenVideo = false;
  bool m_bCalibrating = false;
  RESOLUTION m_Resolution = RES_INVALID;
  float m_fFPSOverride = 0.0f;

  RESOLUTION_INFO m_windowResolution;
  std::stack<CPoint> m_cameras;
  std::stack<CPoint> m_origins;
  std::stack<CRect> m_clipRegions;
  std::stack<float> m_stereoFactors;
  std::stack<CRect> m_viewStack;
  CRect m_scissors;

  class UITransform
  {
  public:
    UITransform() : matrix() {}
    UITransform(const TransformMatrix& m, const float sX = 1.0f, const float sY = 1.0f)
      : matrix(m), scaleX(sX), scaleY(sY)
    {
    }
    void Reset()
    {
      matrix.Reset();
      scaleX = scaleY = 1.0f;
    }

    TransformMatrix matrix;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
  };

  UITransform m_guiTransform;
  UITransform m_finalTransform;
  std::stack<UITransform> m_transforms;
  RENDER_STEREO_VIEW m_stereoView = RENDER_STEREO_VIEW_OFF;
  RENDER_STEREO_MODE m_stereoMode = RENDER_STEREO_MODE_OFF;
  RENDER_STEREO_MODE m_nextStereoMode = RENDER_STEREO_MODE_OFF;
};
