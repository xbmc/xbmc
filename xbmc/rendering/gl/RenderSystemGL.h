/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system_gl.h"
#include "GLShader.h"
#include "rendering/RenderSystem.h"
#include "utils/Color.h"

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
  bool m_supportsNPOT = true;

  std::string m_RenderExtensions;

  int m_glslMajor = 0;
  int m_glslMinor = 0;

  GLint m_viewPort[4];

  std::unique_ptr<CGLShader*[]> m_pShader;
  ESHADERMETHOD m_method = SM_DEFAULT;
  GLuint m_vertexArray = GL_NONE;
};
