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
  virtual ~CRendererMediaCodec();

  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  virtual void AddVideoPicture(const VideoPicture &picture, int index) override;
  virtual void ReleaseBuffer(int idx) override;

  // Feature support
  virtual CRenderInfo GetRenderInfo() override;

protected:
  // textures
  virtual bool UploadTexture(int index) override;
  virtual void DeleteTexture(int index) override;
  virtual bool CreateTexture(int index) override;

  // hooks for hw dec renderer
  virtual bool LoadShadersHook() override;
  virtual bool RenderHook(int index) override;

private:
  float m_textureMatrix[16];
};
