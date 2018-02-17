/*
 *      Copyright (C) 2007-2015 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "VdpauGL.h"

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

  // Feature support
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

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

  bool m_isYuv = false;

  VDPAU::CInteropState m_interopState;
  VDPAU::CVdpauTexture m_vdpauTextures[NUM_BUFFERS];
  GLsync m_fences[NUM_BUFFERS];
};
