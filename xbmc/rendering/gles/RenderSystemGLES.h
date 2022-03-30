/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLESShader.h"
#include "rendering/RenderSystem.h"
#include "utils/ColorUtils.h"

#include <map>

#include <fmt/format.h>

#include "system_gl.h"

enum class ShaderMethodGLES
{
  SM_DEFAULT,
  SM_TEXTURE,
  SM_MULTI,
  SM_FONTS,
  SM_TEXTURE_NOBLEND,
  SM_MULTI_BLENDCOLOR,
  SM_TEXTURE_RGBA,
  SM_TEXTURE_RGBA_OES,
  SM_TEXTURE_RGBA_BLENDCOLOR,
  SM_TEXTURE_RGBA_BOB,
  SM_TEXTURE_RGBA_BOB_OES,
  SM_TEXTURE_NOALPHA
};

template<>
struct fmt::formatter<ShaderMethodGLES> : fmt::formatter<std::string_view>
{

public:
  static constexpr auto toString(ShaderMethodGLES method)
  {
    switch (method)
    {
      case ShaderMethodGLES::SM_DEFAULT:
        return "default";
        break;
      case ShaderMethodGLES::SM_TEXTURE:
        return "texture";
        break;
      case ShaderMethodGLES::SM_MULTI:
        return "multi";
        break;
      case ShaderMethodGLES::SM_FONTS:
        return "fonts";
        break;
      case ShaderMethodGLES::SM_TEXTURE_NOBLEND:
        return "texture no blending";
        break;
      case ShaderMethodGLES::SM_MULTI_BLENDCOLOR:
        return "multi blend colour";
        break;
      case ShaderMethodGLES::SM_TEXTURE_RGBA:
        return "texure rgba";
        break;
      case ShaderMethodGLES::SM_TEXTURE_RGBA_OES:
        return "texure rgba OES";
        break;
      case ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR:
        return "texture rgba blend colour";
        break;
      case ShaderMethodGLES::SM_TEXTURE_RGBA_BOB:
        return "texure rgba bob";
        break;
      case ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES:
        return "texure rgba bob OES";
        break;
      case ShaderMethodGLES::SM_TEXTURE_NOALPHA:
        return "texture no alpha";
        break;
      default:
        return "unknown";
        break;
    }
  }

  template<typename FormatContext>
  constexpr auto format(const ShaderMethodGLES& shaderMethod, FormatContext& ctx)
  {
    auto shaderName = toString(shaderMethod);
    return fmt::formatter<std::string_view>::format(shaderName, ctx);
  }
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
  bool ClearBuffers(UTILS::COLOR::Color color) override;
  bool IsExtSupported(const char* extension) const override;

  void SetVSync(bool vsync);
  void ResetVSync() { m_bVsyncInit = false; }

  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  bool ScissorsCanEffectClipping() override;
  CRect ClipRectToScissorRect(const CRect &rect) override;
  void SetScissors(const CRect& rect) override;
  void ResetScissors() override;

  void CaptureStateBlock() override;
  void ApplyStateBlock() override;

  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f) override;

  bool SupportsStereo(RENDER_STEREO_MODE mode) const override;

  void Project(float &x, float &y, float &z) override;

  std::string GetShaderPath(const std::string &filename) override { return "GLES/2.0/"; }

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

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual void PresentRenderImpl(bool rendered) = 0;
  void CalculateMaxTexturesize();

  bool m_bVsyncInit{false};
  int m_width;
  int m_height;

  std::string m_RenderExtensions;

  std::map<ShaderMethodGLES, std::unique_ptr<CGLESShader>> m_pShader;
  ShaderMethodGLES m_method = ShaderMethodGLES::SM_DEFAULT;

  GLint      m_viewPort[4];
};
