/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDCodecs/Video/NVDEC.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"

/*!
 * \brief GL renderer for NVDEC frames.
 *
 * Decoded frames stay in GPU memory. Their planes are copied device-to-device
 * into GL textures registered with CUDA (CUDA-GL interop), so no frame data
 * goes through system RAM.
 */
class CRendererNVDECGL : public CLinuxRendererGL
{
public:
  CRendererNVDECGL() = default;
  ~CRendererNVDECGL() override;

  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static void Register();

  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;

protected:
  bool LoadShadersHook() override { return false; }
  bool RenderHook(int idx) override { return false; }

  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;

  EShaderFormat GetShaderFormat() override { return SHADER_NV12; }

private:
  struct CudaTextures
  {
    GLuint id[2]{};
    CUgraphicsResource resource[2]{};
    CUcontext context = nullptr;
    AVBufferRef* device = nullptr; // keeps the CUDA context alive
    int width = 0;
    int height = 0;
    AVPixelFormat format = AV_PIX_FMT_NONE;
  };

  bool SetupInterop(CudaTextures& textures, const NVDEC::CVideoBufferNVDEC& buffer);
  void ReleaseInterop(CudaTextures& textures);

  CudaTextures m_cudaTextures[NUM_BUFFERS];
  bool m_interopFailed = false;
};
