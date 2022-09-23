/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VdpauGL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"

class CRendererVDPAU : public CLinuxRendererGL
{
public:
  CRendererVDPAU();
  ~CRendererVDPAU() override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;

  // Player functions
  void ReleaseBuffer(int idx) override;
  bool ConfigChanged(const VideoPicture &picture) override;
  bool NeedBuffer(int idx) override;
  bool Flush(bool saveBuffers) override;

  // Feature support
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

protected:
  bool LoadShadersHook() override;
  bool RenderHook(int idx) override;
  void AfterRenderHook(int idx) override;

  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;

  bool CreateVDPAUTexture(int index);
  void DeleteVDPAUTexture(int index);
  bool UploadVDPAUTexture(int index);

  bool CreateVDPAUTexture420(int index);
  void DeleteVDPAUTexture420(int index);
  bool UploadVDPAUTexture420(int index);

  EShaderFormat GetShaderFormat() override;

  bool CanSaveBuffers() override { return false; }

  bool m_isYuv = false;

  VDPAU::CInteropState m_interopState;
  VDPAU::CVdpauTexture m_vdpauTextures[NUM_BUFFERS];
  GLsync m_fences[NUM_BUFFERS];
};
