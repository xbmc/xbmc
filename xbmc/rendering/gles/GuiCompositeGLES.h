/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/FrameBufferObject.h"

#include <memory>

class CGuiCompositeShaderGLES;
class CRenderSystemGLES;

class CGuiCompositeGLES
{
public:
  CGuiCompositeGLES();
  ~CGuiCompositeGLES();

  bool Enable(int colorTransfer, bool limitedRange, float sdrPeak = 203.0f / 10000.0f);
  void Disable();
  bool IsActive() const { return m_guiCompositing; }
  void SetSdrPeak(float peak);

  bool Prepare(int width, int height, bool attachDepth);
  bool Begin(bool guiWillRender, int width, int height, bool attachDepth);
  void End(bool videoOnSeparatePlane);
  void Composite(CRenderSystemGLES& renderSystem,
                 bool videoOnSeparatePlane,
                 unsigned int guiElementCount);

private:
  bool m_guiCompositing{false};
  CFrameBufferObject m_guiFbo;
  int m_guiFboWidth{0};
  int m_guiFboHeight{0};
  bool m_guiFboAttachDepth{false};
  // True when the GUI FBO is empty (no draws this frame); Composite skips composite when true.
  bool m_guiFboClean{false};
  // Whether the GUI render pass will run this frame; set by Begin.
  bool m_guiWillRender{true};

  std::unique_ptr<CGuiCompositeShaderGLES> m_compositeShader;
};
