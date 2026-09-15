/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/FrameBufferObject.h"

#include <memory>

class CGuiCompositeShaderGLES;

/*!
 * \brief HDR GUI compositing: the GUI is rendered in sRGB into an FBO and
 * composited through the sRGB -> PQ/HLG shader into the back buffer.
 *
 * The windowing system drives it from the CWinSystemBase GUI compositing
 * virtuals and answers the one platform question it has: whether the video is
 * on a separate plane or surface (videoOnSeparatePlane), so that the display
 * hardware or the system compositor blends the GUI over the video, or whether
 * the video was drawn into the same back buffer.
 */
class CGuiCompositeGLES
{
public:
  CGuiCompositeGLES();
  ~CGuiCompositeGLES();

  // colorTransfer: AVCOL_TRC_SMPTE2084 (PQ) or AVCOL_TRC_ARIB_STD_B67 (HLG), 0 to disable
  bool Enable(int colorTransfer, bool limitedRange);
  // guiWillRender: whether the GUI render pass runs this frame. When false the
  // FBO is neither bound nor cleared, so its content survives the frame.
  bool Begin(bool guiWillRender, int width, int height, bool attachDepth);
  void End(bool videoOnSeparatePlane);
  void Composite(bool videoOnSeparatePlane, unsigned int guiElementCount);
  bool IsActive() const { return m_guiCompositing; }

private:
  bool m_guiCompositing{false};
  CFrameBufferObject m_guiFbo;
  int m_guiFboWidth{0};
  int m_guiFboHeight{0};
  // True when the GUI FBO is empty (no draws this frame); Composite skips the pass when true.
  bool m_guiFboClean{false};
  // Whether the GUI render pass will run this frame; set by Begin.
  bool m_guiWillRender{true};

  std::unique_ptr<CGuiCompositeShaderGLES> m_compositeShader;
};
