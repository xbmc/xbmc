/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLShader.h"
#include "rendering/RenderSystem.h"
#include "utils/ColorUtils.h"
#include "utils/Map.h"

#include <map>
#include <memory>

#include <fmt/format.h>

#include "system_gl.h"

enum class ShaderMethodGL
{
  SM_DEFAULT = 0,
  SM_TEXTURE,
  SM_TEXTURE_LIM,
  SM_MULTI,
  SM_FONTS,
  SM_FONTS_SHADER_CLIP,
  SM_TEXTURE_NOBLEND,
  SM_MULTI_BLENDCOLOR,
  SM_MAX
};

template<>
struct fmt::formatter<ShaderMethodGL> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(const ShaderMethodGL& shaderMethod, FormatContext& ctx)
  {
    const auto it = ShaderMethodGLMap.find(shaderMethod);
    if (it == ShaderMethodGLMap.cend())
      throw std::range_error("no string mapping found for shader method");

    return fmt::formatter<string_view>::format(it->second, ctx);
  }

private:
  static constexpr auto ShaderMethodGLMap = make_map<ShaderMethodGL, std::string_view>({
      {ShaderMethodGL::SM_DEFAULT, "default"},
      {ShaderMethodGL::SM_TEXTURE, "texture"},
      {ShaderMethodGL::SM_TEXTURE_LIM, "texture limited"},
      {ShaderMethodGL::SM_MULTI, "multi"},
      {ShaderMethodGL::SM_FONTS, "fonts"},
      {ShaderMethodGL::SM_FONTS_SHADER_CLIP, "fonts with vertex shader based clipping"},
      {ShaderMethodGL::SM_TEXTURE_NOBLEND, "texture no blending"},
      {ShaderMethodGL::SM_MULTI_BLENDCOLOR, "multi blend colour"},
  });

  static_assert(static_cast<size_t>(ShaderMethodGL::SM_MAX) == ShaderMethodGLMap.size(),
                "ShaderMethodGLMap doesn't match the size of ShaderMethodGL, did you forget to "
                "add/remove a mapping?");
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
  void InvalidateColorBuffer() override;
  bool ClearBuffers(UTILS::COLOR::Color color) override;
  bool IsExtSupported(const char* extension) const override;

  void SetVSync(bool vsync);
  void ResetVSync() { m_bVsyncInit = false; }

  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  bool ScissorsCanEffectClipping() override;
  CRect ClipRectToScissorRect(const CRect &rect) override;
  void SetScissors(const CRect &rect) override;
  void ResetScissors() override;

  void SetDepthCulling(DEPTH_CULLING culling) override;

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
  void EnableShader(ShaderMethodGL method);
  void DisableShader();
  GLint ShaderGetPos();
  GLint ShaderGetCol();
  GLint ShaderGetCoord0();
  GLint ShaderGetCoord1();
  GLint ShaderGetDepth();
  GLint ShaderGetUniCol();
  GLint ShaderGetModel();
  GLint ShaderGetMatrix();
  GLint ShaderGetClip();
  GLint ShaderGetCoordStep();

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

  std::map<ShaderMethodGL, std::unique_ptr<CGLShader>> m_pShader;
  ShaderMethodGL m_method = ShaderMethodGL::SM_DEFAULT;
  GLuint m_vertexArray = GL_NONE;
};
