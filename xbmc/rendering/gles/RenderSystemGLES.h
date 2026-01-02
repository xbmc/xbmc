/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLESShader.h"
#include "rendering/RenderSystem.h"
#include "utils/ColorUtils.h"
#include "utils/Map.h"

#include <array>
#include <map>
#include <vector>

#include <fmt/format.h>

#include "system_gl.h"

enum class ShaderMethodGLES
{
  SM_DEFAULT,
  SM_TEXTURE,
  SM_TEXTURE_111R,
  SM_MULTI,
  SM_MULTI_RGBA_111R,
  SM_FONTS,
  SM_FONTS_SHADER_CLIP,
  SM_TEXTURE_NOBLEND,
  SM_MULTI_BLENDCOLOR,
  SM_MULTI_RGBA_111R_BLENDCOLOR,
  SM_MULTI_111R_111R_BLENDCOLOR,
  SM_TEXTURE_RGBA,
  SM_TEXTURE_RGBA_OES,
  SM_TEXTURE_RGBA_BLENDCOLOR,
  SM_TEXTURE_RGBA_BOB,
  SM_TEXTURE_RGBA_BOB_OES,
  SM_TEXTURE_NOALPHA,
  SM_ROUNDRECT_MASK,
  SM_MAX
};

template<>
struct fmt::formatter<ShaderMethodGLES> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(const ShaderMethodGLES& shaderMethod, FormatContext& ctx)
  {
    const auto it = ShaderMethodGLESMap.find(shaderMethod);
    if (it == ShaderMethodGLESMap.cend())
      throw std::range_error("no string mapping found for shader method");

    return fmt::formatter<string_view>::format(it->second, ctx);
  }

private:
  static constexpr auto ShaderMethodGLESMap = make_map<ShaderMethodGLES, std::string_view>({
      {ShaderMethodGLES::SM_DEFAULT, "default"},
      {ShaderMethodGLES::SM_TEXTURE, "texture"},
      {ShaderMethodGLES::SM_TEXTURE_111R, "alpha texture with diffuse color"},
      {ShaderMethodGLES::SM_MULTI, "multi"},
      {ShaderMethodGLES::SM_MULTI_RGBA_111R, "multi with color/alpha texture"},
      {ShaderMethodGLES::SM_FONTS, "fonts"},
      {ShaderMethodGLES::SM_FONTS_SHADER_CLIP, "fonts with vertex shader based clipping"},
      {ShaderMethodGLES::SM_TEXTURE_NOBLEND, "texture no blending"},
      {ShaderMethodGLES::SM_MULTI_BLENDCOLOR, "multi blend colour"},
      {ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR,
       "multi with color/alpha texture and blend color"},
      {ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR,
       "multi with alpha/alpha texture and blend color"},
      {ShaderMethodGLES::SM_TEXTURE_RGBA, "texture rgba"},
      {ShaderMethodGLES::SM_TEXTURE_RGBA_OES, "texture rgba OES"},
      {ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR, "texture rgba blend colour"},
      {ShaderMethodGLES::SM_TEXTURE_RGBA_BOB, "texture rgba bob"},
      {ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES, "texture rgba bob OES"},
      {ShaderMethodGLES::SM_TEXTURE_NOALPHA, "texture no alpha"},
      {ShaderMethodGLES::SM_ROUNDRECT_MASK, "roundrect_mask"},
  });

  static_assert(static_cast<size_t>(ShaderMethodGLES::SM_MAX) == ShaderMethodGLESMap.size(),
                "ShaderMethodGLESMap doesn't match the size of ShaderMethodGLES, did you forget to "
                "add/remove a mapping?");
};

class CRenderSystemGLES : public CRenderSystemBase
{
public:
  CRenderSystemGLES();
  ~CRenderSystemGLES() override = default;

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
  void SetScissors(const CRect& rect) override;
  void ResetScissors() override;

  bool BeginOffscreenRoundedGroup(const CRect& rectScreenTL, float radiusPx) override;
  bool BeginOffscreenRoundedGroup(const CRect& rectScreenTL,
                                  const std::array<float, 4>& radiiPx) override;
  void EndOffscreenRoundedGroup() override;
  void SetDepthCulling(DEPTH_CULLING culling) override;

  void CaptureStateBlock() override;
  void ApplyStateBlock() override;

  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f) override;

  bool SupportsStereo(RENDER_STEREO_MODE mode) const override;

  void Project(float &x, float &y, float &z) override;

  std::string GetShaderPath(const std::string& filename) override;

  void InitialiseShaders();
  void ReleaseShaders();
  void EnableGUIShader(ShaderMethodGLES method);
  void DisableGUIShader();

  GLint GUIShaderGetPos();
  GLint GUIShaderGetCol();
  GLint GUIShaderGetCoord0();
  GLint GUIShaderGetCoord1();
  GLint GUIShaderGetUniCol();
  GLint GUIShaderGetCoord0Matrix();
  GLint GUIShaderGetField();
  GLint GUIShaderGetStep();
  GLint GUIShaderGetContrast();
  GLint GUIShaderGetBrightness();
  GLint GUIShaderGetModel();
  GLint GUIShaderGetMatrix();
  GLint GUIShaderGetClip();
  GLint GUIShaderGetCoordStep();
  GLint GUIShaderGetDepth();

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual void PresentRenderImpl(bool rendered) = 0;
  void CalculateMaxTexturesize();

  bool m_bVsyncInit{false};
  int m_width;
  int m_height;

  std::string m_RenderExtensions;

  std::map<ShaderMethodGLES, std::unique_ptr<CGLESShader>> m_pShader;

  // Round-rect mask shader locations (SM_ROUNDRECT_MASK).
  GLint m_maskRectLoc{-1}; // m_maskRect
  GLint m_maskRadiiLoc{-1}; // m_radii (vec4 tl,tr,br,bl)
  GLint m_maskAAWidthLoc{-1}; // m_aaWidth
  GLint m_maskViewportLoc{-1}; // m_viewport (vec4 x,y,w,h)
  GLint m_maskSamplerLoc{-1}; // m_samp0
  GLint m_maskMatrixLoc{-1}; // m_matrix
  GLint m_roundMaskPosLoc{-1}; // m_attrpos

  // Fullscreen quad resources for round-rect composite.
  GLuint m_roundMaskVbo{0};

  // Offscreen rounded group helpers (GLES only).
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

  ShaderMethodGLES m_method = ShaderMethodGLES::SM_DEFAULT;

  GLint      m_viewPort[4];
};
