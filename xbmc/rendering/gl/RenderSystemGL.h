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

#include <array>
#include <map>
#include <memory>
#include <vector>

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
  SM_ROUNDRECT_MASK,
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
      {ShaderMethodGL::SM_ROUNDRECT_MASK, "roundrect_mask"},
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
  bool ClearBuffers(KODI::UTILS::COLOR::Color color) override;
  bool IsExtSupported(const char* extension) const override;

  void SetVSync(bool vsync);
  void ResetVSync() { m_bVsyncInit = false; }

  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  bool ScissorsCanEffectClipping() override;
  CRect ClipRectToScissorRect(const CRect &rect) override;
  void SetScissors(const CRect &rect) override;
  void ResetScissors() override;

  bool BeginOffscreenRoundedGroup(const CRect& rectScreenTL, float radiusPx) override;
  bool BeginOffscreenRoundedGroup(const CRect& rectScreenTL,
                                  const std::array<float, 4>& radiiPx) override;
  void EndOffscreenRoundedGroup() override;

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

  // Round-rect mask shader locations (SM_ROUNDRECT_MASK).
  GLint m_maskRectLoc{-1}; // m_maskRect
  GLint m_maskRadiiLoc{-1}; // m_radii (vec4 tl,tr,br,bl)
  GLint m_maskAAWidthLoc{-1}; // m_aaWidth
  GLint m_maskViewportLoc{-1}; // m_viewport (vec4 x,y,w,h)
  GLint m_maskSamplerLoc{-1}; // m_samp0
  GLint m_maskMatrixLoc{-1}; // m_matrix
  GLint m_roundMaskPosLoc{-1}; // m_attrpos

  // Fullscreen quad resources for round-rect composite (core profile wants VAO).
  GLuint m_roundMaskVao{0};
  GLuint m_roundMaskVbo{0};

  // Offscreen rounded group helpers (GL only).
  bool EnsureGroupFbo(int w, int h);

  // Offscreen render target (RGBA).
  GLuint m_groupFbo{0};
  GLuint m_groupTex{0};
  int m_groupW{0};
  int m_groupH{0};

  struct OffscreenGroupState
  {
    GLint prevFbo{0};
    GLint prevViewport[4]{0, 0, 0, 0};
    CRect rectScreenTL;
    std::array<float, 4> radiiPx{0.0f, 0.0f, 0.0f, 0.0f};
  };

  // Allow nested rounded groups safely.
  std::vector<OffscreenGroupState> m_groupStack;

  ShaderMethodGL m_method = ShaderMethodGL::SM_DEFAULT;
  GLuint m_vertexArray = GL_NONE;
};
