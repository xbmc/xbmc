/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DebugInfo.h"
#include "RenderCapture.h"
#include "RenderInfo.h"
#include "VideoShaders/ShaderFormats.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "utils/Geometry.h"

#include <utility>
#include <vector>

#define MAX_FIELDS 3
#define NUM_BUFFERS 6

class CSetting;
struct IntegerSettingOption;

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
  RENDER_METHOD_GLSL,
  RENDER_METHOD_SOFTWARE,
  RENDER_METHOD_D3D_PS,
  RENDER_METHOD_DXVA,
  RENDER_OVERLAYS        = 99   // to retain compatibility
};

struct VideoPicture;

class CBaseRenderer
{
public:
  CBaseRenderer();
  virtual ~CBaseRenderer();

  // Player functions
  virtual bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) = 0;
  virtual bool IsConfigured() = 0;
  virtual void AddVideoPicture(const VideoPicture &picture, int index) = 0;
  virtual bool IsPictureHW(const VideoPicture& picture) { return false; }
  virtual void UnInit() = 0;
  virtual bool Flush(bool saveBuffers) { return false; }
  virtual void SetBufferSize(int numBuffers) { }
  virtual void ReleaseBuffer(int idx) { }
  virtual bool NeedBuffer(int idx) { return false; }
  virtual bool IsGuiLayer() { return true; }
  // Render info, can be called before configure
  virtual CRenderInfo GetRenderInfo() { return CRenderInfo(); }
  virtual void Update() = 0;
  virtual void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) = 0;
  virtual bool RenderCapture(CRenderCapture* capture) = 0;
  virtual bool ConfigChanged(const VideoPicture &picture) = 0;

  // Feature support
  virtual bool SupportsMultiPassRendering() = 0;
  virtual bool Supports(ERENDERFEATURE feature) const { return false; }
  virtual bool Supports(ESCALINGMETHOD method) const = 0;

  virtual bool WantsDoublePass() { return false; }

  void SetViewMode(int viewMode);

  /*! \brief Get video rectangle and view window
  \param source is original size of the video
  \param dest is the target rendering area honoring aspect ratio of source
  \param view is the entire target rendering area for the video (including black bars)
  */
  void GetVideoRect(CRect& source, CRect& dest, CRect& view) const;
  float GetAspectRatio() const;

  static void SettingOptionsRenderMethodsFiller(const std::shared_ptr<const CSetting>& setting,
                                                std::vector<IntegerSettingOption>& list,
                                                int& current,
                                                void* data);

  void SetVideoSettings(const CVideoSettings &settings);

  // Gets debug info from render buffer
  virtual DEBUG_INFO_VIDEO GetDebugInfo(int idx) { return {}; }

  virtual CRenderCapture* GetRenderCapture() { return nullptr; }

protected:
  void CalcDestRect(float offsetX,
                    float offsetY,
                    float width,
                    float height,
                    float inputFrameRatio,
                    float zoomAmount,
                    float verticalShift,
                    CRect& destRect);
  void CalcNormalRenderRect(float offsetX, float offsetY, float width, float height,
                            float inputFrameRatio, float zoomAmount, float verticalShift);
  void CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height);
  virtual void ManageRenderArea();
  virtual void ReorderDrawPoints();
  virtual EShaderFormat GetShaderFormat();
  void MarkDirty();
  void EnableAlwaysClip();

  //@todo drop those
  void saveRotatedCoords();//saves the current state of m_rotatedDestCoords
  void syncDestRectToRotatedPoints();//sync any changes of m_destRect to m_rotatedDestCoords
  void restoreRotatedCoords();//restore the current state of m_rotatedDestCoords from saveRotatedCoords

  unsigned int m_sourceWidth = 720;
  unsigned int m_sourceHeight = 480;
  float m_sourceFrameRatio = 1.0f;
  float m_fps = 0.0f;

  unsigned int m_renderOrientation = 0; // orientation of the video in degrees counter clockwise
  // for drawing the texture with glVertex4f (holds all 4 corner points of the destination rect
  // with correct orientation based on m_renderOrientation
  // 0 - top left, 1 - top right, 2 - bottom right, 3 - bottom left
  CPoint m_rotatedDestCoords[4];
  CPoint m_savedRotatedDestCoords[4];//saved points from saveRotatedCoords call

  CRect m_destRect;
  CRect m_sourceRect;
  CRect m_viewRect;

  // rendering flags
  unsigned m_iFlags = 0;
  AVPixelFormat m_format = AV_PIX_FMT_NONE;

  CVideoSettings m_videoSettings;

private:
  bool m_alwaysClip = false;
};
