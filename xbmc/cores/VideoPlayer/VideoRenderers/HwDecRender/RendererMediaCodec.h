/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"

class CRendererMediaCodec : public CLinuxRendererGLES
{
public:
  CRendererMediaCodec();
  ~CRendererMediaCodec() override;

  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void ReleaseBuffer(int idx) override;

  // Feature support
  CRenderInfo GetRenderInfo() override;

protected:
  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;

  // hooks for hw dec renderer
  bool LoadShadersHook() override;
  bool RenderHook(int index) override;

private:
  float m_textureMatrix[16];
};
