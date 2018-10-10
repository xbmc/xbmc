/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLShader.h"
#include "rendering/RenderSystem.h"
#include "utils/Color.h"

#include <array>
#include <memory>

#include "system_gl.h"

enum ESHADERMETHOD
{
  SM_DEFAULT = 0,
  SM_TEXTURE,
  SM_TEXTURE_LIM,
  SM_MULTI,
  SM_FONTS,
  SM_TEXTURE_NOBLEND,
  SM_MULTI_BLENDCOLOR,
  SM_MAX
};

class CRenderSystemGL : public CRenderSystemBase
{
public:
  CRenderSystemGL();
  ~CRenderSystemGL() override;
  bool InitRenderSystem() override;
  bool DestroyRenderSystem() override;
  bool ResetRenderSystem(int width, int height) override;

  bool BeginRender() override;
  bool EndRender() override;
  void PresentRender(bool rendered, bool videoLayer) override;
  bool ClearBuffers(UTILS::Color color) override;
  bool IsExtSupported(const char* extension) const override;

  void SetVSync(bool vsync);
  void ResetVSync() { m_bVsyncInit = false; }

  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  bool ScissorsCanEffectClipping() override;
  CRect ClipRectToScissorRect(const CRect &rect) override;
  void SetScissors(const CRect &rect) override;
  void ResetScissors() override;

  void CaptureStateBlock() override;
  void ApplyStateBlock() override;

  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f) override;

  void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view) override;
  bool SupportsStereo(RENDER_STEREO_MODE mode) const override;
  bool SupportsNPOT(bool dxt) const override;

  void Project(float &x, float &y, float &z) override;

  std::string GetShaderPath(const std::string &filename) override;

  void GetGLVersion(int& major, int& minor);
  void GetGLSLVersion(int& major, int& minor);

  void ResetGLErrors();

  // shaders
  void EnableShader(ESHADERMETHOD method);
  void DisableShader();
  GLint ShaderGetPos();
  GLint ShaderGetCol();
  GLint ShaderGetCoord0();
  GLint ShaderGetCoord1();
  GLint ShaderGetUniCol();
  GLint ShaderGetModel();

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual void PresentRenderImpl(bool rendered) = 0;
  void CalculateMaxTexturesize();
  void InitialiseShaders();
  void ReleaseShaders();

  bool m_bVsyncInit = false;
  int m_width;
  int m_height;

  std::string m_RenderExtensions;

  int m_glslMajor = 0;
  int m_glslMinor = 0;

  GLint m_viewPort[4];

  std::array<std::unique_ptr<CGLShader>, SM_MAX> m_pShader;
  ESHADERMETHOD m_method = SM_DEFAULT;
  GLuint m_vertexArray = GL_NONE;
};
